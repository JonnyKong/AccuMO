/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: ErrorStateIMUGPSFuserBase.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 02-Mar-2022 23:31:18
 */

/* Include Files */
#include "ErrorStateIMUGPSFuserBase.h"
#include "insfilterErrorStateCoder_data.h"
#include "insfilterErrorStateCoder_internal_types.h"
#include "rt_nonfinite.h"
#include "rt_nonfinite.h"
#include <math.h>
#include <string.h>

/* Function Definitions */
/*
 * Arguments    : insfilterErrorState *obj
 *                const double voPos[3]
 *                const double voOrientIn[9]
 * Return Type  : void
 */
void c_ErrorStateIMUGPSFuserBase_fus(insfilterErrorState *obj,
                                     const double voPos[3],
                                     const double voOrientIn[9])
{
  static const double R[36] = {
      1.0E-9, 0.0,    0.0, 0.0,    0.0, 0.0,    0.0, 1.0E-9, 0.0,
      0.0,    0.0,    0.0, 0.0,    0.0, 1.0E-9, 0.0, 0.0,    0.0,
      0.0,    0.0,    0.0, 1.0E-9, 0.0, 0.0,    0.0, 0.0,    0.0,
      0.0,    1.0E-9, 0.0, 0.0,    0.0, 0.0,    0.0, 0.0,    1.0E-9};
  static const signed char b_iv[16] = {1, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0};
  static const signed char b_iv1[16] = {0, 1, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0};
  static const signed char b_iv2[16] = {0, 0, 1, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0};
  double b_y_tmp[256];
  double d_I[256];
  double e_I[256];
  double f_I[256];
  double H[96];
  double K[96];
  double y_tmp[96];
  double resCov[36];
  double b_K[16];
  double z[6];
  double psquared[4];
  double b_obj;
  double d;
  double d_idx_1;
  double d_idx_2;
  double deltaQ_a;
  double deltaQ_b;
  double deltaQ_c;
  double h_idx_0;
  double h_idx_1;
  double h_idx_4;
  double h_idx_5;
  double h_idx_6;
  double pd;
  int b_i;
  int b_tmp;
  int i;
  int j;
  int jA;
  int jAcol;
  int jBcol;
  int jp1j;
  int k;
  int mmj_tmp;
  signed char b_I[256];
  signed char c_I[256];
  signed char ipiv[6];
  signed char i1;
  bool exitg1;
  pd = (voOrientIn[0] + voOrientIn[4]) + voOrientIn[8];
  psquared[0] = (2.0 * pd + 1.0) - pd;
  psquared[1] = (2.0 * voOrientIn[0] + 1.0) - pd;
  psquared[2] = (2.0 * voOrientIn[4] + 1.0) - pd;
  psquared[3] = (2.0 * voOrientIn[8] + 1.0) - pd;
  if (!rtIsNaN(psquared[0])) {
    jA = 1;
  } else {
    jA = 0;
    k = 2;
    exitg1 = false;
    while ((!exitg1) && (k < 5)) {
      if (!rtIsNaN(psquared[k - 1])) {
        jA = k;
        exitg1 = true;
      } else {
        k++;
      }
    }
  }
  if (jA == 0) {
    pd = psquared[0];
    jA = 1;
  } else {
    pd = psquared[jA - 1];
    i = jA + 1;
    for (k = i; k < 5; k++) {
      d = psquared[k - 1];
      if (pd < d) {
        pd = d;
        jA = k;
      }
    }
  }
  switch (jA) {
  case 1:
    pd = sqrt(pd);
    d_idx_2 = 0.5 * pd;
    pd = 0.5 / pd;
    h_idx_0 = pd * (voOrientIn[7] - voOrientIn[5]);
    d_idx_1 = pd * (voOrientIn[2] - voOrientIn[6]);
    d = pd * (voOrientIn[3] - voOrientIn[1]);
    break;
  case 2:
    pd = sqrt(pd);
    h_idx_0 = 0.5 * pd;
    pd = 0.5 / pd;
    d_idx_2 = pd * (voOrientIn[7] - voOrientIn[5]);
    d_idx_1 = pd * (voOrientIn[1] + voOrientIn[3]);
    d = pd * (voOrientIn[2] + voOrientIn[6]);
    break;
  case 3:
    pd = sqrt(pd);
    d_idx_1 = 0.5 * pd;
    pd = 0.5 / pd;
    d_idx_2 = pd * (voOrientIn[2] - voOrientIn[6]);
    h_idx_0 = pd * (voOrientIn[1] + voOrientIn[3]);
    d = pd * (voOrientIn[5] + voOrientIn[7]);
    break;
  default:
    pd = sqrt(pd);
    d = 0.5 * pd;
    pd = 0.5 / pd;
    d_idx_2 = pd * (voOrientIn[3] - voOrientIn[1]);
    h_idx_0 = pd * (voOrientIn[2] + voOrientIn[6]);
    d_idx_1 = pd * (voOrientIn[5] + voOrientIn[7]);
    break;
  }
  if (d_idx_2 < 0.0) {
    d_idx_2 = -d_idx_2;
    h_idx_0 = -h_idx_0;
    d_idx_1 = -d_idx_1;
    d = -d;
  }
  b_obj = obj->pState[16];
  pd = obj->pState[0];
  h_idx_4 = obj->pState[1];
  h_idx_5 = obj->pState[2];
  h_idx_6 = obj->pState[3];
  deltaQ_a =
      ((pd * d_idx_2 - -h_idx_4 * h_idx_0) - -h_idx_5 * d_idx_1) - -h_idx_6 * d;
  deltaQ_b =
      ((pd * h_idx_0 + -h_idx_4 * d_idx_2) + -h_idx_5 * d) - -h_idx_6 * d_idx_1;
  deltaQ_c =
      ((pd * d_idx_1 - -h_idx_4 * d) + -h_idx_5 * d_idx_2) + -h_idx_6 * h_idx_0;
  h_idx_5 =
      ((pd * d + -h_idx_4 * d_idx_1) - -h_idx_5 * h_idx_0) + -h_idx_6 * d_idx_2;
  pd =
      sqrt(((deltaQ_a * deltaQ_a + deltaQ_b * deltaQ_b) + deltaQ_c * deltaQ_c) +
           h_idx_5 * h_idx_5);
  deltaQ_a /= pd;
  deltaQ_b /= pd;
  deltaQ_c /= pd;
  h_idx_5 /= pd;
  h_idx_4 = 2.0 * acos(deltaQ_a);
  h_idx_0 = obj->pState[4] * b_obj;
  h_idx_1 = obj->pState[5] * b_obj;
  h_idx_6 = obj->pState[6] * b_obj;
  pd = sqrt((deltaQ_b * deltaQ_b + deltaQ_c * deltaQ_c) + h_idx_5 * h_idx_5);
  deltaQ_a = 0.0;
  d_idx_1 = 0.0;
  d_idx_2 = 0.0;
  if (pd > 2.2204460492503131E-15) {
    deltaQ_a = h_idx_4 * deltaQ_b / pd;
    d_idx_1 = h_idx_4 * deltaQ_c / pd;
    d_idx_2 = h_idx_4 * h_idx_5 / pd;
  }
  H[0] = 0.0;
  H[6] = 0.0;
  H[12] = 0.0;
  H[18] = obj->pState[16];
  H[24] = 0.0;
  H[30] = 0.0;
  H[36] = 0.0;
  H[42] = 0.0;
  H[48] = 0.0;
  H[54] = 0.0;
  H[60] = 0.0;
  H[66] = 0.0;
  H[72] = 0.0;
  H[78] = 0.0;
  H[84] = 0.0;
  H[90] = obj->pState[4];
  H[1] = 0.0;
  H[7] = 0.0;
  H[13] = 0.0;
  H[19] = 0.0;
  H[25] = obj->pState[16];
  H[31] = 0.0;
  H[37] = 0.0;
  H[43] = 0.0;
  H[49] = 0.0;
  H[55] = 0.0;
  H[61] = 0.0;
  H[67] = 0.0;
  H[73] = 0.0;
  H[79] = 0.0;
  H[85] = 0.0;
  H[91] = obj->pState[5];
  H[2] = 0.0;
  H[8] = 0.0;
  H[14] = 0.0;
  H[20] = 0.0;
  H[26] = 0.0;
  H[32] = obj->pState[16];
  H[38] = 0.0;
  H[44] = 0.0;
  H[50] = 0.0;
  H[56] = 0.0;
  H[62] = 0.0;
  H[68] = 0.0;
  H[74] = 0.0;
  H[80] = 0.0;
  H[86] = 0.0;
  H[92] = obj->pState[6];
  for (i = 0; i < 16; i++) {
    H[6 * i + 3] = b_iv[i];
    H[6 * i + 4] = b_iv1[i];
    H[6 * i + 5] = b_iv2[i];
  }
  for (i = 0; i < 6; i++) {
    for (jp1j = 0; jp1j < 16; jp1j++) {
      jA = i + 6 * jp1j;
      y_tmp[jp1j + (i << 4)] = H[jA];
      d = 0.0;
      for (jAcol = 0; jAcol < 16; jAcol++) {
        d += H[i + 6 * jAcol] * obj->pStateCovariance[jAcol + (jp1j << 4)];
      }
      K[jA] = d;
    }
  }
  for (i = 0; i < 6; i++) {
    for (jp1j = 0; jp1j < 6; jp1j++) {
      d = 0.0;
      for (jAcol = 0; jAcol < 16; jAcol++) {
        d += K[i + 6 * jAcol] * y_tmp[jAcol + (jp1j << 4)];
      }
      jBcol = i + 6 * jp1j;
      resCov[jBcol] = d + R[jBcol];
    }
  }
  for (i = 0; i < 16; i++) {
    for (jp1j = 0; jp1j < 6; jp1j++) {
      d = 0.0;
      for (jAcol = 0; jAcol < 16; jAcol++) {
        d += obj->pStateCovariance[i + (jAcol << 4)] *
             y_tmp[jAcol + (jp1j << 4)];
      }
      K[i + (jp1j << 4)] = d;
    }
  }
  for (i = 0; i < 6; i++) {
    ipiv[i] = (signed char)(i + 1);
  }
  for (j = 0; j < 5; j++) {
    mmj_tmp = 4 - j;
    b_tmp = j * 7;
    jp1j = b_tmp + 2;
    jA = 6 - j;
    jBcol = 0;
    pd = fabs(resCov[b_tmp]);
    for (k = 2; k <= jA; k++) {
      h_idx_4 = fabs(resCov[(b_tmp + k) - 1]);
      if (h_idx_4 > pd) {
        jBcol = k - 1;
        pd = h_idx_4;
      }
    }
    if (resCov[b_tmp + jBcol] != 0.0) {
      if (jBcol != 0) {
        jA = j + jBcol;
        ipiv[j] = (signed char)(jA + 1);
        for (k = 0; k < 6; k++) {
          jAcol = j + k * 6;
          pd = resCov[jAcol];
          jBcol = jA + k * 6;
          resCov[jAcol] = resCov[jBcol];
          resCov[jBcol] = pd;
        }
      }
      i = (b_tmp - j) + 6;
      for (b_i = jp1j; b_i <= i; b_i++) {
        resCov[b_i - 1] /= resCov[b_tmp];
      }
    }
    jA = b_tmp;
    for (jBcol = 0; jBcol <= mmj_tmp; jBcol++) {
      pd = resCov[(b_tmp + jBcol * 6) + 6];
      if (pd != 0.0) {
        i = jA + 8;
        jp1j = (jA - j) + 12;
        for (jAcol = i; jAcol <= jp1j; jAcol++) {
          resCov[jAcol - 1] += resCov[((b_tmp + jAcol) - jA) - 7] * -pd;
        }
      }
      jA += 6;
    }
  }
  for (j = 0; j < 6; j++) {
    jBcol = (j << 4) - 1;
    jAcol = 6 * j;
    for (k = 0; k < j; k++) {
      jA = k << 4;
      d = resCov[k + jAcol];
      if (d != 0.0) {
        for (b_i = 0; b_i < 16; b_i++) {
          jp1j = (b_i + jBcol) + 1;
          K[jp1j] -= d * K[b_i + jA];
        }
      }
    }
    pd = 1.0 / resCov[j + jAcol];
    for (b_i = 0; b_i < 16; b_i++) {
      jp1j = (b_i + jBcol) + 1;
      K[jp1j] *= pd;
    }
  }
  for (j = 5; j >= 0; j--) {
    jBcol = (j << 4) - 1;
    jAcol = 6 * j - 1;
    i = j + 2;
    for (k = i; k < 7; k++) {
      jA = (k - 1) << 4;
      d = resCov[k + jAcol];
      if (d != 0.0) {
        for (b_i = 0; b_i < 16; b_i++) {
          jp1j = (b_i + jBcol) + 1;
          K[jp1j] -= d * K[b_i + jA];
        }
      }
    }
  }
  for (j = 4; j >= 0; j--) {
    i1 = ipiv[j];
    if (i1 != j + 1) {
      for (b_i = 0; b_i < 16; b_i++) {
        jAcol = b_i + (j << 4);
        pd = K[jAcol];
        jp1j = b_i + ((i1 - 1) << 4);
        K[jAcol] = K[jp1j];
        K[jp1j] = pd;
      }
    }
  }
  for (i = 0; i < 16; i++) {
    for (jp1j = 0; jp1j < 16; jp1j++) {
      d = 0.0;
      for (jAcol = 0; jAcol < 6; jAcol++) {
        d += K[i + (jAcol << 4)] * H[jAcol + 6 * jp1j];
      }
      b_y_tmp[i + (jp1j << 4)] = d;
    }
  }
  memset(&b_I[0], 0, 256U * sizeof(signed char));
  for (k = 0; k < 16; k++) {
    b_I[k + (k << 4)] = 1;
  }
  memset(&c_I[0], 0, 256U * sizeof(signed char));
  for (k = 0; k < 16; k++) {
    c_I[k + (k << 4)] = 1;
  }
  for (i = 0; i < 256; i++) {
    d_I[i] = (double)c_I[i] - b_y_tmp[i];
  }
  for (i = 0; i < 16; i++) {
    for (jp1j = 0; jp1j < 16; jp1j++) {
      d = 0.0;
      for (jAcol = 0; jAcol < 16; jAcol++) {
        d += d_I[i + (jAcol << 4)] * obj->pStateCovariance[jAcol + (jp1j << 4)];
      }
      e_I[i + (jp1j << 4)] = d;
    }
  }
  for (i = 0; i < 16; i++) {
    for (jp1j = 0; jp1j < 16; jp1j++) {
      jA = i + (jp1j << 4);
      d_I[jp1j + (i << 4)] = (double)b_I[jA] - b_y_tmp[jA];
    }
    for (jp1j = 0; jp1j < 6; jp1j++) {
      d = 0.0;
      for (jAcol = 0; jAcol < 6; jAcol++) {
        d += K[i + (jAcol << 4)] * R[jAcol + 6 * jp1j];
      }
      y_tmp[i + (jp1j << 4)] = d;
    }
  }
  for (i = 0; i < 16; i++) {
    for (jp1j = 0; jp1j < 16; jp1j++) {
      d = 0.0;
      for (jAcol = 0; jAcol < 16; jAcol++) {
        d += e_I[i + (jAcol << 4)] * d_I[jAcol + (jp1j << 4)];
      }
      jA = i + (jp1j << 4);
      f_I[jA] = d;
      d = 0.0;
      for (jAcol = 0; jAcol < 6; jAcol++) {
        jBcol = jAcol << 4;
        d += y_tmp[i + jBcol] * K[jp1j + jBcol];
      }
      b_y_tmp[jA] = d;
    }
  }
  for (i = 0; i < 256; i++) {
    obj->pStateCovariance[i] = f_I[i] + b_y_tmp[i];
  }
  z[0] = voPos[0] - h_idx_0;
  z[3] = deltaQ_a;
  z[1] = voPos[1] - h_idx_1;
  z[4] = d_idx_1;
  z[2] = voPos[2] - h_idx_6;
  z[5] = d_idx_2;
  for (i = 0; i < 16; i++) {
    d = 0.0;
    for (jp1j = 0; jp1j < 6; jp1j++) {
      d += K[i + (jp1j << 4)] * z[jp1j];
    }
    b_K[i] = d;
  }
  c_ErrorStateIMUGPSFuserBase_inj(obj, b_K);
  for (i = 0; i < 16; i++) {
    for (jp1j = 0; jp1j < 16; jp1j++) {
      d = 0.0;
      for (jAcol = 0; jAcol < 16; jAcol++) {
        d += (double)iv2[i + (jAcol << 4)] *
             obj->pStateCovariance[jAcol + (jp1j << 4)];
      }
      b_y_tmp[i + (jp1j << 4)] = d;
    }
  }
  for (i = 0; i < 16; i++) {
    for (jp1j = 0; jp1j < 16; jp1j++) {
      jAcol = jp1j << 4;
      jBcol = i + jAcol;
      obj->pStateCovariance[jBcol] = 0.0;
      for (jA = 0; jA < 16; jA++) {
        obj->pStateCovariance[jBcol] +=
            b_y_tmp[i + (jA << 4)] * (double)iv2[jA + jAcol];
      }
    }
  }
}

