//
// Created by dsparch on 18. 9. 24.
//
#include <thread>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

#include <android/native_window_jni.h>

#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>

#include <media/NdkImage.h>
#include <media/NdkImageReader.h>

#include <android/surface_texture.h>
#include <android/surface_texture_jni.h>
#include <android/window.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <zconf.h>

#include "opencv2/opencv.hpp"
#include "NativeCamera.h"
#include "LOG.h"

static bool captureReady = true;
static bool sessionAlive = true;

/* Device listeners */

static void onDisconnected(void* context, ACameraDevice* device)
{
    LOGD("onDisconnected");
}

static void onError(void* context, ACameraDevice* device, int error)
{
    LOGD("error %d", error);
}

static ACameraDevice_stateCallbacks cameraDeviceCallbacks = {
        .context = nullptr,
        .onDisconnected = onDisconnected,
        .onError = onError,
};


/* Session state callbacks */

static void onSessionActive(void* context, ACameraCaptureSession *session)
{
    LOGD("Session active");
    sessionAlive = true;
    captureReady = false;
}

static void onSessionReady(void* context, ACameraCaptureSession *session)
{
    LOGD("Session ready");
    sessionAlive = true;
    captureReady = false;
}

static void onSessionClosed(void* context, ACameraCaptureSession *session)
{
    LOGD("Session closed");
    sessionAlive = false;
}

static ACameraCaptureSession_stateCallbacks sessionStateCallbacks {
        .context = nullptr,
        .onActive = onSessionActive,
        .onReady = onSessionReady,
        .onClosed = onSessionClosed
};


/* Capture callbacks */

static void onCaptureStarted(void* context, ACameraCaptureSession* session,
                             const ACaptureRequest* request, int64_t timestamp)
{
    LOGD("Capture Started");
    captureReady = false;
}


static void onCaptureFailed(void* context, ACameraCaptureSession* session,
                     ACaptureRequest* request, ACameraCaptureFailure* failure)
{
    LOGD("Capture Failed");
}

static void onCaptureSequenceCompleted(void* context, ACameraCaptureSession* session,
                                int sequenceId, int64_t frameNumber)
{
    LOGD("onCaptureSequenceCompleted %d", sequenceId);
    captureReady = true;
}

static void onCaptureSequenceAborted(void* context, ACameraCaptureSession* session,
                              int sequenceId)
{
    LOGD("onCaptureSequenceAborted");
}

static void onCaptureCompleted (
        void* context, ACameraCaptureSession* session,
        ACaptureRequest* request, const ACameraMetadata* result)
{
    LOGD("Capture completed");
}

static void onCaptureBufferLost(
        void* context, ACameraCaptureSession* session,
        ACaptureRequest* request, ANativeWindow* window, int64_t frameNumber)
{
    LOGD("Capture Buffer Lost");
}

static void onCaptureProgressed(
        void* context, ACameraCaptureSession* session,
        ACaptureRequest* request, const ACameraMetadata* result)
{
    LOGD("Capture Progressed");
}


/* Image Reader Callback */
void imageAvailableCallback(void* context, AImageReader* reader)
{
    LOGD("Image Available");
    AImage *image = nullptr;
    auto status = AImageReader_acquireNextImage(reader, &image);

    if(status != AMEDIA_OK || image == nullptr)
        return;

    uint8_t *data = nullptr;
    int len = 0;
    AImage_getPlaneData(image, 0, &data, &len);

    int width = 0, height = 0;
    AImage_getWidth(image, &width);
    AImage_getHeight(image, &height);
    LOGD("w = %d, h = %d", width, height);

    // convert src image YUV -> RGBA
    cv::Mat src(height + height/2, width, CV_8UC1, data);
    cv::Mat img;

    // Do image processing
    cv::cvtColor(src, img, cv::COLOR_YUV2GRAY_NV21);
    cv::cvtColor(img, img, cv::COLOR_GRAY2RGBA);

    // 윈도우의 버퍼 포인터를 얻는다
    ANativeWindow_setBuffersGeometry(drawWindow, width, height, WINDOW_FORMAT_RGBA_8888 /*format unchanged*/);
    ANativeWindow_Buffer buf;
    if (int32_t err = ANativeWindow_lock(drawWindow, &buf, nullptr))
    {
        LOGE("ANativeWindow_lock failed with error code %d\n", err);
        return;
    }

    cv::Mat dst(buf.height, buf.width, CV_8UC4, buf.bits);

    // copy to data to surfaceview
    uchar *dbuf = dst.data;
    uchar *ibuf = img.data;

    for (int i = 0; i < img.rows; i++)
    {
        dbuf = dst.data + i * buf.width * dst.channels();
        memcpy(dbuf, ibuf, img.cols * img.channels());
        ibuf += img.cols * img.channels();
    }

    // Process data here
    AImage_delete(image);

    // 윈도우 락 풀어주기
    ANativeWindow_unlockAndPost(drawWindow);
}

