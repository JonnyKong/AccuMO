package com.example.accumo;

import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;
import java.util.Random;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.Semaphore;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import com.example.accumo.OffloadProto.UploadMetadata;
import com.example.accumo.OffloadProto.DownloadMetadata;
import com.example.accumo.sched.MpcScheduler;
import com.example.accumo.sched.ParallelScheduler;
import com.example.accumo.sched.PatternScheduler;
import com.example.accumo.sched.Scheduler;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.databind.SequenceWriter;
import com.fasterxml.jackson.dataformat.csv.CsvMapper;
import com.fasterxml.jackson.dataformat.csv.CsvSchema;

import org.apache.commons.lang3.NotImplementedException;
import org.apache.commons.lang3.tuple.Pair;
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;
import org.opencv.imgcodecs.Imgcodecs;

import lombok.Data;
import lombok.experimental.Delegate;

public class TaskManager {
    private final File resultsRoot;

    private final Scheduler scheduler;

    private final Filter filter = new Filter();

    private final Filter filterImuOnly = new Filter();

    private final VoAccModel voAccModel;

    private final SequenceWriter posesWriter;

    private final State state = new State();

    private final SequenceWriter statesWriter;

    private boolean hasNewDepthResultForDAcc = false;

    @Data
    private static class State {
        private int aFrameIdx;

        // accuracy models
        private double bDepthDropRate = 0.02;

        @Delegate
        @JsonIgnore
        private DepthAccModel.State cDepthAccState = new DepthAccModel.State();

        private double dVoDropRate = 0;

        // offloading
        private int eOffloadTask;

        private int fDepthLatestOffload = -1;

        private boolean gDepthUseLocal;

        private int hVoLatestOffload = -1;

        // timing
        private long iNsFrameAvailable;

        private long jNsTracking;

        private long kNsOffloading = -1;

        private long lNsDepthAcc = -1;

        private long mNsInference = -1;

        private double nAbsRelGolden = 1d;

        private long oNsEncoding = -1;     // Client encoding time

        private long pNsDecoding = -1;  // Server decoding time
    }

    TaskManager(MainActivity ma, String host, String port, String video, String sched,
                int[] schedPattern, boolean enableFastdepth, int imgH, int imgW,
                int offloadH, int offloadW, boolean doEncoding, float mpcWeight) {
        this.ma = ma;
        this.host = host;
        this.port = Integer.parseInt(port);
        if (sched.equals("rr"))
            scheduler = new PatternScheduler(schedPattern);
        else if (sched.equals("mpc"))
            scheduler = new MpcScheduler();
        else if (sched.equals("parallel"))
            scheduler = new ParallelScheduler();
        else
            throw new NotImplementedException();
        StringBuilder saveDir = new StringBuilder(sched);
        if (sched.equals("rr")) {
            for (int i : schedPattern) {
                saveDir.append(i);
            }
            if (enableFastdepth)
                saveDir.append("_w_fastdepth");
        } else if(!enableFastdepth) {
            saveDir.append("_wo_fastdepth");
        }
        if (mpcWeight != 2.5) {
            saveDir.append(String.valueOf(mpcWeight));
        }
        this.resultsRoot = new File("/sdcard/accumo/results/" + saveDir.toString(), video);
        this.resultsRoot.mkdirs();
        this.enableFastdepth = enableFastdepth;
        this.imgH = imgH;
        this.imgW = imgW;
        this.offloadH = offloadH;
        this.offloadW = offloadW;
        this.doEncoding = doEncoding;
        sequential = new Semaphore(1);
        pendingFrames = new LinkedBlockingQueue<>();
        dAcc = new DepthAccModel(ma);
        this.voAccModel = new VoAccModel(filterImuOnly, mpcWeight);

        SequenceWriter posesWriter1 = null;
        SequenceWriter statesWriter1 = null;
        try {
            CsvMapper mapper = new CsvMapper();
            CsvSchema schema = mapper.schema().withColumnSeparator(' ');
            posesWriter1 = mapper.writer(schema).writeValues(new File(resultsRoot, "poses.txt"));
            CsvSchema stateSchema = mapper.schemaFor(State.class)
                    .withHeader();
            statesWriter1 = mapper.writer(stateSchema)
                    .writeValues(new File(resultsRoot, "states.csv"));
        } catch (IOException e) {
            e.printStackTrace();
        }
        posesWriter = posesWriter1;
        statesWriter = statesWriter1;

        // Initialize encoder
        if (doEncoding) {
            avcEncoder = new AvcEncoder(imgW, imgH, 60, bitrate);
            avcEncoder.startEncoderThread();
        }

        // Load paths to pre-computed depth maps
        predDepths = new File(adabinsPredRoot, video).listFiles(((file, s) -> s.endsWith(".bin")));
        if (predDepths != null)
            Arrays.sort(predDepths);
    }

