#include "AudioGateInterface.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

using namespace Nodes;

AudioGateInterface::AudioGateInterface(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    auto *group = new QGroupBox(QStringLiteral("Audio Gate"), this);
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
    depthSpin->setRange(0.0, 60.0);
    depthSpin->setDecimals(1);
    depthSpin->setSingleStep(1.0);
    depthSpin->setValue(60.0);
    depthSpin->setSuffix(QStringLiteral(" dB"));
    addFloatRow(2, QStringLiteral("Depth"), depthSpin);

    attackSpin = new FloatDragValueWidget();
    attackSpin->setRange(0.1, 10000.0);
    attackSpin->setDecimals(1);
    attackSpin->setSingleStep(0.1);
    attackSpin->setValue(1.0);
    attackSpin->setSuffix(QStringLiteral(" ms"));
    addFloatRow(3, QStringLiteral("Attack Time"), attackSpin);

    holdSpin = new FloatDragValueWidget();
    holdSpin->setRange(10.0, 10000.0);
    holdSpin->setDecimals(1);
    holdSpin->setSingleStep(10.0);
    holdSpin->setValue(100.0);
    holdSpin->setSuffix(QStringLiteral(" ms"));
    addFloatRow(4, QStringLiteral("Hold Time"), holdSpin);

    releaseSpin = new FloatDragValueWidget();
    releaseSpin->setRange(10.0, 10000.0);
    releaseSpin->setDecimals(1);
    releaseSpin->setSingleStep(10.0);
    releaseSpin->setValue(200.0);
    releaseSpin->setSuffix(QStringLiteral(" ms"));
    addFloatRow(5, QStringLiteral("Release Time"), releaseSpin);

    auto *hint = new QLabel(QStringLiteral("超过 Threshold 时无损通过；低于 Threshold 时按 Depth 衰减。各通道独立检测。"));
    hint->setWordWrap(true);
    layout->addWidget(hint, 6, 0, 1, 2);

    mainLayout->addWidget(group);
    mainLayout->addStretch(1);
}

AudioGateInterface::~AudioGateInterface() = default;
