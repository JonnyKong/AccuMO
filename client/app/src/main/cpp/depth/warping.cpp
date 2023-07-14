#include <cassert>
#include "warping.h"

float **defaultPixelCoordinates; // 3x(H*W)
float **intrinsics;
float **intrinsicsInv;
float **intrinsicsInvMultiPixelCoordinates; // For memoization
const int H = 256;
const int W = 832;

extern "C" JNIEXPORT void JNICALL
Java_com_example_accumo_MainActivity_InitializeWarping(JNIEnv *env,
                                                          jobject) {
  ALOG("Enter InitializeWarping %d.", __LINE__);

  initializeDefaultPixelCoordinates();

  // Initialize carla params
  float intrinsicsArr[] = {
      483.34, 0., 416., 0., 483.34, 128., 0., 0., 1.,
  };
  intrinsics = convertArrToMat(3, 3, intrinsicsArr);
  float intrinsicsInvArr[] = {
      0.00206894, 0., -0.86067778, 0., 0.00206894, -0.26482393, 0., 0., 1.,
  };
  intrinsicsInv = convertArrToMat(3, 3, intrinsicsInvArr);

  initialize2DArray(&intrinsicsInvMultiPixelCoordinates, 3, H * W);
  crossProductMatrices(intrinsicsInv, defaultPixelCoordinates,
                       intrinsicsInvMultiPixelCoordinates, 3, 3, 3, H * W);
}

void initialize2DArray(float ***matrix, int X, int Y) {
  *matrix = new float *[X];
#pragma omp parallel
  {
#pragma omp for
    for (int i = 0; i < X; ++i) {
      (*matrix)[i] = new float[Y];
    }
  }
}

void free2DArray(float **matrix, int X, int Y) {
#pragma omp parallel
  {
#pragma omp for
    for (int i = 0; i < X; ++i) {
      delete[] matrix[i];
    }
  }
  delete[] matrix;
}

float **convertArrToMat(int X, int Y, float *a) {
  float **matrix;
  initialize2DArray(&matrix, X, Y);
  for (int i = 0; i < X; ++i) {
    for (int j = 0; j < Y; ++j) {
      matrix[i][j] = a[i * Y + j];
    }
  }
  return matrix;
}

void print1DArray(float *arr) {
  for (int i = 100; i < 105; i++)
    ALOG("%f,%f,%f", arr[i * W + 100], arr[i * W + 101], arr[i * W + 102],
         arr[i * W + 103], arr[i * W + 104]);
}

void initializeDefaultPixelCoordinates() {
  defaultPixelCoordinates = new float *[3];
#pragma omp parallel
  {
#pragma omp for
    for (int i = 0; i < 3; ++i) {
      defaultPixelCoordinates[i] = new float[H * W]();
    }
    for (int i = 0; i < H; i++) {
      for (int j = 0; j < W; j++) {
        defaultPixelCoordinates[0][i * W + j] = j;
        defaultPixelCoordinates[1][i * W + j] = i;
        defaultPixelCoordinates[2][i * W + j] = 1;
      }
    }
  }
}

void crossProductMatrices(float **firstMatrix, float **secondMatrix,
                          float **mult, int rowFirst, int columnFirst,
                          int rowSecond, int columnSecond) {
  ALOG("crossProductMatrices %d.", __LINE__);
#pragma omp parallel
  {
    // Multiplying matrix firstMatrix and secondMatrix and storing in array
    // mult.
#pragma omp for
    for (int i = 0; i < rowFirst; ++i) {
      for (int j = 0; j < columnSecond; ++j) {
        mult[i][j] = 0.;
        for (int k = 0; k < columnFirst; ++k) {
          mult[i][j] += firstMatrix[i][k] * secondMatrix[k][j];
        }
      }
    }
  }
}

void scaleCamCoordinatesByDepth(float **matrix, float *depth,
                                float **scaledMatrix, int rowMatrix,
                                int columnMatrix) {
  ALOG("Enter scaleCamCoordinatesByDepth %d.", __LINE__);
#pragma omp parallel
  {
    int k, t;

#pragma omp for
    for (int i = 0; i < rowMatrix; ++i) {
      for (int j = 0; j < columnMatrix; ++j) {
        k = j / W;
        t = j % W;
        scaledMatrix[i][j] = matrix[i][j] * depth[k * W + t];
      }
    }
  }
}

