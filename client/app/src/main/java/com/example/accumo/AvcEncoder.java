package com.example.accumo;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.util.Log;

import java.nio.ByteBuffer;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.TimeUnit;

public class AvcEncoder {
    private final static String TAG = "AvcEncoder";

    public static ArrayBlockingQueue<byte[]> inputQueue = new ArrayBlockingQueue<byte[]>(1000);
    public static ArrayBlockingQueue<byte[]> outputQueue = new ArrayBlockingQueue<byte[]>(1000);

    private int TIMEOUT_USEC = 30000;

    private MediaCodec mediaCodec;

    int m_width;
    int m_height;
    int m_framerate;

    public boolean isRunning = false;

    public AvcEncoder(int width, int height, int framerate, int bitrate) {
        m_width = width;
        m_height = height;
        m_framerate = framerate;

        MediaFormat mediaFormat = MediaFormat.createVideoFormat("video/avc", m_width, m_height);
        mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);
        mediaFormat.setInteger(MediaFormat.KEY_BITRATE_MODE, MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR);
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, framerate);
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);

        try {
            // mediaCodec = MediaCodec.createByCodecName("OMX.google.h264.encoder");
            mediaCodec = MediaCodec.createByCodecName("OMX.qcom.video.encoder.avc");

//            MediaCodecInfo.CodecCapabilities capabilities = mediaCodec.getCodecInfo().getCapabilitiesForType("video/avc");
//            MediaCodecInfo.EncoderCapabilities encoderCapabilities = capabilities.getEncoderCapabilities();
//            if (encoderCapabilities.isBitrateModeSupported(MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR)) {
//                Log.e("Jonny", "Jonny: CBR is supported");
//            } else {
//                Log.e("Jonny", "Jonny: CBR is NOT supported");
//            }

        } catch (Exception e) {
            e.printStackTrace();
        }
        mediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        mediaCodec.start();
    }

    public void startEncoderThread() {
        isRunning = true;

        Thread encoderThread = new Thread(new Runnable() {
            long numQueuedFrames = 0;
            long generateIndex = 0;
            long pts = 0;
            byte[] inputData = null;

            @Override
            public void run() {
                while (isRunning) {
//                    Log.e(TAG, "AvcEncoder inputData " + generateIndex);

                    try {
                        if (generateIndex == 0) inputData = inputQueue.peek();
                        else inputData = inputQueue.poll(33333L, TimeUnit.MICROSECONDS);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }

                    if (inputData == null) continue;
                    numQueuedFrames++;

                    int inputBufferIndex = mediaCodec.dequeueInputBuffer(-1);
                    Log.e(TAG, "inputBufferIndex " + inputBufferIndex + " " + inputData.length);
                    if (inputBufferIndex >= 0) {
                        ByteBuffer inputBuffer = mediaCodec.getInputBuffer(inputBufferIndex);
                        inputBuffer.put(inputData);
                        pts = computePresentationTime(generateIndex);
                        mediaCodec.queueInputBuffer(inputBufferIndex, 0, inputData.length, pts, 0);

                        generateIndex++;
                    }

                    MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
                    int outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, TIMEOUT_USEC);
                    Log.e(TAG, "outputBufferIndex " + outputBufferIndex + " " + numQueuedFrames);
                    while (outputBufferIndex >= 0) {
                        ByteBuffer outputBuffer = mediaCodec.getOutputBuffer(outputBufferIndex);
                        byte[] outputData = new byte[bufferInfo.size];
                        outputBuffer.get(outputData);
                        if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_CODEC_CONFIG) {
                            Log.e(TAG, "Jiayi: ouputBufferInfo.flags CODEC_CONFIG " + numQueuedFrames);
                        } else if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_KEY_FRAME) {
                            numQueuedFrames--;
                            Log.e(TAG, "Jiayi: ouputBufferInfo.flags KEY_FRAME " + numQueuedFrames);
                        } else {
                            numQueuedFrames--;
                            Log.e(TAG, "Jiayi: ouputBufferInfo.flags " + bufferInfo.flags + " " + numQueuedFrames);
                        }
                        outputQueue.add(outputData);
                        mediaCodec.releaseOutputBuffer(outputBufferIndex, false);

                        if (numQueuedFrames != 0) outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, TIMEOUT_USEC);
                        else outputBufferIndex = -1;
                    }

                }
            }
        });
        encoderThread.start();
    }

    public void stopEncoderThread() {
        isRunning = false;
    }

    public void stopMediaCodec() {
        mediaCodec.stop();
        mediaCodec.release();
    }

    private long computePresentationTime(long frameIndex) {
        return 132 + frameIndex * 1000000 / m_framerate;
    }
}