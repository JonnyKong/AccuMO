#include <android/log.h>
#include <jni.h>
#include <string>

#include "depth/warping.h"
#include "net.h"

#define LOG_TAG "ncnn-wrapper"
#define ALOG(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using ncnn::Mat;

namespace {
ncnn::Net net;
// For converting [0-255] to [0-1]
float convert_mean_vals[] = {0, 0, 0};
float convert_norm_vals[] = {1.0 / 255, 1.0 / 255, 1.0 / 255};

// For normalization
float mean_vals[] = {0.485, 0.456, 0.406};
float norm_vals[] = {1.0 / 0.229, 1.0 / 0.224, 1.0 / 0.225};
} // namespace

extern "C" JNIEXPORT void JNICALL
Java_com_example_accumo_MainActivity_loadModel(JNIEnv *env,
                                                  jobject /* this */,
                                                  jstring model) {
  auto model_char = env->GetStringUTFChars(model, nullptr);
  std::string model_str(model_char);
  env->ReleaseStringUTFChars(model, model_char);
  net.load_param(("/sdcard/accumo/models/" + model_str + ".param").c_str());
  net.load_model(("/sdcard/accumo/models/" + model_str + ".bin").c_str());
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_example_accumo_MainActivity_inference(JNIEnv *env,
                                                  jobject /* this */,
                                                  jbyteArray yuv) {
  // Will resize output to this shape
  const int originalWidth = 832;
  const int originalHeight = 256;
  // Inference shape is like this because model trained this way
  const int inferenceWidth = 224;
  const int inferenceHeight = 64;

  auto yuv_arr = env->GetByteArrayElements(yuv, nullptr);
  auto resized = new unsigned char[inferenceHeight * inferenceWidth * 3 / 2];
  ncnn::resize_bilinear_yuv420sp(
          reinterpret_cast<const unsigned char *>(yuv_arr), originalWidth, originalHeight,
          resized, inferenceWidth, inferenceHeight);
  env->ReleaseByteArrayElements(yuv, yuv_arr, JNI_ABORT);

  auto rgb = new unsigned char[inferenceHeight * inferenceWidth * 3];
  ncnn::yuv420sp2rgb(resized, inferenceWidth, inferenceHeight, rgb);
  delete[] resized;

  auto in = Mat::from_pixels(rgb, Mat::PIXEL_RGB, inferenceWidth, inferenceHeight);
  delete[] rgb;

  // [0, 255] -> [0, 1]
  in.substract_mean_normalize(convert_mean_vals, convert_norm_vals);
  // Normalize like in training
  in.substract_mean_normalize(mean_vals, norm_vals);

  auto ex = net.create_extractor();
  ex.set_light_mode(true);
  ex.set_num_threads(4);
  ex.input("input.1", in);
  Mat out;
  ex.extract("424", out);

  // Interp nearest
  interpNearest(out.h, out.w, (float *)out.data, NULL, NULL, NULL);

  // Resize
  Mat outResized;
  resize_bilinear(out, outResized, originalWidth, originalHeight);

  // Marshall output
  jfloatArray ret = env->NewFloatArray(outResized.total());
  env->SetFloatArrayRegion(ret, 0, outResized.total(),
                           (float *)outResized.data);

  return ret;
}
