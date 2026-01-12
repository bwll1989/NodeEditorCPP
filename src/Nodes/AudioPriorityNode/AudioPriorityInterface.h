#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>

namespace Nodes
{
    class AudioPriorityInterface : public QWidget
    {
        Q_OBJECT

    public:
        AudioPriorityInterface(QWidget *parent = nullptr);
        ~AudioPriorityInterface();
        
        QDoubleSpinBox* thresholdSpin;
        QDoubleSpinBox* ratioSpin;
        QDoubleSpinBox* attackSpin;
        QDoubleSpinBox* releaseSpin;
        QDoubleSpinBox* makeupGainSpin;
        QDoubleSpinBox* sidechainGainSpin;
        QDoubleSpinBox* depthSpin;
    };
}

