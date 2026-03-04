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
    class AudioDuckingInterface : public QWidget
    {
        Q_OBJECT

    public:
        AudioDuckingInterface(QWidget *parent = nullptr);
        ~AudioDuckingInterface();
        
        FloatDragValueWidget* thresholdSpin;
        FloatDragValueWidget* ratioSpin;
        FloatDragValueWidget* attackSpin;
        FloatDragValueWidget* releaseSpin;
        FloatDragValueWidget* makeupGainSpin;
        FloatDragValueWidget* sidechainGainSpin;
        FloatDragValueWidget* depthSpin;
    };
}

