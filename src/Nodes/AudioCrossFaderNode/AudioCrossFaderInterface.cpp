#include "AudioCrossFaderInterface.h"
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>

using namespace Nodes;

AudioCrossFaderInterface::AudioCrossFaderInterface(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QGridLayout(this);
    
    // Mix
    mainLayout->addWidget(new QLabel("Mix (0=A, 1=B):"), 0, 0);
    mixSpin = new QDoubleSpinBox();
    mixSpin->setRange(0.0, 1.0);
    mixSpin->setDecimals(3);
    mixSpin->setSingleStep(0.01);
    mixSpin->setValue(0.0);
    mainLayout->addWidget(mixSpin, 0, 1);

    // Note: Direction and Curve comboboxes removed per request
    
    // Fade Duration
    mainLayout->addWidget(new QLabel("Fade Time (ms):"), 1, 0);
    fadeDurationSpin = new QDoubleSpinBox();
    fadeDurationSpin->setRange(10.0, 600000.0);
    fadeDurationSpin->setDecimals(1);
    fadeDurationSpin->setSingleStep(10.0);
    fadeDurationSpin->setValue(2000.0);
    fadeDurationSpin->setSuffix(" ms");
    mainLayout->addWidget(fadeDurationSpin, 1, 1);
    
    // Buttons
    fadeAToBButton = new QPushButton("Start A -> B");
    fadeBToAButton = new QPushButton("Start B -> A");
    mainLayout->addWidget(fadeAToBButton, 2, 0);
    mainLayout->addWidget(fadeBToAButton, 2, 1);
    
    // Reset
    resetButton = new QPushButton("Reset Mix");
    mainLayout->addWidget(resetButton, 3, 0, 1, 2);
    
    setMinimumWidth(250);
}

AudioCrossFaderInterface::~AudioCrossFaderInterface()
{}
