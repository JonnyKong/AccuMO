package com.example.accumo;

public class Filter {
    private final long nativeFilter;

    Filter() {
        nativeFilter = createFilter();
    }

    void cleanup() {
        deleteFilter(nativeFilter);
    }

    void predict(double[] accel, double[] gyro) {
        predict(nativeFilter, accel, gyro);
    }

    void startOffload() {
        startOffload(nativeFilter);
    }

    void fusemvo(double[] vo) {
        fusemvo(nativeFilter, vo);
    }

    double[] pose() {
        return pose(nativeFilter);
    }

    double[] poseRelative(int f1, int f2) {
        return poseRelative(nativeFilter, f1, f2);
    }

    double angle() {
        return angle(nativeFilter);
    }

    private native long createFilter();

    private native void deleteFilter(long handle);

    private native void predict(long handle, double[] accel, double[] gyro);

    private native void startOffload(long handle);

    private native void fusemvo(long handle, double[] vo);

    private native double[] pose(long handle);

    private native double[] poseRelative(long handle, int f1, int f2);

    private native double angle(long handle);
}
