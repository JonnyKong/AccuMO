#ifndef MULTITASK_WARPING_H
#define MULTITASK_WARPING_H

#include <android/log.h>
#include <array>
#include <cstdint>
#include <jni.h>
#include <vector>

#define LOG_TAG "warping"
#define ALOG(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

void initialize2DArray(float ***matrix, int X, int Y);
void free2DArray(float **matrix, int X, int Y);
float **convertArrToMat(int X, int Y, float *a);
void initializeDefaultPixelCoordinates();
void crossProductMatrices(float **firstMatrix, float **secondMatrix,
                          float **mult, int rowFirst, int columnFirst,
                          int rowSecond, int columnSecond);
void scaleCamCoordinatesByDepth(float **matrix, float *depth,
                                float **scaledMatrix, int rowMatrix,
                                int columnMatrix);
void addHomogeneousCoordinate(float **origMatrix, float **outputMatrix, int H,
                              int W, float paddingVal);
void stripHomogeneousCoordinate(float **origMatrix, float **outputMatrix, int H,
                                int W);
void convertPointCloudToDepthMap(float **pixelCoordinates, float *outputDepth);
void interpNearest(int H, int W, float *mat, int *bWarped, int *gWarped, int *rWarped);

void warp(float *prevDepth, float **intrinsicsInv, float **intrinsics,
          float poseChange[], float *outputDepth, int *b, int *bWarped, int *g,
          int *gWarped, int *r, int *rWarped);
void pixelToCam(float *depth, float **intrinsicsInv, float **pixelCoordinates,
                float **outputCamCoordinates);
void convertCamCoordinates(float **prevCamCoordinates, float **poseChange,
                           float **outputCamCoordinates);
void camToPixel(float **camCoordinates, float **intrinsics, float *outputDepth,
                int *b, int *bWarped, int *g, int *gWarped, int *r,
                int *rWarped);

#endif // MULTITASK_WARPING_H
