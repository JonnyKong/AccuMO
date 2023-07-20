package com.example.accumo;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.example.accumo.databinding.ActivityMainBinding;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.PrintWriter;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.imgcodecs.Imgcodecs;
import org.opencv.imgproc.Imgproc;

import com.example.accumo.ml.Flownet2;
import org.ejml.simple.SimpleMatrix;
import org.tensorflow.lite.DataType;
import org.tensorflow.lite.support.model.Model;
import org.tensorflow.lite.support.tensorbuffer.TensorBuffer;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    private static final String IP_MESSAGE = "com.example.accumo.IP";
    private static final String VIDEO_MESSAGE = "com.example.accumo.VIDEO";
    private static final String SCHED_MESSAGE = "com.example.accumo.SCHED";
    private static final String MODE_MESSAGE = "com.example.accumo.MODE";
    private static final String PATTERN_MESSAGE = "com.example.accumo.PATTERN";
    private static final String ENABLE_FASTDEPTH_MESSAGE = "com.example.accumo.ENABLE_FASTDEPTH";
    private static final String MPC_WEIGHT_MESSAGE = "com.example.accumo.MPC_WEIGHT";
    private static final String OFFLINE_STRIDE_MESSAGE = "com.example.accumo.OFFLINE_STRIDE";

    private Executor executor;
    private static Mat meshGridX; // Used by `applyOptflow()` function
    private static Mat meshGridY; // Used by `applyOptflow()` function

    // String host = "192.168.1.146";
    // String host = "xr.ecn.purdue.edu";
    String host;
    String port = "9999";
    private static final int IMG_H = 256;
    private static final int IMG_W = 832;
    private static final int OFFLOAD_H = 256;
    private static final int OFFLOAD_W = 832;
    TaskManager tm;
    DiskFrameReader dfr;

    // Used to load the 'accumo' library on application startup.
    static {
        System.loadLibrary("accumo");
        if (OpenCVLoader.initDebug())
            Log.i(TAG, "opencv installed successfully");
        else
            Log.i(TAG, "opencv not installed");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());

        InitializeWarping();

        // Parse intents
        Intent intent = getIntent();
        host = intent.getStringExtra(IP_MESSAGE);
        if (host == null)
            host = "192.168.1.146";
        String video = intent.getStringExtra(VIDEO_MESSAGE);
        if (video == null)
            video = "1000";
        String sched = intent.getStringExtra(SCHED_MESSAGE);
        if (sched == null)
            sched = "rr";
        int[] schedPattern = intent.getIntArrayExtra(PATTERN_MESSAGE);
        if (schedPattern == null)
            schedPattern = new int[]{0, 1};
        String mode = intent.getStringExtra(MODE_MESSAGE);
        boolean enableFastdepth = intent.getBooleanExtra(ENABLE_FASTDEPTH_MESSAGE, false);
        float mpcWeight = intent.getFloatExtra(MPC_WEIGHT_MESSAGE, (float)2.5);
        int stride = intent.getIntExtra(OFFLINE_STRIDE_MESSAGE, 6);

        executor = Executors.newSingleThreadExecutor();
        if (mode.equals("online")) {
            tm = new TaskManager(this, host, port, video, sched, schedPattern,
                    enableFastdepth, IMG_H, IMG_W, OFFLOAD_H, OFFLOAD_W, false, mpcWeight);
            dfr = new DiskFrameReader(tm, video, IMG_H, IMG_W);
            executor.execute(dfr::run);
        } else if (mode.equals("offline")) {
            OfflineEvaluator e = new OfflineEvaluator(this, video, IMG_H, IMG_W, stride);
            executor.execute(e::run);
        }
    }

    /**
     * A native method that is implemented by the 'accumo' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native void InitializeWarping();
    public native float[] WarpForDepth(float[] depth, float[] poseChange);
    public native float[] WarpForDepthWithRGB(float[] depth, float[] poseChange,
                                              int[] b, int[] g, int[] r);
    public native void loadModel(String model);
    public native float[] inference(byte[] yuv);
    public native void interpNearest(int h, int w, float[] mat);

    protected void onPause() {
        super.onPause();
        dfr.shutdown();
    }
}