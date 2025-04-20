//
// Created by Administrator on 2023/12/13.
//

#pragma once
#include <QWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>

class AudioDeviceOutInterface : public QWidget {
    Q_OBJECT

public:
    explicit AudioDeviceOutInterface(QWidget* parent = nullptr) : QWidget(parent) {
        mainLayout = new QVBoxLayout(this);
        
        deviceSelector = new QComboBox(this);
        deviceSelector->setToolTip(tr("Select Audio Output Device"));
        
        bufferCountLabel = new QLabel("Buffered: 0", this);
        bufferCountLabel->setAlignment(Qt::AlignCenter);
        
        mainLayout->addWidget(new QLabel("Output Device:", this));
        mainLayout->addWidget(deviceSelector);
        mainLayout->addWidget(bufferCountLabel);
        
        setLayout(mainLayout);
    }

    QVBoxLayout* mainLayout;
    QComboBox* deviceSelector;
    QLabel* bufferCountLabel;
};

