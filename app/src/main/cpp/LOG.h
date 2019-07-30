//
// Created by dsparch on 18. 9. 20.
//

#ifndef AROVERLAYDRAWING_LOG_H
#define AROVERLAYDRAWING_LOG_H

#endif //AROVERLAYDRAWING_LOG_H

#include <android/log.h>
#define  LOG_TAG    "sciomagelab"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "mars", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "mars", __VA_ARGS__)
