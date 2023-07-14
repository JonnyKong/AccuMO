/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: insfilterErrorState.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 02-Mar-2022 23:31:18
 */

/* Include Files */
#include "insfilterErrorState.h"
#include "insfilterErrorStateCoder_internal_types.h"
#include "rt_nonfinite.h"

/* Type Definitions */
#ifndef typedef_struct_T
#define typedef_struct_T
typedef struct {
  double ReferenceLocation[3];
  double IMUSampleRate;
  double State[17];
  double StateCovariance[256];
  double AccelerometerNoise[3];
  double GyroscopeNoise[3];
  double AccelerometerBiasNoise[3];
  double GyroscopeBiasNoise[3];
} struct_T;
#endif /* typedef_struct_T */

#ifndef typedef_b_struct_T
#define typedef_b_struct_T
typedef struct {
  double ReferenceLocation[3];
  double IMUSampleRate;
} b_struct_T;
#endif /* typedef_b_struct_T */

/* Variable Definitions */
static const signed char iv[17] = {1, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 1};

/* Function Declarations */
static void b_cast(const double t1_ReferenceLocation[3],
                   double t1_IMUSampleRate, struct_T *r);

static b_struct_T cast(const double t0_ReferenceLocation[3]);

/* Function Definitions */
/*
 * Arguments    : const double t1_ReferenceLocation[3]
 *                double t1_IMUSampleRate
 *                struct_T *r
 * Return Type  : void
 */
static void b_cast(const double t1_ReferenceLocation[3],
                   double t1_IMUSampleRate, struct_T *r)
{
  r->ReferenceLocation[0] = t1_ReferenceLocation[0];
  r->ReferenceLocation[1] = t1_ReferenceLocation[1];
  r->ReferenceLocation[2] = t1_ReferenceLocation[2];
  r->IMUSampleRate = t1_IMUSampleRate;
}

/*
 * Arguments    : const double t0_ReferenceLocation[3]
 * Return Type  : b_struct_T
 */
static b_struct_T cast(const double t0_ReferenceLocation[3])
{
  b_struct_T r;
  r.ReferenceLocation[0] = t0_ReferenceLocation[0];
  r.ReferenceLocation[1] = t0_ReferenceLocation[1];
  r.ReferenceLocation[2] = t0_ReferenceLocation[2];
  return r;
}

/*
 * Arguments    : insfilterErrorState *obj
 * Return Type  : insfilterErrorState *
 */
insfilterErrorState *c_insfilterErrorState_insfilter(insfilterErrorState *obj)
{
  insfilterErrorState *b_obj;
  int i;
  b_obj = obj;
  b_obj->AccelerometerNoise[0] = 0.0001;
  b_obj->AccelerometerNoise[1] = 0.0001;
  b_obj->AccelerometerNoise[2] = 0.0001;
  b_obj->GyroscopeNoise[0] = 1.0E-6;
  b_obj->GyroscopeNoise[1] = 1.0E-6;
  b_obj->GyroscopeNoise[2] = 1.0E-6;
  b_obj->AccelerometerBiasNoise[0] = 0.0001;
  b_obj->AccelerometerBiasNoise[1] = 0.0001;
  b_obj->AccelerometerBiasNoise[2] = 0.0001;
  b_obj->GyroscopeBiasNoise[0] = 1.0E-9;
  b_obj->GyroscopeBiasNoise[1] = 1.0E-9;
  b_obj->GyroscopeBiasNoise[2] = 1.0E-9;
  for (i = 0; i < 17; i++) {
    b_obj->pState[i] = iv[i];
  }
  for (i = 0; i < 256; i++) {
    b_obj->pStateCovariance[i] = 1.0;
  }
  b_obj->ReferenceLocation[0] = 0.0;
  b_obj->ReferenceLocation[1] = 0.0;
  b_obj->ReferenceLocation[2] = 0.0;
  b_obj->IMUSampleRate = 30.0;
  return b_obj;
}

/*
 * Arguments    : const insfilterErrorState *obj
 *                insfilterErrorState *iobj_0
 * Return Type  : insfilterErrorState *
 */
