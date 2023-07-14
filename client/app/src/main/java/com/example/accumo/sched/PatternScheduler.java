package com.example.accumo.sched;

public class PatternScheduler extends Scheduler {
    private final int[] pattern;

    private int curr = -1;

    public PatternScheduler(int[] pattern) {
        this.pattern = pattern;
    }

    @Override
    public int schedule(int frame, double... accDropRates) {
        curr = (curr + 1) % pattern.length;
        return pattern[curr];
    }
}
