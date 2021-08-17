//
// Created by Administrator on 2021/8/14 0014.
//

#ifndef WEPLAYER_JNICALLBACKHELPER_H
#define WEPLAYER_JNICALLBACKHELPER_H

#include <jni.h>
#include "utils/util.h"

class JNICallbackHelper {
private:
    JavaVM *vm = nullptr;
    JNIEnv *env = nullptr;
    jobject job;
    jmethodID jmd_prepared;

public:
    JNICallbackHelper(JavaVM *vm, JNIEnv *env, jobject job);

    virtual ~JNICallbackHelper();

    void onPrepared(int thread_mode);
};


#endif //WEPLAYER_JNICALLBACKHELPER_H
