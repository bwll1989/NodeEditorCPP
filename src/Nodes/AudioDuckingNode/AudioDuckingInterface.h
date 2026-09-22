#pragma once
#include <QWidget>
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief Variable 触发的多通道闪避参数面板
     * Channels + Depth / Attack / Hold / Release
     */
    class AudioDuckingInterface : public QWidget
    {
        Q_OBJECT

    public:
        static constexpr int kMinChannels = 1;
        static constexpr int kMaxChannels = 64;
        static constexpr int kDefaultChannels = 2;

        explicit AudioDuckingInterface(QWidget *parent = nullptr);
        ~AudioDuckingInterface() override;

        IntDragValueWidget *channelsSpin = nullptr;
        FloatDragValueWidget *depthSpin = nullptr;
        FloatDragValueWidget *attackSpin = nullptr;
        FloatDragValueWidget *holdSpin = nullptr;
        FloatDragValueWidget *releaseSpin = nullptr;
    };
}
