package com.example.accumo;

import android.util.Log;

import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Scalar;
import org.opencv.imgcodecs.Imgcodecs;

import java.io.File;
import java.util.Arrays;

public class OptFlow {
    private static final String TAG = "OptFlow";

    public Mat x;
    public Mat y;

    public OptFlow(Mat x, Mat y) {
        this.x = x;
        this.y = y;
    }

    public static OptFlow flowRead(String fileName) {
        Mat m = Imgcodecs.imread(fileName, Imgcodecs.IMREAD_ANYDEPTH);
        if (m.height() % 2 != 0) {
            Log.e(TAG, "Height is not divisible by 2");
            return null;
        }

        // Convert to float
        m.convertTo(m, CvType.CV_32F);

        OptFlow f = new OptFlow(new Mat(m.height(), m.width(), CvType.CV_32F),
                new Mat(m.height(), m.width(), CvType.CV_32F) );
        Core.add(m.submat(0, m.height() / 2, 0, m.width()),
                 new Scalar(-1 * (1 << 15)), f.x);
        Core.add(m.submat(m.height() / 2, m.height(), 0, m.width()),
                 new Scalar(-1 * (1 << 15)), f.y);
        return f;
    }

    public static void flowWrite(String fileName, OptFlow flow) {
        if (!fileName.endsWith(".png")) {
            Log.e(TAG, "Output file is not PNG format");
            return;
        }
        int imgH = flow.x.height();
        int imgW = flow.x.width();

        // Shift by 2^15 to store negative values using uint16
        Mat x = new Mat();
        Mat y = new Mat();
        Core.add(flow.x, new Scalar((1 << 15)), x);
        Core.add(flow.y, new Scalar((1 << 15)), y);

        // Combine two Mat into one
        float[] bufX = new float[imgH * imgW];
        float[] bufY = new float[imgH * imgW];
        x.get(0, 0, bufX);
        y.get(0, 0, bufY);
        float[] buf = new float[2 * imgH * imgW];
        System.arraycopy(bufX, 0, buf, 0, imgH * imgW);
        System.arraycopy(bufY, 0, buf, imgH * imgW, imgH * imgW);
        Mat m = new Mat(2 * imgH, imgW, CvType.CV_32F);
        m.put(0, 0, buf);

        m.convertTo(m, CvType.CV_16U);
        Imgcodecs.imwrite(fileName, m);
    }

    public static void test() {
        File[] frames = new File("/sdcard/multitask/flow/1000").listFiles(((file, s) -> s.endsWith(".png")));
        assert frames != null;
        Arrays.sort(frames);

        new File("/sdcard/multitask/flow/1000-output").mkdirs();
        for (int i = 0; i < frames.length; i++) {
            Log.i(TAG, "testing frame: " + frames[i].getName());

            OptFlow f = OptFlow.flowRead(frames[i].getPath());
            OptFlow.flowWrite("/sdcard/multitask/flow/1000-output/" + frames[i].getName(), f);
        }
    }

}