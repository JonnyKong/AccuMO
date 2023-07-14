package com.example.accumo;

import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.TreeMap;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import lombok.SneakyThrows;

public class Prefetcher {
    private static final String TAG = "Prefetcher";

    Prefetcher(File[] files, int numPrefetch) {
        this.files = files;
        this.numPrefetch = numPrefetch;
    }

    public void run() {
        // Prefetch once before run() returns to make sure there are prefetched frames ready
        prefetch();

        executor.execute(this::prefetchWorker);
    }

    @SneakyThrows(IOException.class)
    public byte[] get(int frameIdx) {
        Log.d(TAG, "get(): " + frameIdx);
        byte[] yuv;
        synchronized (cache) {
            if (cache.containsKey(frameIdx)) {
                yuv = cache.get(frameIdx);
            } else {
                // This shouldn't happen under sequential access
                Log.e(TAG, "Prefetch missed for frame: " + frameIdx);
                yuv = Files.readAllBytes(files[frameIdx].toPath());
            }
            lastAccessedFrameIdx = frameIdx;
            cache.notify();
        }
        return yuv;
    }

    @SneakyThrows(InterruptedException.class)
    private void prefetchWorker() {
        while (true) {
            synchronized (cache) {
                while (lastAccessedFrameIdx + numPrefetch <= lastCachedFrameIdx) {
                    cache.wait();
                }
            }
            if (lastCachedFrameIdx + 1 >= files.length) {
                break;
            }
            prefetch();
        }
    }

    @SneakyThrows(IOException.class)
    private void prefetch() {
        // Evict passed frames
        synchronized (cache) {
            cache.headMap(lastAccessedFrameIdx + 1).clear();
        }

        // Prefetch until numPrefetch frames are cached
        int prefetchFrom;
        int prefetchTo;
        synchronized (cache) {
            prefetchFrom = lastCachedFrameIdx + 1;
            prefetchTo = Math.min(files.length - 1, lastAccessedFrameIdx + numPrefetch);
        }
        for (int frameIdx = prefetchFrom; frameIdx <= prefetchTo; frameIdx++) {
            byte[] yuv = Files.readAllBytes(files[frameIdx].toPath());
            Log.d(TAG, "Prefetched frame: " + frameIdx);
            synchronized (cache) {
                cache.put(frameIdx, yuv);
                lastCachedFrameIdx++;
            }
        }
    }

    private int numPrefetch;
    private int lastAccessedFrameIdx = -1;
    private int lastCachedFrameIdx = -1;
    private final File[] files;
    private final Executor executor = Executors.newSingleThreadExecutor();
    private final TreeMap<Integer, byte[]> cache = new TreeMap<>();
}