/*
 * Arguments    : insfilterErrorState *obj
 *                const double deltaX[16]
 * Return Type  : void
 */
void c_ErrorStateIMUGPSFuserBase_inj(insfilterErrorState *obj,
                                     const double deltaX[16])
{
  double b_quat_a[17];
  double quatErr_a;
  double quatErr_b;
  double quatErr_c;
  double quatErr_d;
  double quat_a;
  double quat_b;
  double quat_c;
  double st;
  double theta;
  quatErr_a = 1.0;
  quatErr_b = 0.0;
  quatErr_c = 0.0;
  quatErr_d = 0.0;
  theta = sqrt((deltaX[0] * deltaX[0] + deltaX[1] * deltaX[1]) +
               deltaX[2] * deltaX[2]);
  st = sin(theta / 2.0);
  if (theta != 0.0) {
    quatErr_a = cos(theta / 2.0);
    quatErr_b = deltaX[0] / theta * st;
    quatErr_c = deltaX[1] / theta * st;
    quatErr_d = deltaX[2] / theta * st;
  }
  quat_a = ((obj->pState[0] * quatErr_a - obj->pState[1] * quatErr_b) -
            obj->pState[2] * quatErr_c) -
           obj->pState[3] * quatErr_d;
  quat_b = ((obj->pState[0] * quatErr_b + obj->pState[1] * quatErr_a) +
            obj->pState[2] * quatErr_d) -
           obj->pState[3] * quatErr_c;
  quat_c = ((obj->pState[0] * quatErr_c - obj->pState[1] * quatErr_d) +
            obj->pState[2] * quatErr_a) +
           obj->pState[3] * quatErr_b;
  theta = ((obj->pState[0] * quatErr_d + obj->pState[1] * quatErr_c) -
           obj->pState[2] * quatErr_b) +
          obj->pState[3] * quatErr_a;
  st = sqrt(((quat_a * quat_a + quat_b * quat_b) + quat_c * quat_c) +
            theta * theta);
  quat_a /= st;
  quat_b /= st;
  quat_c /= st;
  theta /= st;
  b_quat_a[0] = quat_a;
  b_quat_a[1] = quat_b;
  b_quat_a[2] = quat_c;
  b_quat_a[3] = theta;
  b_quat_a[4] = deltaX[3] + obj->pState[4];
  b_quat_a[7] = deltaX[6] + obj->pState[7];
  b_quat_a[10] = deltaX[9] + obj->pState[10];
  b_quat_a[13] = deltaX[12] + obj->pState[13];
  b_quat_a[5] = deltaX[4] + obj->pState[5];
  b_quat_a[8] = deltaX[7] + obj->pState[8];
  b_quat_a[11] = deltaX[10] + obj->pState[11];
  b_quat_a[14] = deltaX[13] + obj->pState[14];
  b_quat_a[6] = deltaX[5] + obj->pState[6];
  b_quat_a[9] = deltaX[8] + obj->pState[9];
  b_quat_a[12] = deltaX[11] + obj->pState[12];
  b_quat_a[15] = deltaX[14] + obj->pState[15];
  b_quat_a[16] = deltaX[15] + obj->pState[16];
  memcpy(&obj->pState[0], &b_quat_a[0], 17U * sizeof(double));
}

