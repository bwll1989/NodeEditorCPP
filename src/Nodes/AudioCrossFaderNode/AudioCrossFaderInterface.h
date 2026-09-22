#pragma once
#include <QWidget>
#include <QPushButton>
#include <QButtonGroup>

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief 多通道交叉淡化参数面板
     * Channels 路 A/B 成对输入，一一对应输出
     */
    class AudioCrossFaderInterface : public QWidget
    {
        Q_OBJECT

    public:
        static constexpr int kMinChannels = 1;
        static constexpr int kMaxChannels = 64;
        static constexpr int kDefaultChannels = 1;

        /** controlAction：0 = B→A（默认输出 A），1 = A→B */
        static constexpr int kActionToA = 0;
        static constexpr int kActionToB = 1;

        explicit AudioCrossFaderInterface(QWidget *parent = nullptr);
        ~AudioCrossFaderInterface() override;

        IntDragValueWidget *channelsSpin = nullptr;
        FloatDragValueWidget *mixSpin = nullptr;          // 0.0 ~ 1.0
        FloatDragValueWidget *fadeDurationSpin = nullptr; // ms
        QPushButton *fadeToBButton = nullptr;             // A -> B
        QPushButton *fadeToAButton = nullptr;             // B -> A
        QButtonGroup *actionGroup = nullptr;
    };
}
