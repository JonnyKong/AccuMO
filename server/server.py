import argparse
import io
import queue
import random
import socket
import socketserver
import sys
import threading
import time

import av
import cv2
import numpy as np
import torch
import torchvision.transforms.functional as TF
from google.protobuf import text_format

from . import adabins_adapter
from . import davo_adapter
from . import helper
from . import offload_proto_pb2

device = torch.device('cuda:0' if torch.cuda.is_available() else 'cpu')
depth_adapter = adabins_adapter.AdaBins(
    desired_hw=(256, 832),
    pretrained_path=(helper.pretrained_dir /
                     'UnetAdaptiveBins_06-Feb_13-42-nodebs4-tep25-lr0.000357-wd0.1-bf729f45-c4ba-45c3-be3f-55209d4f8047_best.pt'),
    device=device)
vo_adapter = davo_adapter.Davo()


parser = argparse.ArgumentParser()
parser.add_argument('--decode-mode', type=int, default=0,
                    help='If 0, decode as raw YUV frames. If 1, decode as h264.')
parser.add_argument('--depth-map-fmt', type=int, default=0,
                    help='If 0, send depth map. If 1, send indicator to let client read from disk')
parser.add_argument('--send-dummy-bytes', action='store_true')
args = parser.parse_args()
args.spin_gpu = True


def spin_gpu():
    x1 = torch.rand(512, 512, device='cuda')
    x2 = torch.rand(512, 512, device='cuda')
    _ = x1 @ x2
    torch.cuda.synchronize()


def send_dummy_bytes(n_bytes, wfile):
    metadata_dl = offload_proto_pb2.DownloadMetadata()
    metadata_dl.resultSize = n_bytes
    metadata_dl.taskType = -1

    # Metadata length
    metadata_dl_bytes = metadata_dl.SerializeToString()
    wfile.write(len(metadata_dl_bytes).to_bytes(4, byteorder='big'))

    # Metadata
    wfile.write(metadata_dl_bytes)

    # Payload
    wfile.write(random.randbytes(n_bytes))
    wfile.flush()

    # print('Sent random bytes: ', n_bytes)


@torch.no_grad()
def inference_daemon(q, wfile):
    if args.decode_mode == 1:
        stream = io.BytesIO()
        cur_pos = 0
        has_skipped_config_frame = False

    while True:
        if args.spin_gpu:
            while q.empty():
                spin_gpu()
            data = q.get()
        else:
            data = q.get(block=True, timeout=None)

        if data is None:
            return
        metadata_ul, payload = data

        # Initialize codec
        if args.decode_mode == 1:
            stream.write(payload)
            # Skip the config frame
            if not has_skipped_config_frame:
                has_skipped_config_frame = True
                continue
            stream.seek(cur_pos)

            if cur_pos == 0:
                container = av.open(stream, mode='r', buffer_size=10**8)
                original_codec_ctx = container.streams.video[0].codec_context
                codec = av.codec.CodecContext.create(original_codec_ctx.name, 'r')

        time_proc = time.perf_counter_ns()

        # Decode frame
        if args.decode_mode == 0:
            yuv = np.frombuffer(payload, dtype=np.ubyte).reshape(-1, metadata_ul.imgW)
            if metadata_ul.yuvFormat == 'NV12':
                cvtMode = cv2.COLOR_YUV2RGB_NV12
            elif metadata_ul.yuvFormat == 'I420':
                cvtMode = cv2.COLOR_YUV2RGB_I420
            img = cv2.cvtColor(yuv, cvtMode)
        elif args.decode_mode == 1:
            # Loop will only run once
            for packet in container.demux():
                if packet.size == 0:
                    continue
                cur_pos += packet.size
                frames = codec.decode(packet)

            yuv = frames[0].to_ndarray().reshape((-1, metadata_ul.imgW))
            img = cv2.cvtColor(yuv, cv2.COLOR_YUV2RGB_I420)

        # # Verify decoding is correct
        # cv2.imwrite(f'/home/kong102/tmp/{metadata_ul.frameIdx}.png',
        #             cv2.cvtColor(img, cv2.COLOR_RGB2BGR))

        # Write dummy data to increase throughput
        if args.send_dummy_bytes:
            t = threading.Thread(target=send_dummy_bytes, args=(int(832 * 256 * 6), wfile))
            t.start()

        time_inf_start = time.perf_counter_ns()

        should_do_task_0 = False
        should_do_task_1 = False
        if metadata_ul.taskType == 0:
            should_do_task_0 = True
        elif metadata_ul.taskType == 1:
            should_do_task_1 = True
        elif metadata_ul.taskType == 2:
            should_do_task_0 = True
            should_do_task_1 = True

        metadata_dl = offload_proto_pb2.DownloadMetadata()
        metadata_dl.taskType = metadata_ul.taskType
        metadata_dl.frameIdx = metadata_ul.frameIdx
        pred_bytes = b""

        if should_do_task_0:
            # Convert to size for inference
            img_resized = cv2.resize(img, (832, 256))
            pred = depth_adapter(TF.to_tensor(img_resized).to(device))

            if args.depth_map_fmt == 0:
                pred = (pred * 256).round().astype(np.uint16)
                pred_bytes += pred.tobytes()
                metadata_dl.task0ResultSize = len(pred_bytes)
                metadata_dl.readDepthFromDisk = False
            else:
                metadata_dl.task0ResultSize = 0
                metadata_dl.readDepthFromDisk = True

        if should_do_task_1:
            img_resized = cv2.resize(img, (448, 128))
            pred = vo_adapter(metadata_ul.frameIdx, img_resized, metadata_ul.distance)
            pred_bytes += pred.tobytes()

        time_fin = time.perf_counter_ns()

        metadata_dl.resultSize = len(pred_bytes)
        metadata_dl.timeDecoding = time_inf_start - time_proc
        metadata_dl.timeInference = time_fin - time_inf_start

        if args.send_dummy_bytes:
            t.join()

        # Metadata length
        metadata_dl_bytes = metadata_dl.SerializeToString()
        wfile.write(len(metadata_dl_bytes).to_bytes(4, byteorder='big'))

        # Metadata
        wfile.write(metadata_dl_bytes)

        # Result frame
        wfile.write(pred_bytes)
        wfile.flush()
        print('metadata_dl:', metadata_dl)


class Handler(socketserver.StreamRequestHandler):

    def handle(self) -> None:
        self.request.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

        q = queue.Queue()
        self.thread_inference = threading.Thread(target=inference_daemon,
                                                 args=[q, self.wfile])
        self.thread_inference.start()

        try:
            while True:
                # Metadata length
                metadata_length_bytes = self.rfile.read(4)
                if len(metadata_length_bytes) < 4:
                    break
                metadata_length = int.from_bytes(metadata_length_bytes, byteorder='big')

                # Metadata
                metadata_bytes = self.rfile.read(metadata_length)
                metadata = offload_proto_pb2.UploadMetadata()
                metadata.ParseFromString(metadata_bytes)
                print('metadata_ul: ', text_format.MessageToString(metadata))

                # YUV
                yuv = self.rfile.read(metadata.frameSize)

                q.put((metadata, yuv))

        finally:
            # Notify daemon to quit
            q.put(None)


if __name__ == '__main__':
    # Do a dummy inference for force cold start
    for i in range(4):
        _ = vo_adapter(
            i,
            np.random.randint(0, 255, size=(128, 448, 3), dtype=np.uint8),
            0.0)

    print('Server ready')

    with socketserver.TCPServer(('', 9999), Handler) as server:
        try:
            server.serve_forever()
        finally:
            server.shutdown()
