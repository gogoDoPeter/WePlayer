//
// Created by Administrator on 2021/8/14 0014.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int stream_index, AVCodecContext *codecContext)
        : BaseChannel(stream_index, codecContext) {

}

AudioChannel::~AudioChannel() {

}

void AudioChannel::stop() {

}
