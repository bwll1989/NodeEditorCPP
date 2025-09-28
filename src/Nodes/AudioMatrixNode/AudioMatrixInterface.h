#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include "Elements/MartixWidget/MatrixWidget.h"
#include "Eigen/Core"
namespace Nodes
{
    class AudioMatrixInterface : public QWidget
    {
        Q_OBJECT

    public:
        AudioMatrixInterface(int col=2,int row=2,QWidget *parent = nullptr);
        ~AudioMatrixInterface();
        MatrixWidget* mMatrixWidget;
        QPushButton* mResetButton;
        int cols;
        int rows;
    };
}

