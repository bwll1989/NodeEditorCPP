#pragma once
#include <QWidget>
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief 音频时间戳延时参数面板
     * Channels + Delay Frames
     */
    class AudioDelayInterface : public QWidget
    {
        Q_OBJECT

    public:
        static constexpr int kMinChannels = 1;
        static constexpr int kMaxChannels = 64;
        static constexpr int kDefaultChannels = 1;
        static constexpr int kMinDelayFrames = 0;
        static constexpr int kMaxDelayFrames = 64;
        static constexpr int kDefaultDelayFrames = 0;

        explicit AudioDelayInterface(QWidget *parent = nullptr);
        ~AudioDelayInterface() override;

        IntDragValueWidget *channelsSpin = nullptr;
        IntDragValueWidget *delayFramesSpin = nullptr;
    };
}
