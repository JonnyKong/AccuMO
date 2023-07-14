/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: INSFilterESKF.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 02-Mar-2022 23:31:18
 */

/* Include Files */
#include "INSFilterESKF.h"
#include "insfilterErrorStateCoder_internal_types.h"
#include "rt_nonfinite.h"

/* Function Definitions */
/*
 * Arguments    : const insfilterErrorState *obj
 *                double pos[3]
 *                double *orient_a
 *                double *orient_b
 *                double *orient_c
 *                double *orient_d
 * Return Type  : void
 */
void INSFilterESKF_pose(const insfilterErrorState *obj, double pos[3],
                        double *orient_a, double *orient_b, double *orient_c,
                        double *orient_d)
{
  *orient_a = obj->pState[0];
  *orient_b = obj->pState[1];
  *orient_c = obj->pState[2];
  *orient_d = obj->pState[3];
  pos[0] = obj->pState[4];
  pos[1] = obj->pState[5];
  pos[2] = obj->pState[6];
}

/*
 * File trailer for INSFilterESKF.c
 *
 * [EOF]
 */
