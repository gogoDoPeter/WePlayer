#include <jni.h>
#include <string>
#include "WePlayer.h"
#include <android/native_window_jni.h> // ANativeWindow 用来渲染画面的 == Surface对象

using namespace std;

WePlayer *gpPlayer = nullptr;
JavaVM *gpVm = nullptr;
ANativeWindow *pWindow = nullptr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;// 静态初始化 锁

jint JNI_OnLoad(JavaVM *vm, void *args) {
    ::gpVm = vm;
    return JNI_VERSION_1_6;
}

// 函数指针 实现  渲染工作
void renderFrame(uint8_t *src_data, int width, int height, int src_lineSize) {
    pthread_mutex_lock(&mutex);
    if (!pWindow) {
        pthread_mutex_unlock(&mutex); // 出现了问题后，必须考虑到，释放锁，怕出现死锁问题
        return; //Add by myself
    }

    // 设置窗口的大小，各个属性
    ANativeWindow_setBuffersGeometry(pWindow, width, height, WINDOW_FORMAT_RGBA_8888);

    // 他自己有个缓冲区 buffer
    ANativeWindow_Buffer window_buffer; // 目前他是指针吗？

    // 如果我在渲染的时候，是被锁住的，那我就无法渲染，我需要释放 ，防止出现死锁
    if (ANativeWindow_lock(pWindow, &window_buffer, 0)) {
        ANativeWindow_release(pWindow);
        pWindow = NULL;

        pthread_mutex_unlock(&mutex); // 解锁，怕出现死锁
        return;
    }

    // TODO 开始真正渲染，因为window没有被锁住了，就可以把 rgba数据 ---> 字节对齐 渲染
    // 填充window_buffer  画面就出来了  === [目标]
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    int dst_linesize = window_buffer.stride * 4;

    for (int i = 0; i < window_buffer.height; ++i) { // 图：一行一行显示 [高度不用管，用循环了，遍历高度]
        // 视频分辨率：426 * 240
        // 视频分辨率：宽 426
        // 426 * 4(rgba8888) = 1704
        // memcpy(dst_data + i * 1704, src_data + i * 1704, 1704); // 花屏
        // 花屏原因：1704 无法 64字节对齐，所以花屏

        // ANativeWindow_Buffer 64字节对齐的算法，  1704无法以64位字节对齐
//         memcpy(dst_data + i * 1792, src_data + i * 1704, 1792); // OK的 1792 = 64 * 28
//         memcpy(dst_data + i * 1856, src_data + i * 1704, 1856); // test 1856 = 64 * 29(当做N的话), 会crash
//         memcpy(dst_data + i * 1920, src_data + i * 1704, 1920); // test 1856 = 64 * 30(当做N的话), 会crash 说明 N 是最近的64倍数积，有两个占位
        // memcpy(dst_data + i * 1793, src_data + i * 1704, 1793); // 部分花屏，无法64字节对齐
        // memcpy(dst_data + i * 1728, src_data + i * 1704, 1728); // 花屏   1728= 64 * 27

        // ANativeWindow_Buffer 64字节对齐的算法  1728
        // 占位 占位 占位 占位 占位 占位 占位 占位
        // 数据 数据 数据 数据 数据 数据 数据 空值

        // ANativeWindow_Buffer 64字节对齐的算法  1792  空间换时间
        // 占位 占位 占位 占位 占位 占位 占位 占位 占位
        // 数据 数据 数据 数据 数据 数据 数据 空值 空值

        // FFmpeg为什么认为  1704 没有问题 ？
        // FFmpeg是默认采用8字节对齐的，他就认为没有问题， 但是ANativeWindow_Buffer他是64字节对齐的，就有问题

        // 通用的
        memcpy(dst_data + i * dst_linesize, src_data + i * src_lineSize, dst_linesize); // OK的
    }

    // 数据刷新
    ANativeWindow_unlockAndPost(pWindow); // 解锁后 并且刷新 window_buffer的数据显示画面

    pthread_mutex_unlock(&mutex);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_peter_weplayer_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_weplayer_player_WePlayer_startNative(JNIEnv *env, jobject thiz) {
    if (gpPlayer) {
        gpPlayer->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_weplayer_player_WePlayer_prepareNative(JNIEnv *env, jobject job,
                                                      jstring _data_source) {
    LOGD("prepareNative +");
    const char *data_source = env->GetStringUTFChars(_data_source, 0);
    JNICallbackHelper *pHelper = new JNICallbackHelper(gpVm, env, job);// C++子线程回调 ， C++主线程回调
    LOGD("prepareNative new WePlayer");
    gpPlayer = new WePlayer(data_source, pHelper);
    LOGD("prepareNative setRenderCallback");
    gpPlayer->setRenderCallback(renderFrame);
    LOGD("prepareNative prepare");
    gpPlayer->prepare();
    env->ReleaseStringUTFChars(_data_source, data_source);
    LOGD("prepareNative -");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_weplayer_player_WePlayer_stopNative(JNIEnv *env, jobject thiz) {
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_weplayer_player_WePlayer_releaseNative(JNIEnv *env, jobject thiz) {
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_peter_weplayer_player_WePlayer_getFfmpegVersionNative(JNIEnv *env, jobject thiz) {
    string ffmpegVersion = "FFmpeg Version:";
    ffmpegVersion.append(av_version_info());
    return env->NewStringUTF(ffmpegVersion.c_str());
}

// 实例化出 window
extern "C"
JNIEXPORT void JNICALL
Java_com_peter_weplayer_player_WePlayer_setSurfaceNative(JNIEnv *env, jobject thiz,
                                                         jobject surface) {
    pthread_mutex_lock(&mutex);

    // 先释放之前的显示窗口
    if (pWindow) {
        ANativeWindow_release(pWindow);
        pWindow = NULL;
    }
    // 创建新的窗口用于视频显示
    pWindow = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&mutex);
}