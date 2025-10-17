#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
namespace Nodes
{
    class AudioAnalysisInterface : public QWidget
    {
        Q_OBJECT

    public:
        AudioAnalysisInterface(QWidget *parent = nullptr);
        ~AudioAnalysisInterface();
        QPushButton* mResetButton;
    };
}

