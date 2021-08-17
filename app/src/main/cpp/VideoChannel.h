//
// Created by Administrator on 2021/8/14 0014.
//

#ifndef WEPLAYER_VIDEOCHANNEL_H
#define WEPLAYER_VIDEOCHANNEL_H

#include "BaseChannel.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>  //getVerison ...
#include <libavutil/imgutils.h>
};

typedef void(*RenderCallback) (uint8_t *, int, int, int); // 函数指针声明定义

class VideoChannel : public BaseChannel {
private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderCallback renderCallback;//void (*renderCallback)(uint8_t *, int, int, int) const;

public:
    VideoChannel(int stream_index, AVCodecContext *codecContext);

    void start();

    void stop();

    void video_decode();

    void video_play();

    void setRenderCallback(RenderCallback renderCallback);
};


#endif //WEPLAYER_VIDEOCHANNEL_H