void addHomogeneousCoordinate(float **origMatrix, float **outputMatrix, int H,
                              int W, float paddingVal) {
  ALOG("Enter addHomogeneousCoordinate %d.", __LINE__);
#pragma omp parallel
  {
#pragma omp for
    for (int j = 0; j < H; ++j) {
      for (int k = 0; k < W; ++k) {
        for (int i = 0; i < 3; ++i) {
          outputMatrix[i][j * W + k] = origMatrix[i][j * W + k];
        }
        outputMatrix[3][j * W + k] = paddingVal;
      }
    }
  }
}

void stripHomogeneousCoordinate(float **origMatrix, float **outputMatrix, int H,
                                int W) {
  ALOG("Enter stripHomogeneousCoordinate %d.", __LINE__);
#pragma omp parallel
  {
#pragma omp for
    for (int j = 0; j < H; ++j) {
      for (int k = 0; k < W; ++k) {
        for (int i = 0; i < 3; ++i) {
          outputMatrix[i][j * W + k] = origMatrix[i][j * W + k];
        }
      }
    }
  }
}

// outputDepth with the default value of FLT_MAX
void convertPointCloudToDepthMap(float **pixelCoordinates, float *outputDepth,
                                 int *b, int *bWarped, int *g, int *gWarped,
                                 int *r, int *rWarped) {
  ALOG("Enter convertPointCloudToDepthMap %d.", __LINE__);
#pragma omp parallel
  {
    int normalized_x, normalized_y;
    float x, y, z;
    memset(outputDepth, 0, W * H * sizeof(outputDepth[0]));
    if (b) {
      memset(bWarped, 0, H * W * sizeof(bWarped[0]));
      memset(gWarped, 0, H * W * sizeof(gWarped[0]));
      memset(rWarped, 0, H * W * sizeof(rWarped[0]));
    }

#pragma omp for
    for (int i = 0; i < H * W; ++i) {
      x = pixelCoordinates[0][i];
      y = pixelCoordinates[1][i];
      z = pixelCoordinates[2][i];
      normalized_x = static_cast<int>(x / z + 0.5);
      normalized_y = static_cast<int>(y / z + 0.5);
      if (normalized_x >= 0 && normalized_x < W && normalized_y >= 0 &&
          normalized_y < H) {
        if (outputDepth[normalized_y * W + normalized_x] == 0.0 ||
            z < outputDepth[normalized_y * W + normalized_x] == 0.0) {
          outputDepth[normalized_y * W + normalized_x] = z;
          if (b) {
            bWarped[normalized_y * W + normalized_x] = b[i];
            gWarped[normalized_y * W + normalized_x] = g[i];
            rWarped[normalized_y * W + normalized_x] = r[i];
          }
        }
      }
    }
  }
}

// Approximation of nearest interpolation
void interpNearest(int H, int W, float *mat, int *bWarped, int *gWarped,
                   int *rWarped)
