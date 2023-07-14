package com.example.accumo;

import java.io.File;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import lombok.SneakyThrows;

public class DiskFrameReader {
    private final File datasetRoot = new File("/sdcard/accumo/dataset");

    private final File videoRoot;

    private Prefetcher prefetcher;

    public DiskFrameReader(TaskManager tm, String video, int imgH, int imgW) {
        this.tm = tm;
        this.videoRoot = new File(datasetRoot, video);
        this.imgH = imgH;
        this.imgW = imgW;
    }

    @SneakyThrows(InterruptedException.class)
    void run() {
        // Read rgb
        File[] frames = videoRoot.listFiles(((file, s) -> s.endsWith(".yuv")));
        assert frames != null;
        Arrays.sort(frames);
        this.prefetcher = new Prefetcher(frames, 5);
        this.prefetcher.run();

        List<double[]> imu = Util.loadArrays(new File(videoRoot, "imu_right_noise.txt"), 6);
        List<Double> distances = Util.loadArray(new File(videoRoot, "distances.txt"));

        try {
            // Use first frame as dummy frame, otherwise first few encoded image has low quality
            byte[] yuvDummyFrame = prefetcher.get(0);
            tm.setDummyFrame(yuvDummyFrame);
            tm.connect();
        } catch (java.io.IOException e) {
            e.printStackTrace();
            return;
        }

        isRunning = true;
        ScheduledExecutorService executor = Executors.newSingleThreadScheduledExecutor();
        CountDownLatch latch = new CountDownLatch(1);
        future = executor.scheduleAtFixedRate(() -> {
            if (!isRunning) {
                future.cancel(false);
                latch.countDown();
            }

            byte[] yuv = prefetcher.get(frameIdx);

            tm.onIncomingFrame(frameIdx, yuv, imu.get(frameIdx), distances.get(frameIdx));

            if (++frameIdx >= frames.length) {
                future.cancel(false);
                latch.countDown();
            }
        }, 0, 16667, TimeUnit.MICROSECONDS);
        latch.await();
    }

    private ScheduledFuture<?> future;
    private int frameIdx = 0;
    private final TaskManager tm;
    private boolean isRunning = false;
    private final int imgH;
    private final int imgW;

    void shutdown() {
        try {
            tm.shutdown();
        } catch (java.io.IOException e) {
            e.printStackTrace();
        }
        isRunning = false;
    }
}