/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: ErrorStateIMUGPSFuserBase.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 02-Mar-2022 23:31:18
 */

#ifndef ERRORSTATEIMUGPSFUSERBASE_H
#define ERRORSTATEIMUGPSFUSERBASE_H

/* Include Files */
#include "insfilterErrorStateCoder_internal_types.h"
#include "rtwtypes.h"
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
void c_ErrorStateIMUGPSFuserBase_fus(insfilterErrorState *obj,
                                     const double voPos[3],
                                     const double voOrientIn[9]);

void c_ErrorStateIMUGPSFuserBase_inj(insfilterErrorState *obj,
                                     const double deltaX[16]);

void c_ErrorStateIMUGPSFuserBase_pre(insfilterErrorState *obj,
                                     double accelMeas[3],
                                     const double gyroMeas[3]);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for ErrorStateIMUGPSFuserBase.h
 *
 * [EOF]
 */
