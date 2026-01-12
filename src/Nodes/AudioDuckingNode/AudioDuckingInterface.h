#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>

namespace Nodes
{
    class AudioDuckingInterface : public QWidget
    {
        Q_OBJECT

    public:
        AudioDuckingInterface(QWidget *parent = nullptr);
        ~AudioDuckingInterface();
        
        QDoubleSpinBox* thresholdSpin;
        QDoubleSpinBox* ratioSpin;
        QDoubleSpinBox* attackSpin;
        QDoubleSpinBox* releaseSpin;
        QDoubleSpinBox* makeupGainSpin;
        QDoubleSpinBox* sidechainGainSpin;
        QDoubleSpinBox* depthSpin;
    };
}

