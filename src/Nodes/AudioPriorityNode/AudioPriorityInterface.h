#pragma once
#include <QWidget>
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief Priority Ducker 参数面板
     * Channels + Threshold / Depth / Priority Gain / Attack / Hold / Release
     */
    class AudioPriorityInterface : public QWidget
    {
        Q_OBJECT

    public:
        static constexpr int kMinChannels = 1;
        static constexpr int kMaxChannels = 64;
        static constexpr int kDefaultChannels = 2;

        explicit AudioPriorityInterface(QWidget *parent = nullptr);
        ~AudioPriorityInterface() override;

        IntDragValueWidget *channelsSpin = nullptr;
        FloatDragValueWidget *thresholdSpin = nullptr;
        FloatDragValueWidget *depthSpin = nullptr;
        FloatDragValueWidget *priorityGainSpin = nullptr;
        FloatDragValueWidget *attackSpin = nullptr;
        FloatDragValueWidget *holdSpin = nullptr;
        FloatDragValueWidget *releaseSpin = nullptr;
    };
}
