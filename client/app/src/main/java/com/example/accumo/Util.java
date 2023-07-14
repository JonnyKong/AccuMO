package com.example.accumo;

import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.imgproc.Imgproc;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class Util {
    static List<Double> loadArray(File fileName) {
        List<Double> ret = new ArrayList<>();
        try (Scanner sc = new Scanner(fileName)) {
            while (sc.hasNextDouble()) {
                ret.add(sc.nextDouble());
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        return ret;
    }

    static List<double[]> loadArrays(File fileName, int length, double[] suffix) {
        List<double[]> poses = new ArrayList<>();
        Scanner sc;
        try {
            sc = new Scanner(new BufferedReader(new FileReader(fileName)));
        } catch(FileNotFoundException e) {
            e.printStackTrace();
            return null;
        }

        while (sc.hasNextLine()) {
            String[] line = sc.nextLine().trim().split(" ");
            assert(line.length == length);

            double[] pose = new double[length + suffix.length];
            for (int i = 0; i < length; i++) {
                pose[i] = Double.parseDouble(line[i]);
            }
            System.arraycopy(suffix, 0, pose, length, suffix.length);
            poses.add(pose);
        }

        return poses;
    }

    static List<double[]> loadArrays(File fileName, int length) {
        return loadArrays(fileName, length, new double[0]);
    }

    static List<double[]> loadKittiPoses(File fileName) {
        return loadArrays(fileName, 12, new double[]{0, 0, 0, 1});
    }

    static Mat convertYUVArraysToMat(int imgH, int imgW, byte[] yuv) {
        Mat yuvMat = new Mat(imgH * 3 / 2, imgW, CvType.CV_8UC1);
        yuvMat.put(0, 0, yuv);

        Mat rgbMat = new Mat();
        Imgproc.cvtColor(yuvMat, rgbMat, Imgproc.COLOR_YUV2RGB_NV12);
        return rgbMat;
    }

    static Mat convertRGBArraysToMat(int imgH, int imgW, int[] r, int[] g, int[] b) {
        ArrayList<Mat> rgbSplitted = new ArrayList<>();
        for (int i = 0; i < 3; i++)
            rgbSplitted.add(new Mat(imgH, imgW, CvType.CV_32S));
        rgbSplitted.get(0).put(0, 0, r);
        rgbSplitted.get(1).put(0, 0, g);
        rgbSplitted.get(2).put(0, 0, b);
        Mat rgb = new Mat();
        Core.merge(rgbSplitted, rgb);
        return rgb;
    }

    /**
     * Convert OpenCV Mat to three int[], each for one of the RGB channels.
     */
    static List<int[]> convertMatToArrays(Mat rgbMat) {
        ArrayList<Mat> rgbSplitted = new ArrayList<>();
        Core.split(rgbMat, rgbSplitted);
        ArrayList<int[]> rgbBuffArr = new ArrayList<>();
        for (int cIdx = 0; cIdx < 3; cIdx++) {
            Mat c = rgbSplitted.get(cIdx);
            c.convertTo(c, CvType.CV_32S);
            int[] cBuff = new int[(int)rgbMat.total()];
            c.get(0, 0, cBuff);
            rgbBuffArr.add(cBuff);
        }
        return rgbBuffArr;
    }
}
