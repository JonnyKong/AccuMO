package com.example.accumo;

import com.example.accumo.ml.FastDepthSim;
import com.example.accumo.ml.Flownet2;

import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.imgcodecs.Imgcodecs;
import org.opencv.imgproc.Imgproc;
import org.tensorflow.lite.DataType;
import org.tensorflow.lite.support.model.Model;
import org.tensorflow.lite.support.tensorbuffer.TensorBuffer;

import java.io.IOException;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import lombok.Data;

public class DepthAccModel {
    @Data
    static class State {
        public int c1FrameIdx1 = -1;

        public int c2FrameIdx2 = -1;
    }

    public DepthAccModel(MainActivity ma) {
        this.ma = ma;
        Model.Options options = new Model.Options.Builder()
                .setDevice(Model.Device.GPU)
                .setNumThreads(4)
                .build();
        try {
            model = Flownet2.newInstance(ma, options);
            fastDepthModel = FastDepthSim.newInstance(ma);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public float[] decide() {
        return slope;
    }

    State getState() {
        return state;
    }

    void observe(int frameIdxPrev, int frameIdxCurr, Mat warpedRGB, float[] warpedDepthBuf,
                 Mat currentRGB) {
        int inputH = warpedRGB.height();
        int inputW = warpedRGB.width();

        // Resize RGB for inference
        Mat warpedRGBResized = new Mat();
        Mat currentRGBResized = new Mat();
        warpedRGB.convertTo(warpedRGBResized, CvType.CV_32F);
        currentRGB.convertTo(currentRGBResized, CvType.CV_32F);
        Imgproc.resize(warpedRGBResized, warpedRGBResized, new Size(inferenceW, inferenceH));
        Imgproc.resize(currentRGBResized, currentRGBResized, new Size(inferenceW, inferenceH));

        // Convert OpenCV Mat to tflite TensorBuffer
        FloatBuffer xBuf = FloatBuffer.allocate(3 * 2 * inferenceH * inferenceW);
        ArrayList<Mat> warpedRGBResizedSplitted = new ArrayList<>();
        ArrayList<Mat> currentRGBResizedSplitted = new ArrayList<>();
        Core.split(warpedRGBResized, warpedRGBResizedSplitted);
        Core.split(currentRGBResized, currentRGBResizedSplitted);
        for (int i = 0; i < 3; i++) {
            float[] buf = new float[inferenceH * inferenceW];
            currentRGBResizedSplitted.get(i).get(0, 0, buf);
            xBuf.put(buf);
            warpedRGBResizedSplitted.get(i).get(0, 0, buf);
            xBuf.put(buf);
        }
        TensorBuffer x = TensorBuffer.createFixedSize(new int[]{1, 3, 2, inferenceH, inferenceW},
                DataType.FLOAT32);
        x.loadArray(xBuf.array());

        // Inference
        Flownet2.Outputs outputs = model.process(x);
        TensorBuffer outputFeature0 = outputs.getOutputFeature0AsTensorBuffer();

        // Convert optflow to original shape
        float[] xy = outputFeature0.getFloatArray();
        float[] optFlowX = new float[inferenceH * inferenceW];
        float[] optFlowY = new float[inferenceH * inferenceW];
        System.arraycopy(xy, 0, optFlowX, 0, inferenceH * inferenceW);
        System.arraycopy(xy, inferenceH * inferenceW, optFlowY, 0, inferenceH * inferenceW);
        Mat OptFlowXMat = new Mat(inferenceH, inferenceW, CvType.CV_32F);
        Mat OptFlowYMat = new Mat(inferenceH, inferenceW, CvType.CV_32F);
        OptFlowXMat.put(0, 0, optFlowX);
        OptFlowYMat.put(0, 0, optFlowY);
        Imgproc.resize(OptFlowXMat, OptFlowXMat, new Size(inputW, inputH));
        Imgproc.resize(OptFlowYMat, OptFlowYMat, new Size(inputW, inputH));
        Core.multiply(OptFlowXMat, new Scalar(832.0 / 448.0), OptFlowXMat);
        Core.multiply(OptFlowYMat, new Scalar(256.0 / 128.0), OptFlowYMat);
        optflow = new OptFlow(OptFlowXMat, OptFlowYMat);

        // Apply optflow
        Mat warpedDepthMat = new Mat(inputH, inputW, CvType.CV_32F);
        warpedDepthMat.put(0, 0, warpedDepthBuf);
        flowedDepthMat = applyOptflow(warpedDepthMat, optflow);

        float[] errors = computeErrors(flowedDepthMat, warpedDepthMat);
        slope[0] = errors[0] / (frameIdxCurr - frameIdxPrev);
        slope[1] = (1 - errors[1]) / (frameIdxCurr - frameIdxPrev);
        slope[2] = (1 - errors[2]) / (frameIdxCurr - frameIdxPrev);
        slope[3] = (1 - errors[3]) / (frameIdxCurr - frameIdxPrev);

        state.c1FrameIdx1 = frameIdxPrev;
        state.c2FrameIdx2 = frameIdxCurr;

        // Useful for debugging
        if (false) {
            executor.execute(() -> {
                Imgcodecs.imwrite("/sdcard/multitask/dAcc/"+ String.format("%06d-warped.jpg", frameIdxCurr), warpedRGB);
                Imgcodecs.imwrite("/sdcard/multitask/dAcc/"+ String.format("%06d-current.jpg", frameIdxCurr), currentRGB);
                OptFlow.flowWrite("/sdcard/multitask/dAcc/"+ String.format("%06d-flow.png", frameIdxCurr), optflow);
                Depth.saveDepthAsync(warpedDepthMat, "/sdcard/multitask/dAcc/"+ String.format("%06d-warped.png", frameIdxCurr));
                Depth.saveDepthAsync(flowedDepthMat, "/sdcard/multitask/dAcc/"+ String.format("%06d-flowed.png", frameIdxCurr));
            });
        }
    }

    public double observeGolden(int frameIdx, float[] lastOffloadedDepth, Mat lastOffloadedDepthRgb) {
        return getErrorsFromGoldenRule(frameIdx, lastOffloadedDepth,
                lastOffloadedDepthRgb);
    }

    public Mat getLatestFlowedDepth() {
        return flowedDepthMat;
    }

    public OptFlow getLatestOptFlow() {
        return optflow;
    }

    private float getErrorsFromGoldenRule(int frameIdx, float[] lastOffloadedDepth,
                                          Mat lastOffloadedDepthRgb) {
        final int inferenceWidth = 224;
        final int inferenceHeight = 64;
        double[] mean_vals = {0.485, 0.456, 0.406};
        double[] norm_vals = {1 / 0.229, 1 / 0.224, 1 / 0.225};

        TensorBuffer x = TensorBuffer.createFixedSize(
                new int[]{1, 3, inferenceHeight, inferenceWidth}, DataType.FLOAT32);
        FloatBuffer xBuf = FloatBuffer.allocate(3 * inferenceHeight * inferenceWidth);

        // 256x832 -> 64x224
        Mat lastOffloadedDepthRgbResized = new Mat(inferenceHeight, inferenceWidth, CvType.CV_8UC1);
        Imgproc.resize(lastOffloadedDepthRgb, lastOffloadedDepthRgbResized,
                new Size(inferenceWidth, inferenceHeight));

        // [0, 255] -> [0, 1]
        lastOffloadedDepthRgbResized.convertTo(lastOffloadedDepthRgbResized, CvType.CV_32FC3);
        lastOffloadedDepthRgbResized.convertTo(lastOffloadedDepthRgbResized, CvType.CV_32FC3, 1 / 256.0);

        // Normalization
        ArrayList<Mat> splitted = new ArrayList<>();
        Core.split(lastOffloadedDepthRgbResized, splitted);
        for (int i = 0; i < 3; i++) {
            splitted.get(i).convertTo(splitted.get(i), CvType.CV_32F, 1, -1 * mean_vals[i]);
            splitted.get(i).convertTo(splitted.get(i), CvType.CV_32F, norm_vals[i], 0);
            float[] buf = new float[inferenceHeight * inferenceWidth];
            splitted.get(i).get(0, 0, buf);
            xBuf.put(buf);
        }

        // Inference
        x.loadArray(xBuf.array());
        float[] lastOffloadedDepthFastdepth =
                fastDepthModel.process(x).getOutputFeature0AsTensorBuffer().getFloatArray();
        ma.interpNearest(inferenceHeight, inferenceWidth, lastOffloadedDepthFastdepth);

        // 64x224 -> 256x832
        Mat lastOffloadedDepthFastdepthMat = new Mat(inferenceHeight, inferenceWidth, CvType.CV_32F);
        lastOffloadedDepthFastdepthMat.put(0, 0, lastOffloadedDepthFastdepth);
        Imgproc.resize(lastOffloadedDepthFastdepthMat, lastOffloadedDepthFastdepthMat,
                new Size(832, 256));

        // Errors
        Mat lastOffloadedDepthMat = new Mat(256, 832, CvType.CV_32F);
        lastOffloadedDepthMat.put(0, 0, lastOffloadedDepth);
        if (false) {
            executor.execute(() -> {
                Depth.saveDepthAsync(lastOffloadedDepthMat, "/sdcard/multitask/dAcc/"+ String.format("%06d-adabins.png", frameIdx));
                Depth.saveDepthAsync(lastOffloadedDepthFastdepthMat, "/sdcard/multitask/dAcc/"+ String.format("%06d-fastdepth.png", frameIdx));
            });
        }
        return computeErrors(lastOffloadedDepthMat, lastOffloadedDepthFastdepthMat)[0];
    }

    /**
     * Apply optical flow to depth map, return the new depth.
     */
    private Mat applyOptflow(Mat depth, OptFlow flow) {
        // Re-init meshgrid if needed
        if (meshGridX == null || meshGridX.height() != depth.height() ||
                meshGridX.width() != depth.width()) {
            meshGridX = new Mat();
            meshGridY = new Mat();

            Mat x = new Mat(1, depth.width(), CvType.CV_32F);
            for (int i = 0; i < depth.width(); i++) {
                float[] val = {(float)i};
                x.put(0, i, val);
            }
            Core.repeat(x, depth.height(), 1, meshGridX);

            Mat y = new Mat(depth.height(), 1, CvType.CV_32F);
            for (int i = 0; i < depth.height(); i++) {
                float[] val = {(float)i};
                y.put(i, 0, val);
            }
            Core.repeat(y, 1, depth.width(), meshGridY);
        }

        Mat x = new Mat();
        Mat y = new Mat();
        Core.add(flow.x, meshGridX, x);
        Core.add(flow.y, meshGridY, y);

        Mat flowedDepth = new Mat();
        Imgproc.remap(depth, flowedDepth, x, y, Imgproc.INTER_NEAREST, Core.BORDER_REPLICATE);

        return flowedDepth;
    }

    static public float[] computeErrors(Mat gt, Mat pred) {
        int maxDepth = 80;
        Mat valid = new Mat();  // Valid is 255.0, invalid is 0
        Mat valid_ = new Mat();

        // Get mask of > 0
        Core.compare(gt, new Scalar(0), valid, Core.CMP_GT);
        Core.compare(pred, new Scalar(0), valid_, Core.CMP_GT);
        Core.bitwise_and(valid, valid_, valid);

        // Get mask of < 80
        Core.compare(gt, new Scalar(maxDepth), valid_, Core.CMP_LT);
        Core.bitwise_and(valid, valid_, valid);
        Core.compare(pred, new Scalar(maxDepth), valid_, Core.CMP_LT);
        Core.bitwise_and(valid, valid_, valid);

        // Compute abs_rel
        Mat abs_rel = new Mat();
        Core.absdiff(pred, gt, abs_rel);
        Core.divide(abs_rel, gt, abs_rel);

        // Apply mask. Scale is 255.0 because in mask, true is 255.0, false is 0.0
        Core.multiply(abs_rel, valid, abs_rel, 1 / 255.0, CvType.CV_32F);
        Core.patchNaNs(abs_rel, 0.0);
        float mean_abs_rel = (float)Core.mean(abs_rel).val[0];
        mean_abs_rel = mean_abs_rel * gt.height() * gt.width() / Core.countNonZero(valid);

        // Compute delta < 1.25
        Mat x = new Mat();
        Mat y = new Mat();
        Mat absRatio = new Mat();
        Core.divide(gt, pred, x);
        Core.divide(pred, gt, y);
        Core.max(x, y, absRatio);
        Mat a1 = new Mat();
        Mat a2 = new Mat();
        Mat a3 = new Mat();
        Core.compare(absRatio, new Scalar(1.25), a1, Core.CMP_LT);
        Core.compare(absRatio, new Scalar(1.25 * 1.25), a2, Core.CMP_LT);
        Core.compare(absRatio, new Scalar(1.25 * 1.25 * 1.25), a3, Core.CMP_LT);
        a1.convertTo(a1, CvType.CV_32F);
        a2.convertTo(a2, CvType.CV_32F);
        a3.convertTo(a3, CvType.CV_32F);
        Core.multiply(a1, valid, a1, 1 / 255.0, CvType.CV_32F);
        Core.multiply(a2, valid, a2, 1 / 255.0, CvType.CV_32F);
        Core.multiply(a3, valid, a3, 1 / 255.0, CvType.CV_32F);
        float mean_a1 = (float)Core.countNonZero(a1) / (float)Core.countNonZero(valid);
        float mean_a2 = (float)Core.countNonZero(a2) / (float)Core.countNonZero(valid);
        float mean_a3 = (float)Core.countNonZero(a3) / (float)Core.countNonZero(valid);

        return new float[]{mean_abs_rel, mean_a1, mean_a2, mean_a3};
    }

    public boolean isBusy = false;

    protected float[] slope = {1, 1, 1, 1};

    protected final State state = new State();

    private Executor executor = Executors.newSingleThreadExecutor();
    private MainActivity ma; // To have access to the JNI functions
    private int inferenceH = 128;
    private int inferenceW = 448;
    private static Mat meshGridX; // Used by `applyOptflow()` function
    private static Mat meshGridY; // Used by `applyOptflow()` function
    public Mat flowedDepthMat;
    private OptFlow optflow;
    Flownet2 model;
    FastDepthSim fastDepthModel;
}
