#include "AudioDuckingInterface.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

using namespace Nodes;

AudioDuckingInterface::AudioDuckingInterface(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    auto *group = new QGroupBox(QStringLiteral("Audio Ducking"), this);
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

    depthSpin = new FloatDragValueWidget();
    depthSpin->setRange(0.0, 100.0);
    depthSpin->setDecimals(1);
    depthSpin->setSingleStep(1.0);
    depthSpin->setValue(20.0);
    depthSpin->setSuffix(QStringLiteral(" dB"));
    addFloatRow(1, QStringLiteral("Depth"), depthSpin);

    attackSpin = new FloatDragValueWidget();
    attackSpin->setRange(5.0, 10000.0);
    attackSpin->setDecimals(1);
    attackSpin->setSingleStep(1.0);
    attackSpin->setValue(10.0);
    attackSpin->setSuffix(QStringLiteral(" ms"));
    addFloatRow(2, QStringLiteral("Attack Time"), attackSpin);

    holdSpin = new FloatDragValueWidget();
    holdSpin->setRange(1.0, 30000.0);
    holdSpin->setDecimals(1);
    holdSpin->setSingleStep(10.0);
    holdSpin->setValue(200.0);
    holdSpin->setSuffix(QStringLiteral(" ms"));
    addFloatRow(3, QStringLiteral("Hold Time"), holdSpin);

    releaseSpin = new FloatDragValueWidget();
    releaseSpin->setRange(10.0, 10000.0);
    releaseSpin->setDecimals(1);
    releaseSpin->setSingleStep(10.0);
    releaseSpin->setValue(1000.0);
    releaseSpin->setSuffix(QStringLiteral(" ms"));
    addFloatRow(4, QStringLiteral("Release Time"), releaseSpin);

    auto *hint = new QLabel(QStringLiteral("最后一路 Duck（Variable）为真时压低各音频通道；支持多声道交错 PCM。"));
    hint->setWordWrap(true);
    layout->addWidget(hint, 5, 0, 1, 2);

    mainLayout->addWidget(group);
    mainLayout->addStretch(1);
}

AudioDuckingInterface::~AudioDuckingInterface() = default;
