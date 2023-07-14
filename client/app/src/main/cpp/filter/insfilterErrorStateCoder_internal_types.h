/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: insfilterErrorStateCoder_internal_types.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 02-Mar-2022 23:31:18
 */

#ifndef INSFILTERERRORSTATECODER_INTERNAL_TYPES_H
#define INSFILTERERRORSTATECODER_INTERNAL_TYPES_H

/* Include Files */
#include "insfilterErrorStateCoder_types.h"
#include "rtwtypes.h"

/* Type Definitions */
#ifndef typedef_insfilterErrorState
#define typedef_insfilterErrorState
typedef struct {
  double ReferenceLocation[3];
  double IMUSampleRate;
  double AccelerometerNoise[3];
  double GyroscopeNoise[3];
  double AccelerometerBiasNoise[3];
  double GyroscopeBiasNoise[3];
  double pState[17];
  double pStateCovariance[256];
} insfilterErrorState;
#endif /* typedef_insfilterErrorState */

#endif
/*
 * File trailer for insfilterErrorStateCoder_internal_types.h
 *
 * [EOF]
 */
