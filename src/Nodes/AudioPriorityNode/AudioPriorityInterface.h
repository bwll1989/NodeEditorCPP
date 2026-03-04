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
    class AudioPriorityInterface : public QWidget
    {
        Q_OBJECT

    public:
        AudioPriorityInterface(QWidget *parent = nullptr);
        ~AudioPriorityInterface();
        
        FloatDragValueWidget* thresholdSpin;
        FloatDragValueWidget* ratioSpin;
        FloatDragValueWidget* attackSpin;
        FloatDragValueWidget* releaseSpin;
        FloatDragValueWidget* makeupGainSpin;
        FloatDragValueWidget* sidechainGainSpin;
        FloatDragValueWidget* depthSpin;
    };
}

