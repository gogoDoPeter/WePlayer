//
// Created by Administrator on 2021/8/14 0014.
//

#include "VideoChannel.h"

VideoChannel::VideoChannel(int stream_index, AVCodecContext *codecContext)
        : BaseChannel(stream_index, codecContext) {

}

void *task_video_decode(void *args) {
    //WePlayer *player = static_cast<WePlayer *>(args);
    auto *video_channel = static_cast<VideoChannel *>(args);
    video_channel->video_decode();
    return 0;//Must return value
}

void *task_video_play(void *args) {
    auto *video_channel = static_cast<VideoChannel *>(args);
    video_channel->video_play();
    return 0;//Must return value
}

// 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列  解码【真正干活的就是他】
void VideoChannel::video_decode() {
    AVPacket *pkt = 0;
    while (isPlaying) {
        int ret = packets.getQueueAndDel(pkt); // 阻塞式函数
        if (!isPlaying) {
            break; // 如果关闭了播放，跳出循环，releaseAVPacket(&pkt);
        }

        if (!ret) { // ret == 0
            continue; // 哪怕是没有成功，也要继续（假设：你生产太慢(压缩包加入队列)，我消费就等一下你）
        }

        // 最新的FFmpeg，和旧版本差别很大， 新版本：1.发送pkt（压缩包）给缓冲区，  2.从缓冲区拿出来（原始包）
        ret = avcodec_send_packet(codecContext, pkt);

        // FFmpeg源码缓存一份pkt，大胆释放即可
        releaseAVPacket(&pkt);

        if (ret) {
            break; // avcodec_send_packet 出现了错误，结束循环
        }

        // 下面是从 FFmpeg缓冲区 获取 原始包
        AVFrame *frame = av_frame_alloc(); // AVFrame： 解码后的视频原始数据包
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            // B帧  B帧参考前面成功  B帧参考后面失败, 产生这种情况的原因可能是P帧没有出来，再拿一次就行了
            continue;
        } else if (ret != 0) {
            break; // 错误了
        }
        // 终于拿到了 原始包
        frames.insertToQueue(frame);
    } // end while
    releaseAVPacket(&pkt); //??会不会存在释放两次的情况？对应 avcodec_send_packet失败跳出 和 avcodec_receive_frame失败跳出的Case
}

// 2.把队列里面的原始包(AVFrame *)取出来， 播放 【真正干活的就是他】
void VideoChannel::video_play() {// 第二线线程：视频：从队列取出原始包，播放 【真正干活了】
    // SWS_FAST_BILINEAR == 很快 可能会模糊
    // SWS_BILINEAR 适中算法

    AVFrame *frame = 0;
    uint8_t *dst_data[4]; // RGBA
    int dst_linesize[4]; // RGBA
    // 原始包（YUV数据）  ---->[libswscale]   Android屏幕（RGBA数据）

    //给 dst_data 申请内存   width * height * 4 xxxx
    av_image_alloc(dst_data, dst_linesize,
                   codecContext->width, codecContext->height, AV_PIX_FMT_RGBA, 1);

    // yuv -> rgba
    SwsContext *sws_ctx = sws_getContext(
            // 下面是输入环节
            codecContext->width,
            codecContext->height,
            codecContext->pix_fmt, // 自动获取 xxx.mp4 的像素格式, 也可以设置AV_PIX_FMT_YUV420P // 写死的，兼容不好

            // 下面是输出环节
            codecContext->width,
            codecContext->height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR, NULL, NULL, NULL);

    while (isPlaying) {
        int ret = frames.getQueueAndDel(frame);
        if (!isPlaying) {
            break; // 如果关闭了播放，跳出循环，releaseAVPacket(&pkt);
        }
        if (!ret) { // ret == 0
            continue; // 哪怕是没有成功，也要继续（假设：你生产太慢(原始包加入队列)，我消费就等一下你）
        }

        // 格式转换 yuv ---> rgba
        sws_scale(sws_ctx,
                // 下面是输入环节 YUV的数据
                  frame->data, frame->linesize,
                  0, codecContext->height,

                // 下面是输出环节  成果：RGBA数据
                  dst_data,
                  dst_linesize
        );

        // ANatvieWindows 渲染工作  1   2下节课
        // SurfaceView ----- ANatvieWindows
        // 如何渲染一帧图像？
        // 答：宽，高，数据  ----> 函数指针的声明
        // 我拿不到Surface，只能回调给 native-lib.cpp

        // 基础：数组被传递会退化成指针，默认就是去1元素
        if(renderCallback){
            renderCallback(dst_data[0], codecContext->width, codecContext->height, dst_linesize[0]);
        }
        releaseAVFrame(&frame); // 释放原始包，因为已经被渲染完了，没用了
    }
    // 简单的释放
    releaseAVFrame(&frame); // 出现错误，所退出的循环，都要释放frame
    isPlaying = false;
    /**
     * Allocate an image with size w and h and pixel format pix_fmt, and
     * fill pointers and linesizes accordingly.
     * The allocated image buffer has to be freed by using
     * av_freep(&pointers[0]).
     *
     * @param align the value to use for buffer size alignment
     * @return the size in bytes required for the image buffer, a negative
     * error code in case of failure
     */
//    int av_image_alloc(uint8_t *pointers[4], int linesizes[4],
//                       int w, int h, enum AVPixelFormat pix_fmt, int align);
    av_free(&dst_data[0]);
    //&dst_data[0] = NULL;   or dst_data[0] = NULL, which is right?
    sws_freeContext(sws_ctx); // free(sws_ctx); FFmpeg必须使用人家的函数释放，直接崩溃
}

// 视频：1.解码    2.播放
// 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
// 2.把队列里面的原始包(AVFrame *)取出来， 播放
void VideoChannel::start() {
    isPlaying = true;

    // 队列开始工作了
    packets.setWork(1);
    frames.setWork(1);

    // 第一个线程： 视频：取出队列的压缩包 进行解码 解码后的原始包 再push队列中去
    pthread_create(&pid_video_decode, 0, task_video_decode, this);

    // 第二线线程：视频：从队列取出原始包，播放
    pthread_create(&pid_video_play, 0, task_video_play, this);
}

void VideoChannel::stop() {

}

void VideoChannel::setRenderCallback(RenderCallback renderCallback) {
    VideoChannel::renderCallback = renderCallback;
}