#pragma omp parallel
{
  ALOG("Enter interpNearest %d.", __LINE__);
  // For each invalid point, get its distance to the nearest valid point in
  // each direction
  float *lDist = new float[H * W];
  float *rDist = new float[H * W];
  float *uDist = new float[H * W];
  float *dDist = new float[H * W];

#pragma omp for
  for (int i = 0; i < H; i++) {
    // left
    for (int j = 0; j < W; j++) {
      if (mat[i * W + j] > 0) {
        lDist[i * W + j] = 0;
      } else if (j == 0) {
        lDist[i * W + j] = 10000; // Means invalid
      } else {
        lDist[i * W + j] = lDist[i * W + j - 1] + 1;
      }
    }
    // right
    for (int j = W - 1; j >= 0; j--) {
      if (mat[i * W + j] > 0) {
        rDist[i * W + j] = 0;
      } else if (j == W - 1) {
        rDist[i * W + j] = 10000;
      } else {
        rDist[i * W + j] = rDist[i * W + j + 1] + 1;
      }
    }
  }
#pragma omp for
  for (int j = 0; j < W; j++) {
    // up
    for (int i = 0; i < H; i++) {
      if (mat[i * W + j] > 0) {
        uDist[i * W + j] = 0;
      } else if (i == 0) {
        uDist[i * W + j] = 10000;
      } else {
        uDist[i * W + j] = uDist[(i - 1) * W + j] + 1;
      }
    }
    // down
    for (int i = H - 1; i >= 0; i--) {
      if (mat[i * W + j] > 0) {
        dDist[i * W + j] = 0;
      } else if (i == H - 1) {
        dDist[i * W + j] = 10000;
      } else {
        dDist[i * W + j] = dDist[(i + 1) * W + j] + 1;
      }
    }
  }

  // Fill
#pragma omp for
  for (int i = 0; i < H; i++) {
    for (int j = 0; j < W; j++) {
      if (mat[i * W + j] == 0) {
        int fillIdx = -1;
        int l = lDist[i * W + j];
        int r = rDist[i * W + j];
        int u = uDist[i * W + j];
        int d = dDist[i * W + j];
        if (l < 10000 || r < 10000 || u < 10000 || d < 10000) {
          if (l < r && l < u && l < d)
            fillIdx = i * W + j - l;
          else if (r < u && r < d)
            fillIdx = i * W + j + r;
          else if (u < d)
            fillIdx = (i - u) * W + j;
          else
            fillIdx = (i + d) * W + j;
        } else {
          // TODO: think about what to fill in this case
        }
        if (fillIdx >= 0) {
          mat[i * W + j] = mat[fillIdx];
          if (bWarped) {
            bWarped[i * W + j] = bWarped[fillIdx];
            gWarped[i * W + j] = gWarped[fillIdx];
            rWarped[i * W + j] = rWarped[fillIdx];
          }
        }
      }
    }
  }

  delete[] lDist;
  delete[] rDist;
  delete[] uDist;
  delete[] dDist;
}

