package com.example.accumo.sched;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class MpcScheduler extends Scheduler {
    private final int lookahead = 30;

    private final double discount = 1;
//    private final double discount = 0.99;

    private final List<List<Integer>> trails;

    public MpcScheduler() {
        trails = generateTrails(Collections.emptyList());
    }

    private List<List<Integer>> generateTrails(List<Integer> prefix) {
        List<List<Integer>> ret = new ArrayList<>();
        for (int i = 0; i < tasks.length; i++) {
            List<Integer> t = new ArrayList<>(prefix);
            t.add(i);
            t.addAll(Collections.nCopies(tasks[i].latency - 1, -1));
            if (t.size() < lookahead) {
                ret.addAll(generateTrails(t));
            } else {
                ret.add(t.subList(0, lookahead));
            }
        }
        return ret;
    }

    @Override
    public int schedule(int frame, double... accDropRates) {
        int minTask = -1;
        double minDrop = Double.MAX_VALUE;
        for (List<Integer> tr : trails) {
            Task[] tc = copy();
            double drop = 0;
            double d = 1;
            for (int i = 0; i < tr.size(); i++) {
                int f = frame + i;
                for (int j = 0; j < tc.length; j++) {
                    if (tc[j].pendingOffload >= 0 && f - tc[j].pendingOffload >= tc[j].latency) {
                        tc[j].finishOffload();
                    }
                    if (f - tc[j].latestOffload > tc[j].strideLimit) {
                        drop = Double.MAX_VALUE;
                        break;
                    }
                    if (tr.get(i) == j) {
                        tc[j].offload(f);
                    }
                    int s = Integer.MAX_VALUE;
                    if (tc[j].latestOffload >= 0) {
                        s = f - tc[j].latestOffload;
                    }
                    drop += d * Math.min(accDropRates[j] * s, tc[j].accDropLimit);
                }
                if (drop == Double.MAX_VALUE) {
                    break;
                }
                d *= discount;
            }
            if (drop < minDrop) {
                minDrop = drop;
                minTask = tr.get(0);
            }
        }

        if (minTask == -1) {
            if (tasks[0].latestOffload < tasks[1].latestOffload)
                minTask = 0;
            else
                minTask = 1;
        }
        return minTask;
    }
}
