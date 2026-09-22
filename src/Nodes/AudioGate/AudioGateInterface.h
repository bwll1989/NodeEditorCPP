#pragma once
#include <QWidget>
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief QSC Gate 参数面板
     * Channels + Threshold / Depth / Attack / Hold / Release
     */
    class AudioGateInterface : public QWidget
    {
        Q_OBJECT

    public:
        static constexpr int kMinChannels = 1;
        static constexpr int kMaxChannels = 64;
        static constexpr int kDefaultChannels = 1;

        explicit AudioGateInterface(QWidget *parent = nullptr);
        ~AudioGateInterface() override;

        IntDragValueWidget *channelsSpin = nullptr;
        FloatDragValueWidget *thresholdSpin = nullptr;
        FloatDragValueWidget *depthSpin = nullptr;
        FloatDragValueWidget *attackSpin = nullptr;
        FloatDragValueWidget *holdSpin = nullptr;
        FloatDragValueWidget *releaseSpin = nullptr;
    };
}