// prevDepth: HxW
// intrinsicsInv: 3x3
// intrinsics: 3x3
// poseChange: 4x4
// outputDepth: HxW
extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_example_accumo_MainActivity_WarpForDepthWithRGB(
    JNIEnv *env, jobject, jfloatArray dArr, jfloatArray poseChangeArr,
    jintArray bArr, jintArray gArr, jintArray rArr) {
  // Sanity check
  assert(env->GetArrayLength(dArr) == H * W);
  assert(env->GetArrayLength(poseChangeArr) == 16);
  assert(env->GetArrayLength(bArr) == 0 || env->GetArrayLength(bArr) == H * W);
  assert(env->GetArrayLength(gArr) == 0 || env->GetArrayLength(gArr) == H * W);
  assert(env->GetArrayLength(rArr) == 0 || env->GetArrayLength(rArr) == H * W);

  // Unmarshall
  float *d = env->GetFloatArrayElements(dArr, 0);
  float *poseChange = env->GetFloatArrayElements(poseChangeArr, 0);
  int *b = NULL;
  int *g = NULL;
  int *r = NULL;
  if (env->GetArrayLength(bArr) > 0) {
    b = env->GetIntArrayElements(bArr, 0);
    g = env->GetIntArrayElements(gArr, 0);
    r = env->GetIntArrayElements(rArr, 0);
  }

  // Warp
  float *dWarped = new float[H * W];
  int *bWarped = NULL;
  int *gWarped = NULL;
  int *rWarped = NULL;
  if (env->GetArrayLength(bArr) > 0) {
    bWarped = new int[H * W];
    gWarped = new int[H * W];
    rWarped = new int[H * W];
  }
  warp(d, intrinsicsInv, intrinsics, poseChange, dWarped, b, bWarped, g,
       gWarped, r, rWarped);

  // Marshall
  jfloatArray ret = env->NewFloatArray(H * W);
  env->SetFloatArrayRegion(ret, 0, H * W, dWarped);

  // Free
  env->ReleaseFloatArrayElements(dArr, d, 0);
  env->ReleaseFloatArrayElements(poseChangeArr, poseChange, 0);
  delete[] dWarped;
  if (env->GetArrayLength(bArr) > 0) {
    ALOG("Copy back");
    memcpy(b, bWarped, H * W * sizeof(b[0]));
    memcpy(g, gWarped, H * W * sizeof(g[0]));
    memcpy(r, rWarped, H * W * sizeof(r[0]));
    env->ReleaseIntArrayElements(bArr, b, 0);
    env->ReleaseIntArrayElements(gArr, g, 0);
    env->ReleaseIntArrayElements(rArr, r, 0);
    delete[] bWarped;
    delete[] gWarped;
    delete[] rWarped;
  }

  return ret;
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_example_accumo_MainActivity_WarpForDepth(
    JNIEnv *env, jobject obj, jfloatArray dArr, jfloatArray poseChangeArr) {
  return Java_com_example_accumo_MainActivity_WarpForDepthWithRGB(
      env, obj, dArr, poseChangeArr, env->NewIntArray(0), env->NewIntArray(0),
      env->NewIntArray(0));
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_accumo_MainActivity_interpNearest(JNIEnv *env, jobject obj,
                                                      jint h, jint w,
                                                      jfloatArray mat) {
  // Sanity check
  assert(env->GetArrayLength(mat) == h * w);

  float *d = env->GetFloatArrayElements(mat, 0);
  interpNearest(h, w, d, NULL, NULL, NULL);
  env->ReleaseFloatArrayElements(mat, d, 0);
}

void warp(float *prevDepth, float **intrinsicsInv, float **intrinsics,
          float poseChange[], float *outputDepth, int *b, int *bWarped, int *g,
          int *gWarped, int *r, int *rWarped) {
  ALOG("Enter warp %d.", __LINE__);
  float **_pose;
  float **prevCamCoordinates;
  float **warppedCamCoordinates;

  initialize2DArray(&_pose, 4, 4);
  initialize2DArray(&prevCamCoordinates, 4, H * W);
  initialize2DArray(&warppedCamCoordinates, 3, H * W);

  for (int i = 0; i < 16; ++i) {
    _pose[i / 4][i % 4] = poseChange[i];
  }

  pixelToCam(prevDepth, intrinsicsInv, defaultPixelCoordinates,
             prevCamCoordinates);
  convertCamCoordinates(prevCamCoordinates, _pose, warppedCamCoordinates);
  camToPixel(warppedCamCoordinates, intrinsics, outputDepth, b, bWarped, g,
             gWarped, r, rWarped);
  interpNearest(H, W, outputDepth, bWarped, gWarped, rWarped);

  free2DArray(_pose, 4, 4);
  free2DArray(prevCamCoordinates, 4, H * W);
  free2DArray(warppedCamCoordinates, 3, H * W);
}

// depth: HxW
// intrinsicsInv: 3x3
// pixelCoordinates: 3x(H*W)
// outputCamCoordinates: 4x(H*W)
void pixelToCam(float *depth, float **intrinsicsInv, float **pixelCoordinates,
                float **outputCamCoordinates) {
  ALOG("Enter pixelToCam %d.", __LINE__);
  float **intermCamCoordinates;
  initialize2DArray(&intermCamCoordinates, 3, H * W);

  scaleCamCoordinatesByDepth(intrinsicsInvMultiPixelCoordinates, depth,
                             intermCamCoordinates, 3, H * W);
  addHomogeneousCoordinate(intermCamCoordinates, outputCamCoordinates, H, W,
                           1.);

  free2DArray(intermCamCoordinates, 3, H * W);
}

// prevCamCamCoordinates: 4x(H*W)
// poseChange: 16 --> 4x4
// outputCamCoordinates: 3x(H*W)
void convertCamCoordinates(float **prevCamCoordinates, float **poseChange,
                           float **outputCamCoordinates) {
  ALOG("Enter convertCamCoordinates %d.", __LINE__);
  float **warppedCamCoordinates;

  initialize2DArray(&warppedCamCoordinates, 4, H * W); // Homogeneous

  crossProductMatrices(poseChange, prevCamCoordinates, warppedCamCoordinates, 4,
                       4, 4, H * W);
  stripHomogeneousCoordinate(warppedCamCoordinates, outputCamCoordinates, H, W);

  free2DArray(warppedCamCoordinates, 4, H * W); // Homogeneous
}

// camCoordinates: 3x(H*W)
// outputDepth: HxW with the default value of FLT_MAX
void camToPixel(float **camCoordinates, float **intrinsics, float *outputDepth,
                int *b, int *bWarped, int *g, int *gWarped, int *r,
                int *rWarped) {
  ALOG("Enter camToPixel %d.", __LINE__);
  float **pixelCoordinates;

  initialize2DArray(&pixelCoordinates, 3, H * W);

  crossProductMatrices(intrinsics, camCoordinates, pixelCoordinates, 3, 3, 3,
                       H * W);
  convertPointCloudToDepthMap(pixelCoordinates, outputDepth, b, bWarped, g,
                              gWarped, r, rWarped);

  free2DArray(pixelCoordinates, 3, H * W);
}