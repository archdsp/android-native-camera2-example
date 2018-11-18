//
// Created by dsparch on 18. 9. 24.
//

#ifndef AROVERLAYDRAWING_NATIVECAMERA_H
#define AROVERLAYDRAWING_NATIVECAMERA_H
#endif //AROVERLAYDRAWING_NATIVECAMERA_H

#include <jni.h>
#include <pthread.h>

#include <android/native_window_jni.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>

// Native Camera Variable
static ACameraManager* cameraManager = nullptr;
static ACameraDevice* cameraDevice = nullptr;
static ACameraOutputTarget* outputTarget = nullptr;
static ACaptureRequest* captureRequest = nullptr;
static ANativeWindow* nativeWindow = nullptr, *drawWindow = nullptr;
static ACameraCaptureSession* captureSession = nullptr;
static ACaptureSessionOutput* windowOutput = nullptr;
static ACaptureSessionOutputContainer* outputs = nullptr;
static ACameraIdList* cameraIdList = nullptr;

static int sequenceID = 0;

static AImageReader* imageReader = nullptr;

/* Image Reader Callback */
void imageAvailableCallback(void* context, AImageReader* reader);


static void initCam();
static void exitCam();
static void initSurface(JNIEnv* env, jobject surface);
static void PrintCameraResolution();

static void onCaptureStarted(void* context, ACameraCaptureSession* session,
                             const ACaptureRequest* request, int64_t timestamp);
static void onCaptureProgressed(
        void* context, ACameraCaptureSession* session,
        ACaptureRequest* request, const ACameraMetadata* result);
static void onCaptureCompleted (
        void* context, ACameraCaptureSession* session,
        ACaptureRequest* request, const ACameraMetadata* result);
static void onCaptureFailed(void* context, ACameraCaptureSession* session,
                            ACaptureRequest* request, ACameraCaptureFailure* failure);
static void onCaptureSequenceCompleted(void* context, ACameraCaptureSession* session,
                                       int sequenceId, int64_t frameNumber);
static void onCaptureSequenceAborted(void* context, ACameraCaptureSession* session,
                                     int sequenceId);
static void onCaptureBufferLost(
        void* context, ACameraCaptureSession* session,
        ACaptureRequest* request, ANativeWindow* window, int64_t frameNumber);

static ACameraCaptureSession_captureCallbacks captureCallbacks {
        .context = nullptr,
        .onCaptureStarted = onCaptureStarted,
        .onCaptureProgressed = onCaptureProgressed,
        .onCaptureCompleted = onCaptureCompleted,
        .onCaptureFailed = onCaptureFailed,
        .onCaptureSequenceCompleted = onCaptureSequenceCompleted,
        .onCaptureSequenceAborted = onCaptureSequenceAborted,
        .onCaptureBufferLost = onCaptureBufferLost,
};

