package com.example.accumo;

import android.util.Log;

import com.opencsv.CSVReaderHeaderAware;
import com.opencsv.exceptions.CsvException;

import org.opencv.core.Mat;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.TreeMap;

import lombok.SneakyThrows;

public class DepthAccModelOracle extends DepthAccModel {
    private final int[] STRIDE_ARR =
            new int[]{0, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};

    File tableRoot = new File("/sdcard/multitask/OfflineEvaluator");

    TreeMap<Integer, double[]> absrelTableMap = new TreeMap<>();

    @SneakyThrows({FileNotFoundException.class, IOException.class, CsvException.class})
    DepthAccModelOracle(MainActivity ma, String videoName) {
        super(ma);

        // Parse warped ac
        for (int stride : STRIDE_ARR) {
            File tablePath = new File(
                    String.format("%s/%s/stride_%d/errors.txt", tableRoot, videoName, stride));
            double[] absRelArr = new CSVReaderHeaderAware(new FileReader(tablePath))
                        .readAll()
                        .stream()
                        .mapToDouble(s -> Double.parseDouble(s[0]))
                        .toArray();
            Log.i("DepthAccModelOracle", "stride and len: " + stride + ", " + absRelArr.length);
            absrelTableMap.put(stride, absRelArr);
        }
    }

    @Override
    void observe(int frameIdxPrev, int frameIdxCurr, Mat warpedRGB, float[] warpedDepthBuf,
                 Mat currentRGB) {
        int stride = frameIdxCurr - frameIdxPrev;
        Log.i("DepthAccModelOracle", "prev, curr: " + frameIdxPrev + ", " + frameIdxCurr);

        // Network interruption may cause stride that is out of bound
        stride = Math.min(stride, absrelTableMap.lastKey());

        double absrelWarped = absrelTableMap.get(stride)[frameIdxPrev];
        double absrelOffline = absrelTableMap.get(0)[frameIdxPrev];
        state.c1FrameIdx1 = frameIdxPrev;
        state.c2FrameIdx2 = frameIdxCurr;

        slope[0] = (float) ((absrelWarped - absrelOffline)/ stride);
    }
}
