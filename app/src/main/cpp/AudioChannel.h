//
// Created by Administrator on 2021/8/14 0014.
//

#ifndef WEPLAYER_AUDIOCHANNEL_H
#define WEPLAYER_AUDIOCHANNEL_H

#include "BaseChannel.h"

class AudioChannel : public BaseChannel{

public:
    AudioChannel(int stream_index, AVCodecContext *codecContext);

    ~AudioChannel();

    void stop();
};


#endif //WEPLAYER_AUDIOCHANNEL_H
