#include "AudioPriorityInterface.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

using namespace Nodes;

AudioPriorityInterface::AudioPriorityInterface(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    auto *group = new QGroupBox(QStringLiteral("Priority Ducker"), this);
    auto *layout = new QGridLayout(group);

    auto addFloatRow = [layout](int row, const QString &label, FloatDragValueWidget *spin) {
        layout->addWidget(new QLabel(label), row, 0);
        layout->addWidget(spin, row, 1);
    };

    channelsSpin = new IntDragValueWidget();
    channelsSpin->setRange(kMinChannels, kMaxChannels);
    channelsSpin->setSingleStep(1);
    channelsSpin->setValue(kDefaultChannels);
    layout->addWidget(new QLabel(QStringLiteral("Channels")), 0, 0);
    layout->addWidget(channelsSpin, 0, 1);

    thresholdSpin = new FloatDragValueWidget();
    thresholdSpin->setRange(-60.0, 20.0);
    thresholdSpin->setDecimals(1);
    thresholdSpin->setSingleStep(1.0);
    thresholdSpin->setValue(-40.0);
    thresholdSpin->setSuffix(QStringLiteral(" dB"));
    addFloatRow(1, QStringLiteral("Threshold Level"), thresholdSpin);

    depthSpin = new FloatDragValueWidget();
    depthSpin->setRange(0.0, 100.0);
    depthSpin->setDecimals(1);
    depthSpin->setSingleStep(1.0);
    depthSpin->setValue(20.0);
    depthSpin->setSuffix(QStringLiteral(" dB"));
    addFloatRow(2, QStringLiteral("Depth"), depthSpin);

    priorityGainSpin = new FloatDragValueWidget();
    priorityGainSpin->setRange(-100.0, 20.0);
    priorityGainSpin->setDecimals(1);
    priorityGainSpin->setSingleStep(0.5);
    priorityGainSpin->setValue(0.0);
    priorityGainSpin->setSuffix(QStringLiteral(" dB"));
    addFloatRow(3, QStringLiteral("Priority Gain"), priorityGainSpin);

    attackSpin = new FloatDragValueWidget();
    attackSpin->setRange(5.0, 10000.0);
    attackSpin->setDecimals(1);
    attackSpin->setSingleStep(1.0);
    attackSpin->setValue(10.0);
    attackSpin->setSuffix(QStringLiteral(" ms"));
    addFloatRow(4, QStringLiteral("Attack Time"), attackSpin);

    holdSpin = new FloatDragValueWidget();
    holdSpin->setRange(1.0, 30000.0);
    holdSpin->setDecimals(1);
    holdSpin->setSingleStep(10.0);
    holdSpin->setValue(200.0);
    holdSpin->setSuffix(QStringLiteral(" ms"));
    addFloatRow(5, QStringLiteral("Hold Time"), holdSpin);

    releaseSpin = new FloatDragValueWidget();
    releaseSpin->setRange(10.0, 10000.0);
    releaseSpin->setDecimals(1);
    releaseSpin->setSingleStep(10.0);
    releaseSpin->setValue(1000.0);
    releaseSpin->setSuffix(QStringLiteral(" ms"));
    addFloatRow(6, QStringLiteral("Release Time"), releaseSpin);

    auto *hint = new QLabel(QStringLiteral("Channels 含 Priority；最后一路为 Priority，超过阈值后压低其余通道并混入各输出。"));
    hint->setWordWrap(true);
    layout->addWidget(hint, 7, 0, 1, 2);

    mainLayout->addWidget(group);
    mainLayout->addStretch(1);
}

AudioPriorityInterface::~AudioPriorityInterface() = default;