static void initCam()
{
    // ASSUMPTION: Back camera is index[0] and front is index[1]
    cameraManager = ACameraManager_create();
    ACameraManager_getCameraIdList(cameraManager, &cameraIdList);
    ACameraManager_openCamera(cameraManager, cameraIdList->cameraIds[0], &cameraDeviceCallbacks, &cameraDevice);
    PrintCameraResolution();
}

static void exitCam()
{
    if (cameraManager)
    {
        LOGD("exit cam");
        // Stop recording to SurfaceTexture and do some cleanup
        ACameraCaptureSession_stopRepeating(captureSession);
        ACameraCaptureSession_close(captureSession);
        ACaptureSessionOutputContainer_free(outputs);
        ACaptureSessionOutput_free(windowOutput);
        ACaptureRequest_free(captureRequest);

        ACameraDevice_close(cameraDevice);
        ACameraManager_delete(cameraManager);
        ACameraManager_deleteCameraIdList(cameraIdList);
        cameraManager = nullptr;

        // Capture request for SurfaceTexture
        AImageReader_delete(imageReader);

        // release native windows
        ANativeWindow_unlockAndPost(nativeWindow);
        ANativeWindow_unlockAndPost(drawWindow);
        ANativeWindow_release(nativeWindow);
        ANativeWindow_release(drawWindow);
    }
}

static void initSurface(JNIEnv* env, jobject surface)
{
    LOGD("In %s", __FUNCTION__);

    /* DEBUG Prepare Debug window */
    drawWindow = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_acquire(drawWindow);
    int width = ANativeWindow_getWidth(drawWindow);
    int height = ANativeWindow_getHeight(drawWindow);

    media_status_t status;

    // Create AImageReader and set callback on it
    status = AImageReader_new(width, height, AIMAGE_FORMAT_YUV_420_888, 1, &imageReader);
    if(status != AMEDIA_OK)
        LOGD("AImageReader_new");

    status = AImageReader_getWindow(imageReader, &nativeWindow);
    if(status != AMEDIA_OK ||nativeWindow == nullptr)
        LOGD("AImageReader_getWindow failed");

    AImageReader_ImageListener listener{
            .context = nullptr,
            .onImageAvailable = imageAvailableCallback,
    };
    AImageReader_setImageListener(imageReader, &listener);

    // Create SessionOutput and container
    ACaptureSessionOutput_create(nativeWindow, &windowOutput);
    ACaptureSessionOutputContainer_create(&outputs);
    ACaptureSessionOutputContainer_add(outputs, windowOutput);

    // Create the session
    ACameraDevice_createCaptureSession(cameraDevice, outputs, &sessionStateCallbacks, &captureSession);

    // Prepare request for texture target
    ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &captureRequest);

    ANativeWindow_acquire(nativeWindow);
    ACameraOutputTarget_create(nativeWindow, &outputTarget);
    ACaptureRequest_addTarget(captureRequest, outputTarget);

    ACameraCaptureSession_setRepeatingRequest(captureSession, &captureCallbacks, 1, &captureRequest, nullptr);
}

static void PrintCameraResolution()
{
    // exposure range
    ACameraMetadata *metadataObj = nullptr;
    ACameraManager_getCameraCharacteristics(cameraManager, cameraIdList->cameraIds[0], &metadataObj);
    ACameraMetadata_const_entry entry = {0};

    // JPEG format
    ACameraMetadata_getConstEntry(metadataObj,
                                  ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);

    for (int i = 0; i < entry.count; i += 4)
    {
        // We are only interested in output streams, so skip input stream
        int32_t input = entry.data.i32[i + 3];
        if (input)
            continue;

        int32_t format = entry.data.i32[i + 0];
        if (format == AIMAGE_FORMAT_YUV_420_888)
        {
            int32_t width = entry.data.i32[i + 1];
            int32_t height = entry.data.i32[i + 2];
            LOGD("camProps: maxWidth=%d vs maxHeight=%d", width, height);
        }
    }
}


extern "C"
{
    JNIEXPORT void JNICALL
    Java_sciomagelab_isp_MainActivity_startPreview(JNIEnv *env, jclass type,
                                                              jobject surface){
        initCam();
        initSurface(env, surface);
    }

    JNIEXPORT void JNICALL
    Java_sciomagelab_isp_MainActivity_stopPreview(JNIEnv *env, jclass type)
    {
        exitCam();
    }
}