/*
 * Arguments    : insfilterErrorState *obj
 *                double accelMeas[3]
 *                const double gyroMeas[3]
 * Return Type  : void
 */
void c_ErrorStateIMUGPSFuserBase_pre(insfilterErrorState *obj,
                                     double accelMeas[3],
                                     const double gyroMeas[3])
{
  static const signed char b_iv[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 1, 0, 0, 0, 0, 0, 0};
  static const signed char b_iv1[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 1, 0, 0, 0, 0, 0};
  static const signed char b_iv2[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 1, 0, 0, 0, 0};
  static const signed char iv3[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                      0, 0, 0, 0, 1, 0, 0, 0};
  static const signed char iv4[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                      0, 0, 0, 0, 0, 1, 0, 0};
  static const signed char iv5[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                      0, 0, 0, 0, 0, 0, 1, 0};
  double F[256];
  double b_F[256];
  double c_F[256];
  double G[192];
  double b_G[192];
  double U[144];
  double x[17];
  double b_d[9];
  double c_d[9];
  double d_d[9];
  double e_d[9];
  double F_tmp;
  double F_tmp_tmp;
  double a;
  double b_F_tmp;
  double b_F_tmp_tmp;
  double b_n_tmp;
  double b_n_tmp_tmp;
  double c_F_tmp;
  double c_n_tmp;
  double d;
  double d_F_tmp;
  double dt_tmp_tmp;
  double e_F_tmp;
  double n;
  double n_tmp;
  double n_tmp_tmp;
  double newQuat_a;
  double newQuat_b;
  double newQuat_c;
  double o_b;
  double o_c;
  double qo_a;
  double qo_b;
  double qo_c;
  double qo_d;
  double varargin_1_idx_2;
  int U_tmp;
  int b_U_tmp;
  int c_U_tmp;
  int f_F_tmp;
  int i;
  accelMeas[0] = -accelMeas[0];
  accelMeas[1] = -accelMeas[1];
  accelMeas[2] = -accelMeas[2];
  memcpy(&x[0], &obj->pState[0], 17U * sizeof(double));
  dt_tmp_tmp = 1.0 / obj->IMUSampleRate;
  a = 0.5 * dt_tmp_tmp;
  n = a * (gyroMeas[0] - x[10]);
  varargin_1_idx_2 = a * (gyroMeas[1] - x[11]);
  a *= gyroMeas[2] - x[12];
  newQuat_a = ((x[0] - x[1] * n) - x[2] * varargin_1_idx_2) - x[3] * a;
  newQuat_b = ((x[0] * n + x[1]) + x[2] * a) - varargin_1_idx_2 * x[3];
  newQuat_c = ((x[0] * varargin_1_idx_2 - x[1] * a) + x[2]) + n * x[3];
  a = ((x[0] * a + x[1] * varargin_1_idx_2) - n * x[2]) + x[3];
  n_tmp_tmp = x[0] * x[0];
  b_n_tmp_tmp = x[1] * x[1];
  n_tmp = n_tmp_tmp + b_n_tmp_tmp;
  b_n_tmp = x[2] * x[2];
  c_n_tmp = x[3] * x[3];
  n = sqrt((n_tmp + b_n_tmp) + c_n_tmp);
  qo_a = x[0] / n;
  qo_b = x[1] / n;
  qo_c = x[2] / n;
  qo_d = x[3] / n;
  n = sqrt(((newQuat_a * newQuat_a + newQuat_b * newQuat_b) +
            newQuat_c * newQuat_c) +
           a * a);
  newQuat_a /= n;
  newQuat_b /= n;
  newQuat_c /= n;
  a /= n;
  obj->pState[0] = newQuat_a;
  obj->pState[1] = newQuat_b;
  obj->pState[2] = newQuat_c;
  obj->pState[3] = a;
  n = accelMeas[0] - x[13];
  obj->pState[4] = x[4] + x[7] * dt_tmp_tmp;
  varargin_1_idx_2 = accelMeas[1] - x[14];
  obj->pState[5] = x[5] + x[8] * dt_tmp_tmp;
  a = accelMeas[2] - x[15];
  obj->pState[6] = x[6] + x[9] * dt_tmp_tmp;
  newQuat_c = ((qo_a * 0.0 - qo_b * n) - qo_c * varargin_1_idx_2) - qo_d * a;
  o_b = ((qo_a * n + qo_b * 0.0) + qo_c * a) - qo_d * varargin_1_idx_2;
  o_c = ((qo_a * varargin_1_idx_2 - qo_b * a) + qo_c * 0.0) + qo_d * n;
  a = ((qo_a * a + qo_b * varargin_1_idx_2) - qo_c * n) + qo_d * 0.0;
  qo_b = -qo_b;
  qo_c = -qo_c;
  qo_d = -qo_d;
  obj->pState[7] =
      x[7] +
      (((newQuat_c * qo_b + o_b * qo_a) + o_c * qo_d) - a * qo_c) * dt_tmp_tmp;
  obj->pState[10] = x[10];
  obj->pState[13] = x[13];
  obj->pState[8] =
      x[8] +
      (((newQuat_c * qo_c - o_b * qo_d) + o_c * qo_a) + a * qo_b) * dt_tmp_tmp;
  obj->pState[11] = x[11];
  obj->pState[14] = x[14];
  obj->pState[9] =
      x[9] +
      ((((newQuat_c * qo_d + o_b * qo_c) - o_c * qo_b) + a * qo_a) - 9.81) *
          dt_tmp_tmp;
  obj->pState[12] = x[12];
  obj->pState[15] = x[15];
  obj->pState[16] = x[16];
  d = 1.0 / obj->IMUSampleRate;
  F[0] = 1.0;
  F[16] = 0.0;
  F[32] = 0.0;
  F[48] = 0.0;
  F[64] = 0.0;
  F[80] = 0.0;
  F[96] = 0.0;
  F[112] = 0.0;
  F[128] = 0.0;
  qo_b = (n_tmp - b_n_tmp) - c_n_tmp;
  qo_c = -d * qo_b;
  F[144] = qo_c;
  F_tmp = 2.0 * x[0] * x[3];
  qo_d = 2.0 * x[1] * x[2];
  F_tmp_tmp = F_tmp - qo_d;
  dt_tmp_tmp = d * F_tmp_tmp;
  F[160] = dt_tmp_tmp;
  b_F_tmp = 2.0 * x[0] * x[2];
  n_tmp = 2.0 * x[1] * x[3];
  b_F_tmp_tmp = b_F_tmp + n_tmp;
  c_F_tmp = -d * b_F_tmp_tmp;
  F[176] = c_F_tmp;
  F[192] = 0.0;
  F[208] = 0.0;
  F[224] = 0.0;
  F[240] = 0.0;
  F[1] = 0.0;
  F[17] = 1.0;
  F[33] = 0.0;
  F[49] = 0.0;
  F[65] = 0.0;
  F[81] = 0.0;
  F[97] = 0.0;
  F[113] = 0.0;
  F[129] = 0.0;
  F_tmp += qo_d;
  qo_d = -d * F_tmp;
  F[145] = qo_d;
  d_F_tmp = n_tmp_tmp - b_n_tmp_tmp;
  n_tmp_tmp = (d_F_tmp + b_n_tmp) - c_n_tmp;
  newQuat_c = -d * n_tmp_tmp;
  F[161] = newQuat_c;
  e_F_tmp = 2.0 * x[0] * x[1];
  varargin_1_idx_2 = 2.0 * x[2] * x[3];
  b_n_tmp_tmp = e_F_tmp - varargin_1_idx_2;
  qo_a = d * b_n_tmp_tmp;
  F[177] = qo_a;
  F[193] = 0.0;
  F[209] = 0.0;
  F[225] = 0.0;
  F[241] = 0.0;
  F[2] = 0.0;
  F[18] = 0.0;
  F[34] = 1.0;
  F[50] = 0.0;
  F[66] = 0.0;
  F[82] = 0.0;
  F[98] = 0.0;
  F[114] = 0.0;
  F[130] = 0.0;
  b_F_tmp -= n_tmp;
  n_tmp = d * b_F_tmp;
  F[146] = n_tmp;
  e_F_tmp += varargin_1_idx_2;
  varargin_1_idx_2 = -d * e_F_tmp;
  F[162] = varargin_1_idx_2;
  d_F_tmp = (d_F_tmp - b_n_tmp) + c_n_tmp;
  a = -d * d_F_tmp;
  F[178] = a;
  F[194] = 0.0;
  F[210] = 0.0;
  F[226] = 0.0;
  F[242] = 0.0;
  F[3] = 0.0;
  F[19] = 0.0;
  F[35] = 0.0;
  F[51] = 1.0;
  F[67] = 0.0;
  F[83] = 0.0;
  F[99] = d;
  F[115] = 0.0;
  F[131] = 0.0;
  F[147] = 0.0;
  F[163] = 0.0;
  F[179] = 0.0;
  F[195] = 0.0;
  F[211] = 0.0;
  F[227] = 0.0;
  F[243] = 0.0;
  F[4] = 0.0;
  F[20] = 0.0;
  F[36] = 0.0;
  F[52] = 0.0;
  F[68] = 1.0;
  F[84] = 0.0;
  F[100] = 0.0;
  F[116] = d;
  F[132] = 0.0;
  F[148] = 0.0;
  F[164] = 0.0;
  F[180] = 0.0;
  F[196] = 0.0;
  F[212] = 0.0;
  F[228] = 0.0;
  F[244] = 0.0;
  F[5] = 0.0;
  F[21] = 0.0;
  F[37] = 0.0;
  F[53] = 0.0;
  F[69] = 0.0;
  F[85] = 1.0;
  F[101] = 0.0;
  F[117] = 0.0;
  F[133] = d;
  F[149] = 0.0;
  F[165] = 0.0;
  F[181] = 0.0;
  F[197] = 0.0;
  F[213] = 0.0;
  F[229] = 0.0;
  F[245] = 0.0;
  F[6] = 0.0;
  n = x[14] - accelMeas[1];
  o_b = x[13] - accelMeas[0];
  o_c = x[15] - accelMeas[2];
  newQuat_a = (o_c * d_F_tmp - o_b * b_F_tmp) + n * e_F_tmp;
  F[22] = -d * newQuat_a;
  newQuat_b = (n * n_tmp_tmp + o_b * F_tmp) - o_c * b_n_tmp_tmp;
  F[38] = d * newQuat_b;
  F[54] = 0.0;
  F[70] = 0.0;
  F[86] = 0.0;
  F[102] = 1.0;
  F[118] = 0.0;
  F[134] = 0.0;
  F[150] = 0.0;
  F[166] = 0.0;
  F[182] = 0.0;
  F[198] = qo_c;
  F[214] = dt_tmp_tmp;
  F[230] = c_F_tmp;
  F[246] = 0.0;
  F[7] = d * newQuat_a;
  F[23] = 0.0;
  qo_c = (o_b * qo_b - n * F_tmp_tmp) + o_c * b_F_tmp_tmp;
  F[39] = -d * qo_c;
  F[55] = 0.0;
  F[71] = 0.0;
  F[87] = 0.0;
  F[103] = 0.0;
  F[119] = 1.0;
  F[135] = 0.0;
  F[151] = 0.0;
  F[167] = 0.0;
  F[183] = 0.0;
  F[199] = qo_d;
  F[215] = newQuat_c;
  F[231] = qo_a;
  F[247] = 0.0;
  F[8] = -d * newQuat_b;
  F[24] = d * qo_c;
  F[40] = 0.0;
  F[56] = 0.0;
  F[72] = 0.0;
  F[88] = 0.0;
  F[104] = 0.0;
  F[120] = 0.0;
  F[136] = 1.0;
  F[152] = 0.0;
  F[168] = 0.0;
  F[184] = 0.0;
  F[200] = n_tmp;
  F[216] = varargin_1_idx_2;
  F[232] = a;
  F[248] = 0.0;
  for (i = 0; i < 16; i++) {
    f_F_tmp = i << 4;
    F[f_F_tmp + 9] = b_iv[i];
    F[f_F_tmp + 10] = b_iv1[i];
    F[f_F_tmp + 11] = b_iv2[i];
    F[f_F_tmp + 12] = iv3[i];
    F[f_F_tmp + 13] = iv4[i];
    F[f_F_tmp + 14] = iv5[i];
    F[f_F_tmp + 15] = iv1[i];
  }
  d = 1.0 / obj->IMUSampleRate;
  G[0] = 0.0;
  G[16] = 0.0;
  G[32] = 0.0;
  qo_a = d * qo_b;
  G[48] = qo_a;
  varargin_1_idx_2 = -d * F_tmp_tmp;
  G[64] = varargin_1_idx_2;
  newQuat_b = d * b_F_tmp_tmp;
  G[80] = newQuat_b;
  G[96] = 0.0;
  G[112] = 0.0;
  G[128] = 0.0;
  G[144] = 0.0;
  G[160] = 0.0;
  G[176] = 0.0;
  G[1] = 0.0;
  G[17] = 0.0;
  G[33] = 0.0;
  newQuat_a = d * F_tmp;
  G[49] = newQuat_a;
  o_c = d * n_tmp_tmp;
  G[65] = o_c;
  o_b = -d * b_n_tmp_tmp;
  G[81] = o_b;
  G[97] = 0.0;
  G[113] = 0.0;
  G[129] = 0.0;
  G[145] = 0.0;
  G[161] = 0.0;
  G[177] = 0.0;
  G[2] = 0.0;
  G[18] = 0.0;
  G[34] = 0.0;
  newQuat_c = -d * b_F_tmp;
  G[50] = newQuat_c;
  n = d * e_F_tmp;
  G[66] = n;
  a = d * d_F_tmp;
  G[82] = a;
  G[98] = 0.0;
  G[114] = 0.0;
  G[130] = 0.0;
  G[146] = 0.0;
  G[162] = 0.0;
  G[178] = 0.0;
  G[6] = qo_a;
  G[22] = varargin_1_idx_2;
  G[38] = newQuat_b;
  G[54] = 0.0;
  G[70] = 0.0;
  G[86] = 0.0;
  G[102] = 0.0;
  G[118] = 0.0;
  G[134] = 0.0;
  G[150] = 0.0;
  G[166] = 0.0;
  G[182] = 0.0;
  G[7] = newQuat_a;
  G[23] = o_c;
  G[39] = o_b;
  G[55] = 0.0;
  G[71] = 0.0;
  G[87] = 0.0;
  G[103] = 0.0;
  G[119] = 0.0;
  G[135] = 0.0;
  G[151] = 0.0;
  G[167] = 0.0;
  G[183] = 0.0;
  G[8] = newQuat_c;
  G[24] = n;
  G[40] = a;
  G[56] = 0.0;
  G[72] = 0.0;
  G[88] = 0.0;
  G[104] = 0.0;
  G[120] = 0.0;
  G[136] = 0.0;
  G[152] = 0.0;
  G[168] = 0.0;
  G[184] = 0.0;
  G[9] = 0.0;
  G[25] = 0.0;
  G[41] = 0.0;
  G[57] = 0.0;
  G[73] = 0.0;
  G[89] = 0.0;
  G[105] = 0.0;
  G[121] = 0.0;
  G[137] = 0.0;
  G[153] = d;
  G[169] = 0.0;
  G[185] = 0.0;
  G[10] = 0.0;
  G[26] = 0.0;
  G[42] = 0.0;
  G[58] = 0.0;
  G[74] = 0.0;
  G[90] = 0.0;
  G[106] = 0.0;
  G[122] = 0.0;
  G[138] = 0.0;
  G[154] = 0.0;
  G[170] = d;
  G[186] = 0.0;
  G[11] = 0.0;
  G[27] = 0.0;
  G[43] = 0.0;
  G[59] = 0.0;
  G[75] = 0.0;
  G[91] = 0.0;
  G[107] = 0.0;
  G[123] = 0.0;
  G[139] = 0.0;
  G[155] = 0.0;
  G[171] = 0.0;
  G[187] = d;
  G[12] = 0.0;
  G[28] = 0.0;
  G[44] = 0.0;
  G[60] = 0.0;
  G[76] = 0.0;
  G[92] = 0.0;
  G[108] = d;
  G[124] = 0.0;
  G[140] = 0.0;
  G[156] = 0.0;
  G[172] = 0.0;
  G[188] = 0.0;
  G[13] = 0.0;
  G[29] = 0.0;
  G[45] = 0.0;
  G[61] = 0.0;
  G[77] = 0.0;
  G[93] = 0.0;
  G[109] = 0.0;
  G[125] = d;
  G[141] = 0.0;
  G[157] = 0.0;
  G[173] = 0.0;
  G[189] = 0.0;
  G[14] = 0.0;
  G[30] = 0.0;
  G[46] = 0.0;
  G[62] = 0.0;
  G[78] = 0.0;
  G[94] = 0.0;
  G[110] = 0.0;
  G[126] = 0.0;
  G[142] = d;
  G[158] = 0.0;
  G[174] = 0.0;
  G[190] = 0.0;
  for (i = 0; i < 12; i++) {
    f_F_tmp = i << 4;
    G[f_F_tmp + 3] = 0.0;
    G[f_F_tmp + 4] = 0.0;
    G[f_F_tmp + 5] = 0.0;
    G[f_F_tmp + 15] = 0.0;
  }
  memset(&b_d[0], 0, 9U * sizeof(double));
  b_d[0] = obj->AccelerometerNoise[0];
  b_d[4] = obj->AccelerometerNoise[1];
  b_d[8] = obj->AccelerometerNoise[2];
  memset(&c_d[0], 0, 9U * sizeof(double));
  c_d[0] = obj->GyroscopeNoise[0];
  c_d[4] = obj->GyroscopeNoise[1];
  c_d[8] = obj->GyroscopeNoise[2];
  memset(&d_d[0], 0, 9U * sizeof(double));
  d_d[0] = obj->AccelerometerBiasNoise[0];
  d_d[4] = obj->AccelerometerBiasNoise[1];
  d_d[8] = obj->AccelerometerBiasNoise[2];
  memset(&e_d[0], 0, 9U * sizeof(double));
  e_d[0] = obj->GyroscopeBiasNoise[0];
  e_d[4] = obj->GyroscopeBiasNoise[1];
  e_d[8] = obj->GyroscopeBiasNoise[2];
  memset(&U[0], 0, 144U * sizeof(double));
  for (i = 0; i < 3; i++) {
    U[12 * i] = b_d[3 * i];
    f_F_tmp = 12 * (i + 3);
    U[f_F_tmp + 3] = c_d[3 * i];
    U_tmp = 12 * (i + 6);
    U[U_tmp + 6] = d_d[3 * i];
    b_U_tmp = 12 * (i + 9);
    U[b_U_tmp + 9] = e_d[3 * i];
    c_U_tmp = 3 * i + 1;
    U[12 * i + 1] = b_d[c_U_tmp];
    U[f_F_tmp + 4] = c_d[c_U_tmp];
    U[U_tmp + 7] = d_d[c_U_tmp];
    U[b_U_tmp + 10] = e_d[c_U_tmp];
    c_U_tmp = 3 * i + 2;
    U[12 * i + 2] = b_d[c_U_tmp];
    U[f_F_tmp + 5] = c_d[c_U_tmp];
    U[U_tmp + 8] = d_d[c_U_tmp];
    U[b_U_tmp + 11] = e_d[c_U_tmp];
  }
  for (i = 0; i < 144; i++) {
    U[i] *= obj->IMUSampleRate;
  }
  for (i = 0; i < 16; i++) {
    for (U_tmp = 0; U_tmp < 16; U_tmp++) {
      d = 0.0;
      for (b_U_tmp = 0; b_U_tmp < 16; b_U_tmp++) {
        d += F[i + (b_U_tmp << 4)] *
             obj->pStateCovariance[b_U_tmp + (U_tmp << 4)];
      }
      c_F[i + (U_tmp << 4)] = d;
    }
    for (U_tmp = 0; U_tmp < 12; U_tmp++) {
      d = 0.0;
      for (b_U_tmp = 0; b_U_tmp < 12; b_U_tmp++) {
        d += G[i + (b_U_tmp << 4)] * U[b_U_tmp + 12 * U_tmp];
      }
      b_G[i + (U_tmp << 4)] = d;
    }
    for (U_tmp = 0; U_tmp < 16; U_tmp++) {
      d = 0.0;
      for (b_U_tmp = 0; b_U_tmp < 16; b_U_tmp++) {
        f_F_tmp = b_U_tmp << 4;
        d += c_F[i + f_F_tmp] * F[U_tmp + f_F_tmp];
      }
      b_F[i + (U_tmp << 4)] = d;
    }
  }
  for (i = 0; i < 16; i++) {
    for (U_tmp = 0; U_tmp < 16; U_tmp++) {
      d = 0.0;
      for (b_U_tmp = 0; b_U_tmp < 12; b_U_tmp++) {
        f_F_tmp = b_U_tmp << 4;
        d += b_G[i + f_F_tmp] * G[U_tmp + f_F_tmp];
      }
      F[i + (U_tmp << 4)] = d;
    }
  }
  for (i = 0; i < 256; i++) {
    obj->pStateCovariance[i] = b_F[i] + F[i];
  }
}

/*
 * File trailer for ErrorStateIMUGPSFuserBase.c
 *
 * [EOF]
 */
