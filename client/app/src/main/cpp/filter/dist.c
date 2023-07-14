/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: dist.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 02-Mar-2022 23:31:18
 */

/* Include Files */
#include "dist.h"
#include "rt_nonfinite.h"
#include <math.h>

/* Function Definitions */
/*
 * Arguments    : double q_a
 *                double q_b
 *                double q_c
 *                double q_d
 *                double p_a
 *                double p_b
 *                double p_c
 *                double p_d
 * Return Type  : double
 */
double quaternionBase_dist(double q_a, double q_b, double q_c, double q_d,
                           double p_a, double p_b, double p_c, double p_d)
{
  double deltaQuat_b;
  double deltaQuat_c;
  double deltaQuat_d;
  double rpart;
  rpart = ((q_a * p_a - q_b * -p_b) - q_c * -p_c) - q_d * -p_d;
  deltaQuat_b = ((q_a * -p_b + q_b * p_a) + q_c * -p_d) - q_d * -p_c;
  deltaQuat_c = ((q_a * -p_c - q_b * -p_d) + q_c * p_a) + q_d * -p_b;
  deltaQuat_d = ((q_a * -p_d + q_b * -p_c) - q_c * -p_b) + q_d * p_a;
  rpart /= sqrt(((rpart * rpart + deltaQuat_b * deltaQuat_b) +
                 deltaQuat_c * deltaQuat_c) +
                deltaQuat_d * deltaQuat_d);
  if (rpart < 0.0) {
    rpart = -rpart;
  }
  return 2.0 * acos(rpart);
}

/*
 * File trailer for dist.c
 *
 * [EOF]
 */
