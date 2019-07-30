cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)
set(ANDROID_STL "c++_shared")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

set(PROJECT_NAME isp-helper)

set(PATH_PROJECT /home/dsparch/Project/${PROJECT_NAME})
set(PATH_LIB_OPENCV ${PATH_PROJECT}/3rdparty_libraries/libraries/OpenCV-4.1.1/libs/${ANDROID_ABI}/libopencv_java4.so)

# OpenCV-4.1.1 include
include_directories(${PATH_PROJECT}/3rdparty_libraries/include)

include_directories(${PATH_PROJECT}/app/src/main/cpp/)

## EXAMPLE OF add_library
#add_library( # Sets the name of the library.
#             native-lib

             # Sets the library as a shared library.
#             SHARED

             # Provides a relative path to your source file(s).
#             src/main/cpp/native-lib.cpp )

add_library( lib_opencv SHARED IMPORTED )
set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${PATH_LIB_OPENCV})

add_library(native-camera2-jni SHARED
            src/main/cpp/NativeCamera.cpp)


find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

target_link_libraries(
        # Specifies the target library.
        native-camera2-jni
        -landroid
        -ljnigraphics
        camera2ndk
        mediandk
        lib_opencv

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )