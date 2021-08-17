//
// Created by Administrator on 2021/8/14 0014.
//

#include "WePlayer.h"

// 此函数和DerryPlayer这个对象没有关系，你没法拿DerryPlayer的私有成员
//void *(*task_prepare)(void *);
void *task_prepare(void *args) {
    WePlayer *player = static_cast<WePlayer *>(args);
    LOGD("prepare_");
    player->prepare_();

    return nullptr;//TODO here must return,引起错误很难查!!!
}

WePlayer::WePlayer(const char *data_source, JNICallbackHelper *helper) {
    this->pDataSource = new char[strlen(data_source) + 1];
    strcpy(this->pDataSource, data_source);
    this->pHelper = helper;
}

WePlayer::~WePlayer() {
    if (pDataSource) {
        delete pDataSource;
        pDataSource = nullptr;//释放资源后，地址要赋值为null，避免野指针出现
    }
    if (pHelper) {
        delete pHelper;
        pHelper = nullptr;
    }
}

void WePlayer::prepare() {
// 问题：当前的prepare函数，是子线程 还是 主线程 ？
    // 答：此函数是被MainActivity的onResume调用下来的（主线程）

    // 解封装 FFmpeg来解析  data_source 可以直接解析吗？
    // 答：data_source == 文件io流，  直播网络rtmp， 所以按道理来说，会耗时，所以必须使用子线程
    LOGD("prepare +");
    // 创建子线程
    pthread_create(&pid_prepare, 0, task_prepare, this);
    LOGD("prepare -");
}

//TODO 此函数 是 子线程中运行
void WePlayer::prepare_() {
    LOGD("prepare_ +");
    // 为什么FFmpeg源码，大量使用上下文Context？
    // 答：因为FFmpeg源码是纯C的，他不像C++、Java ， 上下文的出现是为了贯彻环境，就相当于Java的this能够操作成员

    //第一步：打开媒体地址（文件路径， 直播地址rtmp）
    pFormatContext = avformat_alloc_context();  //avformat_free_context()

    AVDictionary *dictionary = nullptr;
    av_dict_set(&dictionary, "timeout", "5000000", 0);// 单位微妙
    LOGD("prepare_  pDataSource:%s", pDataSource);
    av_register_all();
    /**
        * 1，AVFormatContext *
        * 2，路径
        * 3，AVInputFormat *fmt  Mac、Windows 摄像头、麦克风， 我们目前安卓用不到
        * 4，各种设置：例如：Http 连接超时， 打开rtmp的超时  AVDictionary **options
        */
    int ret = avformat_open_input(&pFormatContext, pDataSource, 0, &dictionary);
    LOGD("prepare_ avformat_open_input, ret:%d", ret);
    //释放字典，用完注意及时释放
    av_dict_free(&dictionary);
    LOGD("prepare_ after av_dict_free, ret:%d", ret);
    if (ret) {
        // 把错误信息反馈给Java，回调给Java  Toast【打开媒体格式失败，请检查代码】
        // TODO 第一节课作业：JNI 反射回调到Java方法，并提示
        char buf[2048] = {0};
        av_strerror(ret, buf, 1024);
        LOGE("prepare_ Couldn't open file %s: %d(%s)", pDataSource, ret, buf);
        LOGE("prepare_ avformat_open_input fail, retval:%d", ret);
        //Notice resource need release
        return;
    }
    LOGD("prepare_ avformat_find_stream_info 查找媒体中的音视频流的信息");
    //TODO 第二步：查找媒体中的音视频流的信息
    ret = avformat_find_stream_info(pFormatContext, 0);
    if (ret < 0) {
        // TODO JNI 反射回调到Java方法，并提示
        LOGE("avformat_find_stream_info fail, retval:%d", ret);
        //Notice resource need release
        return;
    }
    LOGD("prepare_ pFormatContext->nb_streams 根据流信息，流的个数，用循环来找");
    /** nb_streams
     * Number of elements in AVFormatContext.streams.
     *
     * Set by avformat_new_stream(), must not be modified by any other code.
     */
    //  第三步：根据流信息，流的个数，用循环来找
    for (int i = 0; i < pFormatContext->nb_streams; ++i) {
        //  第四步：获取媒体流（视频，音频）
        AVStream *pAvStream = pFormatContext->streams[i];
        //  第五步：从上面的流中 获取 编码解码的【参数】
        AVCodecParameters *pParameters = pAvStream->codecpar;
        //  第六步：（根据上面的【参数】）获取编解码器
        AVCodec *pCodec = avcodec_find_decoder(pParameters->codec_id);
        LOGD("prepare_ avcodec_alloc_context3, i=%d", i);
        //  第七步：编解码器 上下文 （这个才是真正干活的）
        AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
        if (NULL == pCodecContext) {
            LOGD("avcodec_alloc_contect3 fail");
            return;
        }
        LOGD("prepare_ parameters copy codecContext");
        //  第八步：他目前是一张白纸（parameters copy codecContext）
        ret = avcodec_parameters_to_context(pCodecContext, pParameters);
        if (ret < 0) {
            LOGD("avcodec_parameters_to_context fail,ret:%d", ret);
            return;
        }
        LOGD("prepare_ avcodec_open2");
        //  第九步：打开解码器
        ret = avcodec_open2(pCodecContext, pCodec, 0);
        if (ret) {
            LOGD("avcodec open fail, ret:%d", ret);
            return;
        }
        LOGD("prepare_ 获取流的类型 codec_type");
        //  第十步：从编解码器参数中，获取流的类型 codec_type  ===  音频 视频
        if (pParameters->codec_type == AVMEDIA_TYPE_AUDIO) {// 音频
            LOGD("prepare_ create audioChannel");
            pAudioChannel = new AudioChannel(i, pCodecContext);
        } else if (pParameters->codec_type == AVMEDIA_TYPE_VIDEO) {    //  视频
            LOGD("prepare_  create videoChannel");
            pVideoChannel = new VideoChannel(i, pCodecContext);
            pVideoChannel->setRenderCallback(renderCallback);
        }
    }
    if (!pAudioChannel || !pVideoChannel) {
        LOGD("pAudioChannle = %p, pVideoChannel=%p", pAudioChannel, pVideoChannel);
        return;
    }
    LOGD("prepare_ pHelper->onPrepare");
    //媒体文件 OK了，通知给上层
    if (pHelper) {
        pHelper->onPrepared(THREAD_CHILD);
    }
    LOGD("prepare_ -");
}