    public void connect() throws java.io.IOException {
        ma.loadModel("fast-depth-64x224-simplified");

        socket = new Socket(this.host, this.port);
        socket.setTcpNoDelay(true);
        out = new BufferedOutputStream(socket.getOutputStream());
        in = new DataInputStream(new BufferedInputStream(socket.getInputStream()));

        offloadDummyFrame(0x10000000, 0);
        offloadDummyFrame(0x10000000 + 9, 1);
        offloadDummyFrame(0x10000000 + 18, 0);
        offloadDummyFrame(0x10000000 + 27, 1);

        new Thread(this::pollResults).start();
        Depth.initDepthSaver();
    }

    public void shutdown() throws java.io.IOException {
        socket.close();
        filter.cleanup();
        filterImuOnly.cleanup();
        posesWriter.close();
        statesWriter.close();
    }

    /*
     * Called by DiskFrameReader on each new frame.
     */
    public void onIncomingFrame(int frameIdx, byte[] yuv, double[] imu, double dist) {
        try {
            statesWriter.write(state);
        } catch (IOException e) {
            e.printStackTrace();
        }
        state.aFrameIdx = frameIdx;
        state.iNsFrameAvailable = System.nanoTime();

        // Offload if no outstanding requests
        if (sequential == null || sequential.tryAcquire()) {
            executor.execute(() -> offload(yuv, frameIdx, dist));
        } else {
            state.eOffloadTask = -1;
        }

        double[] t = Arrays.copyOfRange(imu, 0, 3);
        double[] r = Arrays.copyOfRange(imu, 3, 6);
        long start = System.nanoTime();
        filter.predict(t, r);
        filterImuOnly.predict(t, r);
        poseTrackerDuration.add(System.nanoTime() - start);
        try {
            posesWriter.write(Arrays.copyOf(filter.pose(), 12));
        } catch (IOException e) {
            e.printStackTrace();
        }

        float[] depthArr;
        // If possible, warp from the last offloaded depth
        boolean canWarp = (lastOffloadedDepth != null);
        boolean shouldUseSmallModel = false;
        if (enableFastdepth)
            shouldUseSmallModel = (state.bDepthDropRate * (frameIdx - state.fDepthLatestOffload)
                    >= scheduler.getTask(0).getAccDropLimit());
        boolean shouldRunDepthAcc = (!dAcc.isBusy && frameIdx - state.fDepthLatestOffload >= 6
                && (!shouldUseSmallModel || hasNewDepthResultForDAcc));
        if (canWarp && (!shouldUseSmallModel || shouldRunDepthAcc)) {
            // Save a local copy of needed variables to reduce race
            int frameIdxLocal = frameIdx;
            int fDepthLatestOffload = state.fDepthLatestOffload;
            Mat lastOffloadedDepthRgbLocal = lastOffloadedDepthRgb.clone();
            float[] lastOffloadedDepthLocal = lastOffloadedDepth.clone();

            state.gDepthUseLocal = false;
            // Get relative pose change
            double[] poseChangeDouble = filterImuOnly.poseRelative(frameIdxLocal, fDepthLatestOffload);
            float[] poseChangeArr = new float[16];
            for (int j = 0; j < poseChangeArr.length; j++) {
                poseChangeArr[j] = (float) poseChangeDouble[j];
            }

            // Warp
            List<int[]> rgbArr = Util.convertMatToArrays(lastOffloadedDepthRgbLocal);
            depthArr = ma.WarpForDepthWithRGB(lastOffloadedDepthLocal, poseChangeArr,
                    rgbArr.get(0), rgbArr.get(1), rgbArr.get(2));

            // Convert RGB from int[] to Mat
            Mat warpedRGB = Util.convertRGBArraysToMat(imgH, imgW, rgbArr.get(0), rgbArr.get(1), rgbArr.get(2));

            // Run depth acc model
            if (shouldRunDepthAcc) {
                dAccExecutor.execute(() -> {
                    long nsDepthAccStart = System.nanoTime();
                    int offloadIdx = fDepthLatestOffload;
                    Mat currentRGB = Util.convertYUVArraysToMat(imgH, imgW, yuv);

                    dAcc.isBusy = true;
                    dAcc.observe(offloadIdx, frameIdxLocal, warpedRGB, depthArr, currentRGB);
                    state.bDepthDropRate = dAcc.decide()[0];
                    state.cDepthAccState = dAcc.getState();
                    state.lNsDepthAcc = System.nanoTime() - nsDepthAccStart;
                    state.nAbsRelGolden = dAcc.observeGolden(offloadIdx, lastOffloadedDepthLocal, lastOffloadedDepthRgbLocal);
                    if (enableFastdepth)
                        scheduler.getTask(0).setAccDropLimit(state.nAbsRelGolden - 0.04);
                    dAcc.isBusy = false;
                });
                hasNewDepthResultForDAcc = false;
            }

            // Unmarshall depth and save
            if (false) {
                Mat warpedBGR = new Mat();
                warpedRGB.convertTo(warpedRGB, CvType.CV_8UC1);
                Imgproc.cvtColor(warpedRGB, warpedBGR, Imgproc.COLOR_RGB2BGR);
                imageCodecs.imwrite("/sdcard/multitask/offloaded_depth/" + String.format("%06d.jpg", frameIdxLocal), warpedBGR);
            }
        } else {
            state.gDepthUseLocal = true;
            depthArr = ma.inference(yuv);
        }
        File depthSaveFile = new File(resultsRoot, String.format("depth/%06d.png", frameIdx));
        Depth.saveDepthAsync(depthArr, imgH, imgW, depthSaveFile.getPath());

        state.dVoDropRate = voAccModel.decide();
        state.jNsTracking = System.nanoTime() - state.iNsFrameAvailable;
        if (frameIdx == 2330) {
            Log.i("TaskManager", "depth mean: " + calculateAverage(depthPostProcessing));
            Log.i("TaskManager", "pose mean: " + calculateAverage(posePostProcessing));
            Log.i("TaskManager", "scheduler mean: " + calculateAverage(schedulerDuration));
            Log.i("TaskManager", "pose tracker mean: " + calculateAverage(poseTrackerDuration));
        }
    }

