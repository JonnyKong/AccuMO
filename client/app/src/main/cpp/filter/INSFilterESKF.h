/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: INSFilterESKF.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 02-Mar-2022 23:31:18
 */

#ifndef INSFILTERESKF_H
#define INSFILTERESKF_H

/* Include Files */
#include "insfilterErrorStateCoder_internal_types.h"
#include "rtwtypes.h"
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
void INSFilterESKF_pose(const insfilterErrorState *obj, double pos[3],
                        double *orient_a, double *orient_b, double *orient_c,
                        double *orient_d);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for INSFilterESKF.h
 *
 * [EOF]
 */