insfilterErrorState *insfilterErrorState_copy(const insfilterErrorState *obj,
                                              insfilterErrorState *iobj_0)
{
  insfilterErrorState *cpObj;
  struct_T expl_temp;
  double s_StateCovariance[256];
  double s_State[17];
  double s_ReferenceLocation[3];
  double s_AccelerometerBiasNoise_idx_0;
  double s_AccelerometerBiasNoise_idx_1;
  double s_AccelerometerBiasNoise_idx_2;
  double s_GyroscopeBiasNoise_idx_0;
  double s_GyroscopeBiasNoise_idx_1;
  double s_GyroscopeBiasNoise_idx_2;
  double s_GyroscopeNoise_idx_1;
  double s_GyroscopeNoise_idx_2;
  double s_IMUSampleRate;
  int i;
  s_ReferenceLocation[0] = obj->ReferenceLocation[0];
  s_ReferenceLocation[1] = obj->ReferenceLocation[1];
  s_ReferenceLocation[2] = obj->ReferenceLocation[2];
  s_IMUSampleRate = obj->IMUSampleRate;
  s_ReferenceLocation[0] = (cast(s_ReferenceLocation)).ReferenceLocation[0];
  s_ReferenceLocation[1] = (cast(s_ReferenceLocation)).ReferenceLocation[1];
  s_ReferenceLocation[2] = (cast(s_ReferenceLocation)).ReferenceLocation[2];
  b_cast(s_ReferenceLocation, s_IMUSampleRate, &expl_temp);
  for (i = 0; i < 17; i++) {
    s_State[i] = obj->pState[i];
  }
  for (i = 0; i < 256; i++) {
    s_StateCovariance[i] = obj->pStateCovariance[i];
  }
  s_ReferenceLocation[0] = obj->AccelerometerNoise[0];
  s_ReferenceLocation[1] = obj->AccelerometerNoise[1];
  s_ReferenceLocation[2] = obj->AccelerometerNoise[2];
  s_IMUSampleRate = obj->GyroscopeNoise[0];
  s_GyroscopeNoise_idx_1 = obj->GyroscopeNoise[1];
  s_GyroscopeNoise_idx_2 = obj->GyroscopeNoise[2];
  s_AccelerometerBiasNoise_idx_0 = obj->AccelerometerBiasNoise[0];
  s_AccelerometerBiasNoise_idx_1 = obj->AccelerometerBiasNoise[1];
  s_AccelerometerBiasNoise_idx_2 = obj->AccelerometerBiasNoise[2];
  s_GyroscopeBiasNoise_idx_0 = obj->GyroscopeBiasNoise[0];
  s_GyroscopeBiasNoise_idx_1 = obj->GyroscopeBiasNoise[1];
  s_GyroscopeBiasNoise_idx_2 = obj->GyroscopeBiasNoise[2];
  iobj_0->AccelerometerNoise[0] = 0.0001;
  iobj_0->AccelerometerNoise[1] = 0.0001;
  iobj_0->AccelerometerNoise[2] = 0.0001;
  iobj_0->GyroscopeNoise[0] = 1.0E-6;
  iobj_0->GyroscopeNoise[1] = 1.0E-6;
  iobj_0->GyroscopeNoise[2] = 1.0E-6;
  iobj_0->AccelerometerBiasNoise[0] = 0.0001;
  iobj_0->AccelerometerBiasNoise[1] = 0.0001;
  iobj_0->AccelerometerBiasNoise[2] = 0.0001;
  iobj_0->GyroscopeBiasNoise[0] = 1.0E-9;
  iobj_0->GyroscopeBiasNoise[1] = 1.0E-9;
  iobj_0->GyroscopeBiasNoise[2] = 1.0E-9;
  for (i = 0; i < 17; i++) {
    iobj_0->pState[i] = iv[i];
  }
  for (i = 0; i < 256; i++) {
    iobj_0->pStateCovariance[i] = 1.0;
  }
  iobj_0->ReferenceLocation[0] = 0.0;
  iobj_0->ReferenceLocation[1] = 0.0;
  iobj_0->ReferenceLocation[2] = 0.0;
  cpObj = iobj_0;
  iobj_0->ReferenceLocation[0] = expl_temp.ReferenceLocation[0];
  iobj_0->ReferenceLocation[1] = expl_temp.ReferenceLocation[1];
  iobj_0->ReferenceLocation[2] = expl_temp.ReferenceLocation[2];
  iobj_0->IMUSampleRate = expl_temp.IMUSampleRate;
  for (i = 0; i < 17; i++) {
    iobj_0->pState[i] = s_State[i];
  }
  for (i = 0; i < 256; i++) {
    iobj_0->pStateCovariance[i] = s_StateCovariance[i];
  }
  iobj_0->AccelerometerNoise[0] = s_ReferenceLocation[0];
  iobj_0->AccelerometerNoise[1] = s_ReferenceLocation[1];
  iobj_0->AccelerometerNoise[2] = s_ReferenceLocation[2];
  iobj_0->GyroscopeNoise[0] = s_IMUSampleRate;
  iobj_0->GyroscopeNoise[1] = s_GyroscopeNoise_idx_1;
  iobj_0->GyroscopeNoise[2] = s_GyroscopeNoise_idx_2;
  iobj_0->AccelerometerBiasNoise[0] = s_AccelerometerBiasNoise_idx_0;
  iobj_0->AccelerometerBiasNoise[1] = s_AccelerometerBiasNoise_idx_1;
  iobj_0->AccelerometerBiasNoise[2] = s_AccelerometerBiasNoise_idx_2;
  iobj_0->GyroscopeBiasNoise[0] = s_GyroscopeBiasNoise_idx_0;
  iobj_0->GyroscopeBiasNoise[1] = s_GyroscopeBiasNoise_idx_1;
  iobj_0->GyroscopeBiasNoise[2] = s_GyroscopeBiasNoise_idx_2;
  return cpObj;
}

/*
 * File trailer for insfilterErrorState.c
 *
 * [EOF]
 */
