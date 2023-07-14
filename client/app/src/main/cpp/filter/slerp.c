/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: slerp.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 02-Mar-2022 23:31:18
 */

/* Include Files */
#include "slerp.h"
#include "rt_nonfinite.h"
#include "rt_nonfinite.h"
#include <math.h>

/* Function Definitions */
/*
 * Arguments    : double q1_a
 *                double q1_b
 *                double q1_c
 *                double q1_d
 *                double q2_a
 *                double q2_b
 *                double q2_c
 *                double q2_d
 *                double t
 *                double *qo_a
 *                double *qo_b
 *                double *qo_c
 *                double *qo_d
 * Return Type  : void
 */
void quaternionBase_slerp(double q1_a, double q1_b, double q1_c, double q1_d,
                          double q2_a, double q2_b, double q2_c, double q2_d,
                          double t, double *qo_a, double *qo_b, double *qo_c,
                          double *qo_d)
{
  double n;
  double q1n_a;
  double q1n_b;
  double q1n_c;
  double q1n_d;
  double q2n_a;
  double q2n_b;
  double q2n_c;
  double q2n_d;
  double sinv;
  double y;
  n = sqrt(((q1_a * q1_a + q1_b * q1_b) + q1_c * q1_c) + q1_d * q1_d);
  q1n_a = q1_a / n;
  q1n_b = q1_b / n;
  q1n_c = q1_c / n;
  q1n_d = q1_d / n;
  n = sqrt(((q2_a * q2_a + q2_b * q2_b) + q2_c * q2_c) + q2_d * q2_d);
  q2n_a = q2_a / n;
  q2n_b = q2_b / n;
  q2n_c = q2_c / n;
  q2n_d = q2_d / n;
  n = ((q1n_a * q2n_a + q1n_b * q2n_b) + q1n_c * q2n_c) + q1n_d * q2n_d;
  if (n < 0.0) {
    q2n_a = -q2n_a;
    q2n_b = -q2n_b;
    q2n_c = -q2n_c;
    q2n_d = -q2n_d;
    n = -n;
  }
  if (n > 1.0) {
    n = 1.0;
  }
  n = acos(n);
  sinv = 1.0 / sin(n);
  y = sin((1.0 - t) * n);
  n = sin(t * n);
  *qo_a = sinv * (y * q1n_a + n * q2n_a);
  *qo_b = sinv * (y * q1n_b + n * q2n_b);
  *qo_c = sinv * (y * q1n_c + n * q2n_c);
  *qo_d = sinv * (y * q1n_d + n * q2n_d);
  if (rtIsInf(sinv)) {
    *qo_a = q1n_a;
    *qo_b = q1n_b;
    *qo_c = q1n_c;
    *qo_d = q1n_d;
  }
  n = sqrt(((*qo_a * *qo_a + *qo_b * *qo_b) + *qo_c * *qo_c) + *qo_d * *qo_d);
  *qo_a /= n;
  *qo_b /= n;
  *qo_c /= n;
  *qo_d /= n;
}

/*
 * File trailer for slerp.c
 *
 * [EOF]
 */
