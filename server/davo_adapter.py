'''
conda install 'pytorch=*=cuda*' tensorflow cudatoolkit-dev -c conda-forge
cd flownet2-pytorch
./install.sh
'''

from collections import namedtuple

import numpy as np
import tensorflow as tf
import torch
from numpy import linalg as LA
from scipy.spatial.transform import Rotation as R

try:
    from . import helper
except:
    import helper

helper.push_path('flownet2-pytorch')
from models import FlowNet2
helper.pop_path()

davo_strides = [4, 8, 9, 12, 14, 18]

flownet2_pretrained = helper.pretrained_dir / 'FlowNet2_checkpoint.pth.tar'
segformer_pretrained = helper.pretrained_dir / 'segformer_b4_saved_model'
davo_pretrained = {s: helper.pretrained_dir / f'davo_s{s}_saved_model' for s in davo_strides}

def euler2mat(v):
    r = R.from_euler('zyx', v[:3]).as_matrix()
    m = np.hstack((r, v[3:, np.newaxis]))
    return np.vstack((m, np.array([0, 0, 0, 1])))


class Davo:
    def __init__(self, stride_selection='nearest'):
        self.stride_selection = stride_selection

        FlowNetArgs = namedtuple('FlowNetArgs', ['fp16', 'rgb_max'])
        args = FlowNetArgs(False, 255)
        self.flownet2 = FlowNet2(args).to(helper.device)
        self.flownet2.load_state_dict(torch.load(flownet2_pretrained, helper.device)['state_dict'])

        self.segformer = tf.saved_model.load(str(segformer_pretrained))
        self.davo = {s: tf.saved_model.load(str(p)) for s, p in davo_pretrained.items()}

        self.reset()

    @torch.no_grad()
    def __call__(self, i, x, dist=None):
        if i <= self.last_i:
            self.reset()

        x = x[np.newaxis, ...]
        x_torch = torch.tensor(x, dtype=torch.float, device=helper.device)
        x_torch = x_torch.permute(0, 3, 1, 2)

        if self.last_i >= 0:
            stack = torch.stack((self.last_x_torch, x_torch), 2)
            flow = self.flownet2(stack).permute(0, 2, 3, 1)
            flow = tf.constant(flow.cpu().numpy())

        x_tf = tf.constant(x)
        x_tf_seg = (tf.transpose(x_tf, [0, 3, 1, 2]) / 255 - 0.5) / 0.5
        seg = self.segformer(x=x_tf_seg)['argmax_0.tmp_0']
        seg = tf.cast(seg, tf.float32)
        seg = tf.transpose(seg, [1, 2, 0])[tf.newaxis, ...]

        if self.last_i >= 0:
            if self.stride_selection == 'exact':
                ms = i - self.last_i
            elif self.stride_selection == 'nearest':
                ms = min(davo_strides, key=lambda s: abs(s - (i - self.last_i)))
            else:
                ms = self.stride_selection

            concat = tf.concat((self.last_x_tf, x_tf), 2)
            stack = tf.stack((self.last_seg, seg), 1)
            pose = self.davo[ms].signatures['serving_default']\
                (input_uint8=concat, input_flow=flow, input_seglabel=stack)['pose']
            pose = euler2mat(tf.squeeze(pose).numpy())

            if dist is not None:
                s = (dist - self.last_dist) / LA.norm(pose[:3, 3])
                pose[:3, 3] = pose[:3, 3] * s
        else:
            pose = np.identity(4)
        pose = self.last_pose @ pose

        self.last_i = i
        self.last_dist = dist
        self.last_pose = pose
        self.last_x_torch = x_torch
        self.last_x_tf = x_tf
        self.last_seg = seg

        return pose

    def reset(self):
        self.last_i = -1
        self.last_dist = None
        self.last_pose = np.identity(4)
        self.last_x_torch = None
        self.last_x_tf = None
        self.last_seg = None

if __name__ == '__main__':
    import argparse
    from pathlib import Path

    from evo.core.trajectory import PosePath3D
    from evo.tools import file_interface
    from PIL import Image

    parser = argparse.ArgumentParser()
    parser.add_argument('img_dir', type=Path)
    parser.add_argument('traj_out')
    parser.add_argument('--davo-stride', type=int)
    parser.add_argument('--stride', type=int)
    parser.add_argument('--mask')
    parser.add_argument('--save-gt')
    args = parser.parse_args()

    images = sorted(args.img_dir.glob('*.jpg'))
    indices = np.arange(len(images))[::args.stride]
    images = images[::args.stride]
    if args.mask:
        mask = np.loadtxt(args.mask, bool)
        indices = indices[mask]
        images = np.array(images)[mask]

    davo = Davo(args.davo_stride) if args.davo_stride else Davo()

    poses = []
    for i, im in zip(indices, images):
        im = Image.open(im).resize((448, 128))
        pose = davo(i, np.asarray(im))
        poses.append(pose)
    traj = PosePath3D(poses_se3=poses)
    file_interface.write_kitti_poses_file(args.traj_out, traj)

    if args.save_gt:
        poses_ref = file_interface.read_kitti_poses_file(args.img_dir / 'poses.txt').poses_se3
        poses_ref = poses_ref[::args.stride]
        if args.mask:
            poses_ref = np.array(poses_ref)[mask]
        traj_ref = PosePath3D(poses_se3=poses_ref)
        file_interface.write_kitti_poses_file(args.save_gt, traj_ref)
