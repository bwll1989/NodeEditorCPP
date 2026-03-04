#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

namespace Nodes
{
    class AudioCrossFaderInterface : public QWidget
    {
        Q_OBJECT

    public:
        AudioCrossFaderInterface(QWidget *parent = nullptr);
        ~AudioCrossFaderInterface();
        
        FloatDragValueWidget* mixSpin;          // 0.0 ~ 1.0
        FloatDragValueWidget* fadeDurationSpin; // ms
        QComboBox* actionCombo;           // A->B / B->A / Reset
    };
}

