package com.example.accumo;

import android.util.Log;

import org.apache.commons.lang3.tuple.Pair;
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.imgcodecs.Imgcodecs;

import java.io.File;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class Depth {
    private static final String TAG = "Depth";
    private static final float SAVING_FACTOR = 256.0f;
    private static final float SAVING_FACTOR_JPG = 2.55f;

    private static final BlockingQueue<Pair<Mat, String>> QUEUE = new LinkedBlockingQueue<>();

    public static Mat loadDepth(String fileName) {
        Mat m = Imgcodecs.imread(fileName, Imgcodecs.IMREAD_ANYDEPTH);
        if (m.channels() != 1) {
            Log.e(TAG, "Channel number is not 1");
            return null;
        }

        // Convert to float
        m.convertTo(m, CvType.CV_32F);

        // Apply savingFactor
        m.convertTo(m, CvType.CV_32F, 1 / SAVING_FACTOR);
        return m;
    }

    public static Mat loadDepthJpg(String fileName) {
        Mat m = Imgcodecs.imread(fileName, Imgcodecs.IMREAD_COLOR);
        if (m.channels() != 3) {
            Log.e(TAG, "Channel number is not 3");
            return null;
        }
        m.convertTo(m, CvType.CV_32F);
        m.convertTo(m, CvType.CV_32F, 1 / SAVING_FACTOR_JPG);

        // Pick any one of the 3 channels as the depth map
        Mat c = new Mat();
        Core.extractChannel(m, c, 0);
        return c;
    }

    public static void saveDepth(Mat m, String fileName) {
        File dir = new File(fileName).getParentFile();
        if (dir != null && !dir.exists())
            dir.mkdirs();
        if (m.channels() != 1) {
            Log.e(TAG, "Channel number is not 1");
            return;
        }
        if (!fileName.endsWith(".png")) {
            Log.e(TAG, "Output file name is not PNG");
            return;
        }

        // Convert to float32 first, to avoid overflow when applying saving factor
        m.convertTo(m, CvType.CV_32F);

        // Apply savingFactor, and convert back to uint16
        m.convertTo(m, CvType.CV_16U, SAVING_FACTOR);

        Imgcodecs.imwrite(fileName, m);
    }

    static void initDepthSaver() {
        Thread t = new Thread(() -> {
            try {
                while (true) {
                    Pair<Mat, String> p = QUEUE.take();
                    saveDepth(p.getLeft(), p.getRight());
                }
            } catch (InterruptedException e) {
                e.printStackTrace();
                Thread.currentThread().interrupt();
            }
        });
        t.setPriority(2);
        t.start();
    }

    static void saveDepthAsync(float[] depth, int height, int width, String fileName) {
        Mat mat = new Mat(height, width, CvType.CV_32F);
        mat.put(0, 0, depth);
        saveDepthAsync(mat, fileName);
    }

    static void saveDepthAsync(Mat m, String fileName) {
        try {
            QUEUE.put(Pair.of(m, fileName));
        } catch (InterruptedException e) {
            e.printStackTrace();
            Thread.currentThread().interrupt();
        }
    }
}