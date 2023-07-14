# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("accumo")

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        accumo

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        native-lib.cpp
        filter/BasicESKF.c filter/dist.c filter/ErrorStateIMUGPSFuserBase.c
        filter/insfilterErrorState.c filter/insfilterErrorStateCoder_data.c filter/INSFilterESKF.c
        filter/rotmat.c filter/rt_nonfinite.c filter/slerp.c
        filter/filter.cpp
        depth/warping.cpp
        ncnn-wrapper.cpp)

target_include_directories(accumo
        PUBLIC .
        PUBLIC OpenCV-android-sdk/sdk/native/jni/include)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library(log-lib log)
find_package(OpenCV 4.5 REQUIRED java)

set(ncnn_DIR ${CMAKE_SOURCE_DIR}/ncnn-20220216-android/${ANDROID_ABI}/lib/cmake/ncnn)
find_package(ncnn REQUIRED)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries(accumo ncnn ${log-lib})