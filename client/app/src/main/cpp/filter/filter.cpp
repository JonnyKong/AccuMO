#include <jni.h>

#include "filter.h"

const Matrix4d Filter::kitti2right {{0, 0, 1, 0}, {-1, 0, 0, 0}, {0, -1, 0, 0}, {0, 0, 0, 1}};

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_accumo_Filter_createFilter(JNIEnv *env, jobject thiz) {
    return reinterpret_cast<jlong>(new Filter());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_accumo_Filter_deleteFilter(JNIEnv *env, jobject thiz, jlong handle) {
    delete reinterpret_cast<Filter *>(handle);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_accumo_Filter_predict(JNIEnv *env, jobject thiz, jlong handle,
                                          jdoubleArray accel, jdoubleArray gyro) {
    jdouble *accel_array = env->GetDoubleArrayElements(accel, nullptr);
    jdouble *gyro_array = env->GetDoubleArrayElements(gyro, nullptr);
    reinterpret_cast<Filter *>(handle)->predict(accel_array, gyro_array);
    env->ReleaseDoubleArrayElements(accel, accel_array, JNI_ABORT);
    env->ReleaseDoubleArrayElements(gyro, gyro_array, JNI_ABORT);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_accumo_Filter_startOffload(JNIEnv *env, jobject thiz, jlong handle) {
    reinterpret_cast<Filter *>(handle)->start_offload();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_accumo_Filter_fusemvo(JNIEnv *env, jobject thiz, jlong handle,
                                          jdoubleArray vo) {
    jdouble *vo_array = env->GetDoubleArrayElements(vo, nullptr);
    reinterpret_cast<Filter *>(handle)->fusemvo(vo_array);
    env->ReleaseDoubleArrayElements(vo, vo_array, JNI_ABORT);
}

extern "C"
JNIEXPORT jdoubleArray JNICALL
Java_com_example_accumo_Filter_pose(JNIEnv *env, jobject thiz, jlong handle) {
    Matrix4d pt = reinterpret_cast<Filter *>(handle)->pose().transpose();
    auto ret = env->NewDoubleArray(16);
    env->SetDoubleArrayRegion(ret, 0, 16, pt.data());
    return ret;
}

extern "C"
JNIEXPORT jdoubleArray JNICALL
Java_com_example_accumo_Filter_poseRelative(JNIEnv *env, jobject thiz, jlong handle, jint f1,
                                               jint f2) {
    Matrix4d pt = reinterpret_cast<Filter *>(handle)->pose_relative(f1, f2).transpose();
    auto ret = env->NewDoubleArray(16);
    env->SetDoubleArrayRegion(ret, 0, 16, pt.data());
    return ret;
}

extern "C"
JNIEXPORT jdouble JNICALL
Java_com_example_accumo_Filter_angle(JNIEnv *env, jobject thiz, jlong handle) {
    return reinterpret_cast<Filter *>(handle)->angle();
}
