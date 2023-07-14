package com.example.accumo;

import java.io.File;
import java.util.List;

public class VoAccModelOracle {
    final File TABLE_ROOT = new File("/sdcard/multitask/voAccTable");

    List<double[]> accTable;

    private final float mpcWeight;

    VoAccModelOracle(String videoName, float mpcWeight) {
        File tablePath = new File(TABLE_ROOT, String.format("%s.txt", videoName));
        accTable = Util.loadArrays(tablePath, 2);
        this.mpcWeight = mpcWeight;
    }

    double decide(int frameIdx) {
        // Table lookup
        // return 8 * mpcWeight * accTable.get(Math.min(frameIdx / 8, accTable.size() - 1))[1]; // Maybe x2 here
        double angle = accTable.get(Math.min(frameIdx / 8, accTable.size() - 1))[0];
        double rate = 0.10127043039054989 / 4;
        return 3.5 * mpcWeight * rate * (2 * angle);
    }
}
