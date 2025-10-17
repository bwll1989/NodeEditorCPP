#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
namespace Nodes
{
    class AudioIntentInterface : public QWidget
    {
        Q_OBJECT

    public:
        AudioIntentInterface(QWidget *parent = nullptr);
        ~AudioIntentInterface();
        QPushButton* mResetButton;
    };
}

