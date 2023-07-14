/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: BasicESKF.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 02-Mar-2022 23:31:18
 */

/* Include Files */
#include "BasicESKF.h"
#include "ErrorStateIMUGPSFuserBase.h"
#include "insfilterErrorStateCoder_data.h"
#include "insfilterErrorStateCoder_internal_types.h"
#include "rt_nonfinite.h"
#include <string.h>

/* Function Definitions */
/*
 * Arguments    : insfilterErrorState *obj
 * Return Type  : void
 */
void BasicESKF_correct(insfilterErrorState *obj)
{
  double b_W[256];
  double b_obj[256];
  double W[16];
  double H;
  double d;
  int i;
  int i1;
  int i2;
  int i3;
  int obj_tmp;
  H = 0.0;
  for (i = 0; i < 16; i++) {
    d = 0.0;
    for (i1 = 0; i1 < 16; i1++) {
      d += (double)iv1[i1] * obj->pStateCovariance[i1 + (i << 4)];
    }
    H += d * (double)iv1[i];
  }
  for (i = 0; i < 16; i++) {
    d = 0.0;
    for (i1 = 0; i1 < 16; i1++) {
      d += obj->pStateCovariance[i + (i1 << 4)] * (double)iv1[i1];
    }
    W[i] = d / (H + 1.0E-9);
  }
  for (i = 0; i < 16; i++) {
    for (i1 = 0; i1 < 16; i1++) {
      b_W[i1 + (i << 4)] = W[i1] * (double)iv1[i];
    }
  }
  for (i = 0; i < 16; i++) {
    for (i1 = 0; i1 < 16; i1++) {
      d = 0.0;
      for (i2 = 0; i2 < 16; i2++) {
        d += b_W[i + (i2 << 4)] * obj->pStateCovariance[i2 + (i1 << 4)];
      }
      obj_tmp = i + (i1 << 4);
      b_obj[obj_tmp] = obj->pStateCovariance[obj_tmp] - d;
    }
  }
  memcpy(&obj->pStateCovariance[0], &b_obj[0], 256U * sizeof(double));
  d = 1.0 - obj->pState[16];
  for (i = 0; i < 16; i++) {
    W[i] *= d;
  }
  c_ErrorStateIMUGPSFuserBase_inj(obj, W);
  for (i = 0; i < 16; i++) {
    for (i1 = 0; i1 < 16; i1++) {
      d = 0.0;
      for (i2 = 0; i2 < 16; i2++) {
        d += (double)iv2[i + (i2 << 4)] * obj->pStateCovariance[i2 + (i1 << 4)];
      }
      b_W[i + (i1 << 4)] = d;
    }
  }
  for (i = 0; i < 16; i++) {
    for (i1 = 0; i1 < 16; i1++) {
      i2 = i1 << 4;
      obj_tmp = i + i2;
      obj->pStateCovariance[obj_tmp] = 0.0;
      for (i3 = 0; i3 < 16; i3++) {
        obj->pStateCovariance[obj_tmp] +=
            b_W[i + (i3 << 4)] * (double)iv2[i3 + i2];
      }
    }
  }
}

/*
 * File trailer for BasicESKF.c
 *
 * [EOF]
 */
