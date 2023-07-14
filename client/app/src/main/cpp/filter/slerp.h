/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: slerp.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 02-Mar-2022 23:31:18
 */

#ifndef SLERP_H
#define SLERP_H

/* Include Files */
#include "rtwtypes.h"
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
void quaternionBase_slerp(double q1_a, double q1_b, double q1_c, double q1_d,
                          double q2_a, double q2_b, double q2_c, double q2_d,
                          double t, double *qo_a, double *qo_b, double *qo_c,
                          double *qo_d);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for slerp.h
 *
 * [EOF]
 */