// 此函数和 WePlayer 这个对象没有关系，你没法拿 WePlayer 的私有成员, 通过args的指针传递数据过来
//其他两种方法可以做到，没这个好
//1)将私有成员改成public
//2)将task_start声明为友元函数

//void *(*task_start)(void *);
void *task_start(void *args) {
    WePlayer *player = static_cast<WePlayer *>(args);
    LOGD("task_start +");
    player->start_();
    LOGD("task_start -");
    return 0;//TODO here must return,引起错误很难查!!!
}

// 把 视频 音频 的压缩包(AVPacket *) 循环获取出来 加入到队列里面去
void WePlayer::start_() {// 子线程
    LOGD("WePlayer start_ +, isPlaying =%d",isPlaying);
    while(isPlaying){
        // AVPacket 可能是音频 也可能是视频（压缩包）
        AVPacket *packet = av_packet_alloc();
        int ret =av_read_frame(pFormatContext, packet);
        if (!ret) { // ret == 0
            // AudioChannel    队列
            // VideioChannel   队列

            // 把我们的 AVPacket* 加入队列， 音频 和 视频
            /*AudioChannel.insert(packet);
            VideioChannel.insert(packet);*/
            if (pVideoChannel && pVideoChannel->stream_index == packet->stream_index) {
                // 代表是视频
                pVideoChannel->packets.insertToQueue(packet);
            } else if (pAudioChannel && pAudioChannel->stream_index == packet->stream_index) {
                // 代表是音频
                // audio_channel->packets.insertToQueue(packet);
            }
        } else if (ret == AVERROR_EOF) { //   end of file == 读到文件末尾了 == AVERROR_EOF
            // TODO 表示读完了，要考虑释放播放完成，表示读完了 并不代表播放完毕，以后处理【同学思考 怎么处理】
        } else {
            break; // av_read_frame 出现了错误，结束当前循环
        }
    }// end while
    isPlaying=false;
    pVideoChannel->stop();
    pAudioChannel->stop();
    LOGD("WePlayer start_ -, isPlaying =%d",isPlaying);
}

void WePlayer::start() {
    LOGD("WePlayer start +, isPlaying =%d",isPlaying);
    isPlaying = true;
    // 视频：1.解码    2.播放
    // 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
    // 2.把队列里面的原始包(AVFrame *)取出来， 播放
    if (pVideoChannel) {
        pVideoChannel->start();
    }
    //TODO
//    if(pAudioChannel){
//        pAudioChannel->start();
//    }

    // 把 音频和视频 压缩包 加入队列里面去
    // 创建子线程
    pthread_create(&pid_start, 0, task_start, this);
    LOGD("WePlayer start -, isPlaying =%d",isPlaying);
}

void WePlayer::setRenderCallback(RenderCallback renderCallback) {
    WePlayer::renderCallback = renderCallback;
}
