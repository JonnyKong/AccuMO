package com.example.accumo;

import android.util.Log;

public class VoAccModel {
    private final Filter filter;
    private final float mpcWeight;

    VoAccModel(Filter filter, float mpcWeight) {
        this.filter = filter;
        this.mpcWeight = mpcWeight;
        Log.i("TaskManager", "initialized VoAccModel with weight " + mpcWeight);
    }

    double decide() {
        double angle = filter.angle();
        double rate = 0.10127043039054989 / 4;
        return mpcWeight * rate * (2 * angle);
    }
}
