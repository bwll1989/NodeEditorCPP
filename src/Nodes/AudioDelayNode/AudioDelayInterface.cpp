#include "AudioDelayInterface.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

using namespace Nodes;

AudioDelayInterface::AudioDelayInterface(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    auto *group = new QGroupBox(QStringLiteral("Audio Delay"), this);
    auto *layout = new QGridLayout(group);

    channelsSpin = new IntDragValueWidget();
    channelsSpin->setRange(kMinChannels, kMaxChannels);
    channelsSpin->setSingleStep(1);
    channelsSpin->setValue(kDefaultChannels);
    layout->addWidget(new QLabel(QStringLiteral("Channels")), 0, 0);
    layout->addWidget(channelsSpin, 0, 1);

    delayFramesSpin = new IntDragValueWidget();
    delayFramesSpin->setRange(kMinDelayFrames, kMaxDelayFrames);
    delayFramesSpin->setSingleStep(1);
    delayFramesSpin->setValue(kDefaultDelayFrames);
    delayFramesSpin->setSuffix(QStringLiteral(" 帧"));
    delayFramesSpin->setToolTip(QStringLiteral(
        "将输出时间戳整体后移 N 帧（系统时钟约 50 Hz，1 帧 ≈ 20 ms）。\n"
        "用于补偿不同路径上的固定延时差（如矩阵混音侧多加了输出延时）。"));
    layout->addWidget(new QLabel(QStringLiteral("Delay")), 1, 0);
    layout->addWidget(delayFramesSpin, 1, 1);

    auto *hint = new QLabel(QStringLiteral(
        "各路 In/Out 一一对应，PCM 原样透传；仅把输出时间戳加上 Delay 帧数，用于对齐多路径固定延时。"));
    hint->setWordWrap(true);
    layout->addWidget(hint, 2, 0, 1, 2);

    mainLayout->addWidget(group);
    mainLayout->addStretch(1);
}

AudioDelayInterface::~AudioDelayInterface() = default;
