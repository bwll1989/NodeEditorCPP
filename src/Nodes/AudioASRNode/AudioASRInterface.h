#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include "Elements/MartixWidget/MatrixWidget.h"
#include "Eigen/Core"
namespace Nodes
{
    class AudioASRInterface : public QWidget
    {
        Q_OBJECT

    public:
        AudioASRInterface(int col=2,int row=2,QWidget *parent = nullptr);
        ~AudioASRInterface();
        MatrixWidget* mMatrixWidget;
        QPushButton* mResetButton;
        int cols;
        int rows;
    };
}

