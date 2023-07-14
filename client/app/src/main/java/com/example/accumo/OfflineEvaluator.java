package com.example.accumo;

import android.util.Log;

import com.example.accumo.ml.Flownet2Fullres;

import org.ejml.simple.SimpleMatrix;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.imgproc.Imgproc;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class OfflineEvaluator {

    private static final String TAG = "OfflineEvaluator";

    public OfflineEvaluator(MainActivity ma, String video, int imgH, int imgW, int stride) {
        this.ma = ma;
        this.video = video;
        this.imgH = imgH;
        this.imgW = imgW;
        this.stride = stride;
        this.resultsRoot = new File("/sdcard/multitask/OfflineEvaluator/" + video,
                "stride_" + stride);
        this.resultsRoot.mkdirs();
        this.dAcc = new DepthAccModel(ma);
        Depth.initDepthSaver();
    }

    public void run() {
        File videoPath = new File(datasetRoot, video);
        File adabinsPredPath = new File(adabinsPredRoot, video);
        File gtPath = new File(gtRoot, video);

        // Read RGB
        File[] framesRgb = videoPath.listFiles(((file, s) -> s.endsWith(".yuv")));
        assert framesRgb != null;
        Arrays.sort(framesRgb);

        // Read depth
        File[] framesDepth = adabinsPredPath.listFiles(((file, s) -> s.endsWith(".png")));
        assert framesDepth != null;
        assert framesDepth.length == framesRgb.length;
        Arrays.sort(framesDepth);
        File[] framesDepthGt = gtPath.listFiles(((file, s) -> s.endsWith(".png")));
        assert framesDepthGt != null;
        Log.i(TAG, gtPath.toString());
        Log.i(TAG, String.valueOf(framesDepthGt.length) + "," + framesRgb.length);
        assert framesDepthGt.length == framesRgb.length;
        Arrays.sort(framesDepthGt);

        // Read poses
        List<double[]> poses = Util.loadKittiPoses(new File(videoPath, "poses_imu.txt"));
        assert poses.size() == framesRgb.length;

        // Save dummy depth for the first few frames, to make total depth map number consistent
        int frameIdx = 0;
        for (; frameIdx < stride; frameIdx++) {
//            Mat d = new Mat(imgH, imgW, CvType.CV_32F);
//            File depthSaveFile = new File(resultsRoot, String.format("%06d.png", frameIdx));
//            Depth.saveDepthAsync(d, depthSaveFile.getPath());
        }

        for (; frameIdx < framesRgb.length; frameIdx++) {
            Mat rgb1 = readYUVIntoRGBMat(framesRgb[frameIdx - stride]);
            Mat rgb2 = readYUVIntoRGBMat(framesRgb[frameIdx]);
            Mat depthPred1 = Depth.loadDepth(framesDepth[frameIdx - stride].toString());
            Mat depthGt2 = Depth.loadDepth(framesDepthGt[frameIdx].toString());
            Imgproc.resize(depthPred1, depthPred1, depthGt2.size());
            float[] poseChange = getRelativePoseChange(
                    poses.get(frameIdx - stride), poses.get(frameIdx));
            runForOneFrame(frameIdx, rgb1, rgb2, depthPred1, depthGt2, poseChange);
        }

        try {
            FileWriter writer = new FileWriter(new File(this.resultsRoot, "dAccPred.txt.tmp"));
            writer.write("abs_rel,a1,a2,a3" + System.lineSeparator());
            for (float[] errors : dAccPredArr) {
                for (int i = 0; i < errors.length; i++) {
                    writer.write(String.valueOf(errors[i]));
                    if (i < errors.length - 1)
                        writer.write(",");
                    else
                        writer.write(System.lineSeparator());
                }
            }
            writer.close();
        } catch (java.io.IOException e) {
            e.printStackTrace();
        }

        try {
            FileWriter writer = new FileWriter(new File(this.resultsRoot, "errors.txt"));
            writer.write("abs_rel,a1,a2,a3" + System.lineSeparator());
            for (float[] errors : errorArr) {
                for (int i = 0; i < errors.length; i++) {
                    writer.write(String.valueOf(errors[i]));
                    if (i < errors.length - 1)
                        writer.write(",");
                    else
                        writer.write(System.lineSeparator());
                }
            }
            writer.close();
        } catch (java.io.IOException e) {
            e.printStackTrace();
        }

        // Make saving of dAccPred.txt atomic, because it signifies the end of the run
        File src = new File(this.resultsRoot, "dAccPred.txt.tmp");
        File dst = new File(this.resultsRoot, "dAccPred.txt");
        src.renameTo(dst);
    }

    private Mat readYUVIntoRGBMat(File f) {
        byte[] yuv = null;
        try {
            yuv = Files.readAllBytes(f.toPath());
        } catch (IOException e) {
            e.printStackTrace();
        }
        return Util.convertYUVArraysToMat(imgH, imgW, yuv);
    }

    private float[] getRelativePoseChange(double[] pose1, double[] pose2) {
        SimpleMatrix p1 = new SimpleMatrix(4, 4, true, pose1);
        SimpleMatrix p2 = new SimpleMatrix(4, 4, true, pose2);
        SimpleMatrix poseChange = p2.invert().mult(p1);
        float[] poseChangeArr = new float[16];
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                poseChangeArr[j * 4 + k] = (float)poseChange.get(j, k);
            }
        }
        return poseChangeArr;
    }

    private void runForOneFrame(int frameIdx, Mat rgb1, Mat rgb2, Mat depthPred1, Mat depthGt2,
                                float[] poseChange) {
        Log.i(TAG, "Processing frame: " + frameIdx);
        float[] depth1Buff = new float[(int)depthPred1.total()];
        depthPred1.get(0, 0, depth1Buff);
        List<int[]> rgb1Arr = Util.convertMatToArrays(rgb1);
        float[] depth2Arr = ma.WarpForDepthWithRGB(depth1Buff, poseChange,
                rgb1Arr.get(0), rgb1Arr.get(1), rgb1Arr.get(2));

        Mat depth2 = new Mat(depthPred1.height(), depthPred1.width(), CvType.CV_32F);
        depth2.put(0, 0, depth2Arr);
        Mat rgb2Warped = Util.convertRGBArraysToMat(imgH, imgW, rgb1Arr.get(0), rgb1Arr.get(1),
                rgb1Arr.get(2));

//        dAcc.observe(frameIdx - stride, frameIdx, rgb2Warped, depth2Arr, rgb2);

//        dAccPredArr.add(dAcc.decide().clone());
        dAccPredArr.add(new float[]{0, 0, 0, 0});

        float[] errors = DepthAccModel.computeErrors(depthGt2, depth2);
        Log.i(TAG, "errors: " + errors[0]);
        errorArr.add(errors);

//        // Save warped depth
//        File depthSaveFile = new File(resultsRoot, String.format("%06d.png", frameIdx));
//        Depth.saveDepthAsync(depth2, depthSaveFile.getPath());
//
//        // Save flowed depth
//        depthSaveFile = new File(resultsRoot, String.format("%06d-flowed.png", frameIdx));
//        Depth.saveDepthAsync(dAcc.getLatestFlowedDepth(), depthSaveFile.getPath());
//
//        // Save warped RGB
//        Imgcodecs.imwrite(new File(resultsRoot, String.format("%06d-warped.jpg", frameIdx)).toString(),
//                rgb2Warped);
//
//        // Save optical flow
//        OptFlow.flowWrite(new File(resultsRoot, String.format("%06d-flow.png", frameIdx)).toString(),
//                dAcc.getLatestOptFlow());
    }

    private MainActivity ma; // To have access to the JNI functions
    private final File datasetRoot = new File("/sdcard/multitask/dataset");
    private final File adabinsPredRoot = new File("/sdcard/multitask/adabinsPred");
    private final File resultsRoot;
    private final File gtRoot = new File("/sdcard/multitask/gt");
    private final String video;
    private final int imgH;
    private final int imgW;
    private int stride;
    private DepthAccModel dAcc;
    private ArrayList<float[]> dAccPredArr = new ArrayList<>();
    private ArrayList<float[]> errorArr = new ArrayList<>();
}
