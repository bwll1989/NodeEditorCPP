#include "AudioCrossFaderInterface.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace Nodes;

AudioCrossFaderInterface::AudioCrossFaderInterface(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    auto *group = new QGroupBox(QStringLiteral("Audio Cross Fader"), this);
    auto *layout = new QGridLayout(group);

    channelsSpin = new IntDragValueWidget();
    channelsSpin->setRange(kMinChannels, kMaxChannels);
    channelsSpin->setSingleStep(1);
    channelsSpin->setValue(kDefaultChannels);
    layout->addWidget(new QLabel(QStringLiteral("Channels")), 0, 0);
    layout->addWidget(channelsSpin, 0, 1);

    layout->addWidget(new QLabel(QStringLiteral("Mix (0=A, 1=B):")), 1, 0);
    mixSpin = new FloatDragValueWidget();
    mixSpin->setRange(0.0, 1.0);
    mixSpin->setDecimals(3);
    mixSpin->setSingleStep(0.01);
    mixSpin->setValue(0.0); // 默认输出 A，不混音
    layout->addWidget(mixSpin, 1, 1);

    layout->addWidget(new QLabel(QStringLiteral("Fade Time (ms):")), 2, 0);
    fadeDurationSpin = new FloatDragValueWidget();
    fadeDurationSpin->setRange(10.0, 600000.0);
    fadeDurationSpin->setDecimals(1);
    fadeDurationSpin->setSingleStep(10.0);
    fadeDurationSpin->setValue(2000.0);
    fadeDurationSpin->setSuffix(QStringLiteral(" ms"));
    layout->addWidget(fadeDurationSpin, 2, 1);

    layout->addWidget(new QLabel(QStringLiteral("Action:")), 3, 0);
    auto *actionRow = new QWidget(group);
    auto *actionLayout = new QHBoxLayout(actionRow);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(6);

    fadeToBButton = new QPushButton(QStringLiteral("A -> B"), actionRow);
    fadeToAButton = new QPushButton(QStringLiteral("B -> A"), actionRow);
    fadeToBButton->setCheckable(true);
    fadeToAButton->setCheckable(true);

    actionGroup = new QButtonGroup(this);
    actionGroup->setExclusive(true);
    actionGroup->addButton(fadeToAButton, kActionToA);
    actionGroup->addButton(fadeToBButton, kActionToB);
    fadeToAButton->setChecked(true); // 默认输出 A

    actionLayout->addWidget(fadeToBButton);
    actionLayout->addWidget(fadeToAButton);
    layout->addWidget(actionRow, 3, 1);

    auto *hint = new QLabel(QStringLiteral("默认输出 A；SWTCH B=true→B，false→A。Channels=N：A1…AN / B1…BN + SWTCH B → Out 1…N。"));
    hint->setWordWrap(true);
    layout->addWidget(hint, 4, 0, 1, 2);

    mainLayout->addWidget(group);
    mainLayout->addStretch(1);
    setMinimumWidth(250);
}

AudioCrossFaderInterface::~AudioCrossFaderInterface() = default;
