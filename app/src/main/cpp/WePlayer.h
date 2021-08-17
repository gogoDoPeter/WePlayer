//
// Created by Administrator on 2021/8/14 0014.
//

#ifndef WEPLAYER_WEPLAYER_H
#define WEPLAYER_WEPLAYER_H

#include <pthread.h>
#include "JNICallbackHelper.h"
#include "utils/AndroidLogHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"
#include "utils/util.h"

extern "C" { // ffmpeg是纯c写的，必须采用c的编译方式，否则奔溃
#include <libavformat/avformat.h>
}

class WePlayer {
private:
    char *pDataSource = nullptr;// 指针 请赋初始值
    pthread_t pid_prepare;
    pthread_t pid_start;
    AVFormatContext *pFormatContext = nullptr;// 媒体上下文 封装格式
    AudioChannel *pAudioChannel = nullptr;
    VideoChannel *pVideoChannel = nullptr;
    JNICallbackHelper *pHelper = nullptr;
    bool isPlaying;// 是否播放
    RenderCallback renderCallback;//void (*renderCallback)(uint8_t *, int, int, int) const;

public:
    WePlayer(const char *data_source, JNICallbackHelper *helper);

    ~WePlayer();
//    virtual ~WePlayer();
    void prepare();

    void prepare_();

    void start();

    void start_();

    void setRenderCallback(RenderCallback renderCallback);
};


#endif //WEPLAYER_WEPLAYER_H