    private double calculateAverage(List <Long> marks) {
        Long sum = (long)0;
        if(!marks.isEmpty()) {
            for (Long mark : marks) {
                sum += mark;
            }
            return sum.doubleValue() / marks.size();
        }
        return sum;
    }

    /*
     * Offload one frame to the server.
     */
    private void offload(byte[] yuv, int frameIdx, double dist) {
        long start = System.nanoTime();
        int task = scheduler.schedule(frameIdx, state.bDepthDropRate, state.dVoDropRate);
        schedulerDuration.add(System.nanoTime() - start);
        scheduler.offload(task, frameIdx);
        if (task == 1 || task == 2) {
            filter.startOffload();
        }
        state.eOffloadTask = task;

        // Save pending frame info, to be used when result comes back in pollResults()
        PendingFrameInfo info = new PendingFrameInfo();
        info.frameIdx = frameIdx;
        info.nsOffloadTime = state.iNsFrameAvailable;

        if (this.doEncoding)
            offloadNetH264(yuv, frameIdx, dist, task, info);
        else
            offloadNet(yuv, frameIdx, dist, task, info);

        info.rgb = Util.convertYUVArraysToMat(imgH, imgW, yuv);
        try{
            pendingFrames.put(info);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void offloadNet(byte[] yuv, int frameIdx, double dist, int task,
                            PendingFrameInfo info) {

        long encodingStart = System.nanoTime();

        String yuvFormat = "NV12";
        if (imgH != offloadH || imgW != offloadW) {
            // Convert YUV to RGB, resize, and convert back to YUV for transmission
            Mat yuvMat = new Mat(imgH * 3 / 2, imgW, CvType.CV_8UC1);
            yuvMat.put(0, 0, yuv);
            Mat rgbMat = new Mat();
            Imgproc.cvtColor(yuvMat, rgbMat, Imgproc.COLOR_YUV2RGB_NV12);

            Imgproc.resize(rgbMat, rgbMat, new Size(offloadW, offloadH));

            // No conversion to NV12, convert to I420 instead
            Imgproc.cvtColor(rgbMat, yuvMat, Imgproc.COLOR_RGB2YUV_I420);
            yuvFormat = "I420";
            yuv = new byte[(int)(offloadH * offloadW * 1.5)];
            yuvMat.get(0, 0, yuv);
        }

        if (info != null)
            info.oNsEncoding = System.nanoTime() - encodingStart;

        UploadMetadata metadata = UploadMetadata.newBuilder()
                .setFrameIdx(frameIdx)
                .setFrameSize(yuv.length)
                .setTaskType(task)
                .setImgW(offloadW)
                .setImgH(offloadH)
                .setDistance(dist)
                .setYuvFormat(yuvFormat)
                .build();
        try{
            // Metadata length
            byte[] metadataBytes = metadata.toByteArray();
            ByteBuffer buf = ByteBuffer.allocate(4);
            buf.order(ByteOrder.BIG_ENDIAN);
            out.write(buf.putInt(metadataBytes.length).array());

            // Metadata
            out.write(metadataBytes);

            // Frame
            out.write(yuv);
            out.flush();

        } catch (java.io.IOException e) {
            e.printStackTrace();
        }
    }

    private void offloadNetH264(byte[] yuv, int frameIdx, double dist, int task,
                                PendingFrameInfo info) {
        long encodingStart = System.nanoTime();
        AvcEncoder.inputQueue.add(yuv);

        // For the first frame, need to poll encoder twice
        int encoderRepeatTimes = 1;
        if (!hasEncodedFirstFrame) {
            encoderRepeatTimes = 2;
        }

        for (int i = 0; i < encoderRepeatTimes; i++) {
            try{
                // Get data from decoder
                byte[] encodedData = null;
                try {
                    encodedData = AvcEncoder.outputQueue.poll(25L, TimeUnit.MILLISECONDS);
                } catch (Exception e) {
                    e.printStackTrace();
                }

                if (info != null)
                    info.oNsEncoding = System.nanoTime() - encodingStart;

                UploadMetadata metadata = UploadMetadata.newBuilder()
                        .setFrameIdx(frameIdx)
                        .setFrameSize(encodedData.length)
                        .setTaskType(task)
                        .setImgW(imgW)
                        .setImgH(imgH)
                        .setDistance(dist)
                        .build();

                // Metadata length
                byte[] metadataBytes = metadata.toByteArray();
                ByteBuffer buf = ByteBuffer.allocate(4);
                buf.order(ByteOrder.BIG_ENDIAN);
                out.write(buf.putInt(metadataBytes.length).array());

                // Metadata
                out.write(metadataBytes);

                // Frame
                out.write(encodedData);
                out.flush();
            } catch (java.io.IOException e) {
                e.printStackTrace();
                return;
            }
        }

        // Drop repetitive data
        if (!hasEncodedFirstFrame) {
            try {
                AvcEncoder.outputQueue.poll(25L, TimeUnit.MILLISECONDS);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        hasEncodedFirstFrame = true;
    }

    private void pollResults() {
        while (true) {
            Pair<DownloadMetadata, byte[]> data = pullNet();
            DownloadMetadata metadata = data.getLeft();
            byte[] buf = data.getRight();

            scheduler.finishOffload(metadata.getTaskType());
            PendingFrameInfo info;
            try {
                info = pendingFrames.take();
            } catch (InterruptedException e) {
                e.printStackTrace();
                return;
            }

            // If two tasks offloaded in parallel, break up buf into two
            byte[] bufTask0 = null;
            byte[] bufTask1 = null;
            if (metadata.getTaskType() == 0) {
                bufTask0 = buf;
            }
            else if (metadata.getTaskType() == 1) {
                bufTask1 = buf;
            }
            else if (metadata.getTaskType() == 2) {
                bufTask0 = Arrays.copyOfRange(buf, 0, metadata.getTask0ResultSize());
                bufTask1 = Arrays.copyOfRange(buf, metadata.getTask0ResultSize(), buf.length);
            }

            long start = System.nanoTime();
            if (bufTask0 != null) {
                float[] offloadedDepth = new float[imgH * imgW];
                if (metadata.getReadDepthFromDisk()) {
                    try {
//                        Log.i("TaskManager", "loading from disk: " + predDepths[metadata.getFrameIdx()].toString());
                        bufTask0 = Files.readAllBytes(predDepths[metadata.getFrameIdx()].toPath());
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }

                ByteBuffer byteBuffer = ByteBuffer.wrap(bufTask0).order(ByteOrder.LITTLE_ENDIAN);
                short[] depthShort = new short[bufTask0.length / Short.BYTES];
                byteBuffer.asShortBuffer().get(depthShort);

                // Borrow OpenCV to cast from uint16 to float32 because it's faster
                Mat depth = new Mat(imgH, imgW, CvType.CV_16UC1);
                depth.put(0, 0, depthShort);
                depth.convertTo(depth, CvType.CV_32F);
                Core.multiply(depth, new Scalar(1 / 256.0), depth);
                depth.get(0, 0, offloadedDepth);

                state.fDepthLatestOffload = info.frameIdx;
                lastOffloadedDepth = offloadedDepth;
                lastOffloadedDepthRgb = info.rgb;
                hasNewDepthResultForDAcc = true;

                // Unmarshall depth and save
                if (false) {
                    Mat offloadDepthMat = new Mat(imgH, imgW, CvType.CV_32F);
                    offloadDepthMat.put(0, 0, offloadedDepth);
                    Depth.saveDepthAsync(offloadDepthMat,
                            "/sdcard/multitask/offloaded_depth/" + info.frameIdx + ".png");
                }
                depthPostProcessing.add((System.nanoTime() - start));
            }
            if (bufTask1 != null) {
                ByteBuffer byteBuffer = ByteBuffer.wrap(bufTask1).order(ByteOrder.LITTLE_ENDIAN);
                double[] pose = new double[bufTask1.length / Double.BYTES];
                byteBuffer.asDoubleBuffer().get(pose);
                filter.fusemvo(pose);
                state.hVoLatestOffload = info.frameIdx;
                posePostProcessing.add((System.nanoTime() - start));
            }

            state.kNsOffloading = System.nanoTime() - info.nsOffloadTime;
            state.mNsInference = metadata.getTimeInference();
            state.oNsEncoding = info.oNsEncoding;
            state.pNsDecoding = metadata.getTimeDecoding();

            if (sequential != null) {
                sequential.release();
            }
        }
    }

    private Pair<DownloadMetadata, byte[]> pullNet() {
        try {
            DownloadMetadata metadata;
            byte[] buf;
            // Discard dummy bytes
            do {
                // Metadata length
                int metadataSize = in.readInt();

                // Metadata
                buf = new byte[metadataSize];
                in.readFully(buf);
                metadata = DownloadMetadata.parseFrom(buf);

                buf = new byte[metadata.getResultSize()];
                in.readFully(buf);
            } while (metadata.getTaskType() == -1);

            return Pair.of(metadata, buf);
        } catch (java.io.IOException e) {
            e.printStackTrace();
            return null;
        }
    }

    private void offloadDummyFrame(int frameIdx, int task) {
        // If dummy frame is set, offload dummy frame, otherwise offload a random frame
        byte[] yuv;
        if (yuvDummyFrame != null) {
            yuv = yuvDummyFrame;
        } else {
            yuv = new byte[imgH*imgW*3/2];
            new Random().nextBytes(yuv);
        }
        if (this.doEncoding)
            offloadNetH264(yuv, frameIdx, 1, task, null);
        else
            offloadNet(yuv, frameIdx, 1, task, null);
        pullNet();
    }

    public void setDummyFrame(byte[] f) {
        yuvDummyFrame = f;
    }

    private MainActivity ma; // To have access to the JNI functions
    private Socket socket;
    private OutputStream out;
    private DataInputStream in;
    private final String host;
    private final boolean enableFastdepth;
    private final int port;
    private final int imgH;
    private final int imgW;
    private final int offloadH;
    private final int offloadW;
    private final boolean doEncoding;
    private final Semaphore sequential;
    private final BlockingQueue<Runnable> executorQueue = new ArrayBlockingQueue(1);
    private final ThreadPoolExecutor executor =
            new ThreadPoolExecutor(1, 1, 0L, TimeUnit.MILLISECONDS, executorQueue);
    private float[] lastOffloadedDepth;       // Depth of the last offloaded depth frame
    private Mat lastOffloadedDepthRgb;     // RGB of the last offloaded depth frame
    private final BlockingQueue<PendingFrameInfo> pendingFrames;
    Imgcodecs imageCodecs = new Imgcodecs();
    private DepthAccModel dAcc;
    private byte[] yuvDummyFrame;

    private final ExecutorService dAccExecutor = Executors.newSingleThreadExecutor();
    private AvcEncoder avcEncoder = null;
    private boolean hasEncodedFirstFrame = false;
    private int bitrate = 24576000;
    private final File adabinsPredRoot = new File("/sdcard/multitask/adabinsPred");
    private final File[] predDepths;
    private List<Long> depthPostProcessing = new ArrayList<>();
    private List<Long> posePostProcessing = new ArrayList<>();
    private List<Long> schedulerDuration = new ArrayList<>();
    private List<Long> poseTrackerDuration = new ArrayList<>();
}

class PendingFrameInfo {
    public int frameIdx;
    public long nsOffloadTime;
    public Mat rgb;
    public long oNsEncoding;
}