#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>

namespace Nodes
{
    class AudioCrossFaderInterface : public QWidget
    {
        Q_OBJECT

    public:
        AudioCrossFaderInterface(QWidget *parent = nullptr);
        ~AudioCrossFaderInterface();
        
        QDoubleSpinBox* mixSpin;          // 0.0 ~ 1.0
        QDoubleSpinBox* fadeDurationSpin; // ms
        QComboBox* actionCombo;           // A->B / B->A / Reset
    };
}

