#include "AudioPriorityInterface.h"
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>

using namespace Nodes;

AudioPriorityInterface::AudioPriorityInterface(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    
    QGroupBox *group = new QGroupBox("Ducking Parameters", this);
    auto *layout = new QGridLayout(group);
    
    // Threshold
    layout->addWidget(new QLabel("Threshold (dB):"), 0, 0);
    thresholdSpin = new FloatDragValueWidget();
    thresholdSpin->setRange(-60.0, 0.0);
    thresholdSpin->setValue(-20.0);
    thresholdSpin->setSuffix(" dB");
    layout->addWidget(thresholdSpin, 0, 1);
    
    // Ratio
    layout->addWidget(new QLabel("Ratio:"), 1, 0);
    ratioSpin = new FloatDragValueWidget();
    ratioSpin->setRange(1.0, 10000.0);
    ratioSpin->setValue(4.0);
    ratioSpin->setSingleStep(0.5);
    layout->addWidget(ratioSpin, 1, 1);
    
    // Attack
    layout->addWidget(new QLabel("Attack (ms):"), 2, 0);
    attackSpin = new FloatDragValueWidget();
    attackSpin->setRange(0.1, 1000.0);
    attackSpin->setValue(10.0);
    attackSpin->setSuffix(" ms");
    layout->addWidget(attackSpin, 2, 1);
    
    // Release
    layout->addWidget(new QLabel("Release (ms):"), 3, 0);
    releaseSpin = new FloatDragValueWidget();
    releaseSpin->setRange(10.0, 5000.0);
    releaseSpin->setValue(100.0);
    releaseSpin->setSuffix(" ms");
    layout->addWidget(releaseSpin, 3, 1);

    // Makeup Gain
    layout->addWidget(new QLabel("Makeup Gain (dB):"), 4, 0);
    makeupGainSpin = new FloatDragValueWidget();
    makeupGainSpin->setRange(0.0, 24.0);
    makeupGainSpin->setValue(0.0);
    makeupGainSpin->setSuffix(" dB");
    layout->addWidget(makeupGainSpin, 4, 1);

    // Ducking Depth
    layout->addWidget(new QLabel("Depth (dB):"), 5, 0);
    depthSpin = new FloatDragValueWidget();
    depthSpin->setRange(0.0, 96.0);
    depthSpin->setValue(24.0);
    depthSpin->setSuffix(" dB");
    layout->addWidget(depthSpin, 5, 1);

    // Sidechain Gain
    layout->addWidget(new QLabel("SC Gain (dB):"), 6, 0);
    sidechainGainSpin = new FloatDragValueWidget();
    sidechainGainSpin->setRange(0.0, 48.0);
    sidechainGainSpin->setValue(0.0);
    sidechainGainSpin->setSuffix(" dB");
    layout->addWidget(sidechainGainSpin, 6, 1);
    mainLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    mainLayout->addWidget(group);
}

AudioPriorityInterface::~AudioPriorityInterface()
{}
