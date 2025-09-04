//
// Created by Administrator on 2023/12/13.
//

#pragma once
#include <QWidget>
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QDoubleSpinBox"
#include "QGridLayout"

/**
 * @brief 音频输入设备节点界面类
 */
class AudioDeviceInInterface: public QWidget{
Q_OBJECT
public:
    /**
     * @brief 构造函数
     */
    explicit AudioDeviceInInterface(QWidget* parent = nullptr) : QWidget(parent){
        // 设置音量控制范围（分贝）
        volume_spinbox->setRange(-60.0, 20.0);
        volume_spinbox->setValue(0.0);
        volume_spinbox->setSuffix(" dB");
        volume_spinbox->setDecimals(1);
        volume_spinbox->setSingleStep(1.0);
        // 设置设备选择器
        device_selector->addItem("正在加载设备...");
        // 布局
        layout->addWidget(new QLabel("输入设备:"), 0, 0, 1, 1);
        layout->addWidget(device_selector, 0, 1, 1, 2);
        layout->addWidget(new QLabel("音量:"), 1, 0, 1, 1);
        layout->addWidget(volume_spinbox, 1, 1, 1, 2);
        this->setLayout(layout);
        this->setMinimumSize(QSize(100,50));
    }

signals:
    void recordingStarted();
    void recordingStopped();

public:
    QGridLayout *layout = new QGridLayout(this);
    QComboBox *device_selector = new QComboBox();
    QDoubleSpinBox *volume_spinbox = new QDoubleSpinBox();

};



