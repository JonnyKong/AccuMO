package com.example.accumo.sched;

public abstract class Scheduler {
    public static class Task {
        final int latency;

        final int strideLimit;

        double accDropLimit;

        int latestOffload = -1;

        int pendingOffload = -1;

        Task(int latency, int strideLimit, double accDropLimit) {
            this.latency = latency;
            this.strideLimit = strideLimit;
            this.accDropLimit = accDropLimit;
        }

        Task(Task other) {
            latency = other.latency;
            strideLimit = other.strideLimit;
            accDropLimit = other.accDropLimit;
            latestOffload = other.latestOffload;
            pendingOffload = other.pendingOffload;
        }

        void offload(int frame) {
            pendingOffload = frame;
        }

        void finishOffload() {
            latestOffload = pendingOffload;
            pendingOffload = -1;
        }

        public double getAccDropLimit() {
            return accDropLimit;
        }

        public void setAccDropLimit(double other) {
            accDropLimit = other;
        }
    }

//     2080 Ti + 5G depth from disk
//    protected Task[] tasks = {new Task(5, 25 + 3, Double.MAX_VALUE),
//            new Task(5, 25 + 3, Double.MAX_VALUE)};
    // 2080 Ti + 5G long latency
//    protected Task[] tasks = {new Task(6, 27 + 3, Double.MAX_VALUE),
//            new Task(5, 28 + 3, Double.MAX_VALUE)};
//    // 2080 Ti + 5G long latency 2:1
//    protected Task[] tasks = {new Task(6, 27 + 3, Double.MAX_VALUE),
//            new Task(5, 22 + 3, Double.MAX_VALUE)};
    // 2080 Ti + 802.11ac
    protected Task[] tasks = {new Task(5, 22 + 3, Double.MAX_VALUE),
            new Task(4, 23 + 3, Double.MAX_VALUE)};
    // a40 + 802.11ac
//    protected Task[] tasks = {new Task(4, 20 + 2, Double.MAX_VALUE),
//            new Task(4, 20 + 2, Double.MAX_VALUE)};


    public abstract int schedule(int frame, double... accDropRates);

    public void offload(int task, int frame) {
        tasks[task].offload(frame);
    }

    public void finishOffload(int task) {
        tasks[task].finishOffload();
    }

    public Task getTask(int task) {
        return tasks[task];
    }

    protected Task[] copy() {
        Task[] ret = new Task[tasks.length];
        for (int i = 0; i < tasks.length; i++) {
            ret[i] = new Task(tasks[i]);
        }
        return ret;
    }
}
