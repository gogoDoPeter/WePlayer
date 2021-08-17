//
// Created by Administrator on 2021/8/15 0015.
//

#ifndef WEPLAYER_BASECHANNEL_H
#define WEPLAYER_BASECHANNEL_H

//#include "ffmpeg/include/libavcodec/avcodec.h"
extern "C" {
#include <libavcodec/avcodec.h>
};

#include "safe_queue.h"


class BaseChannel {
public:
    int stream_index; // 音频 或 视频 的下标
    SafeQueue<AVPacket *> packets;// 压缩的 数据包
    SafeQueue<AVFrame *> frames;//原始的 数据包
    bool isPlaying;// 音频 和 视频 都会有的标记 是否播放
    AVCodecContext *codecContext = nullptr;// 音频 视频 都需要的 解码器上下文
    BaseChannel(int streamIndex, AVCodecContext *codecContext) : stream_index(streamIndex),
                                                                 codecContext(codecContext) {
        //set callback
        packets.setReleaseCallback(releaseAVPacket);// 给队列设置Callback，Callback释放队列里面的数据
        frames.setReleaseCallback(releaseAVFrame); // 给队列设置Callback，Callback释放队列里面的数据
    }

    //TODO 父类析构一定要加virtual
    virtual ~BaseChannel() {
        packets.clear();
        frames.clear();
    }

    /**
     * 释放 队列中 所有的 AVPacket *
     * @param packet
     */
    // typedef void (*ReleaseCallback)(T *);
    static void releaseAVPacket(AVPacket **p) {
        if (p) {
            av_packet_free(p);// 释放队列里面的 T == AVPacket
            *p = NULL; //把一级指针地址置空，避免野指针问题
        }
    }
    /**
     * 释放 队列中 所有的 AVFrame *
     * @param packet
     */
    // typedef void (*ReleaseCallback)(T *);
    static void releaseAVFrame(AVFrame **f) {
        if (f) {
            av_frame_free(f);// 释放队列里面的 T == AVFrame
            *f = NULL;//把一级指针地址置空，避免野指针问题
        }
    }

};

#endif //WEPLAYER_BASECHANNEL_H
