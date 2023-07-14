#include <jni.h>
#include <string>

#include <android/log.h>
#include "opencv2/imgproc/imgproc.hpp"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_accumo_MainActivity_stringFromJNI(JNIEnv *env,
                                                      jobject /* this */) {
  std::string hello = "Hello from C++";
  __android_log_print(ANDROID_LOG_INFO, "TAG", "Logging from C++");
  return env->NewStringUTF(hello.c_str());
}