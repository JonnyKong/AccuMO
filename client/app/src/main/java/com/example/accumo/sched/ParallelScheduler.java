package com.example.accumo.sched;

public class ParallelScheduler extends Scheduler {
    @Override
    public int schedule(int frame, double... accDropRates) {
        return 2;
    }

    @Override
    public void offload(int task, int frame) {}

    @Override
    public void finishOffload(int task) {}
}