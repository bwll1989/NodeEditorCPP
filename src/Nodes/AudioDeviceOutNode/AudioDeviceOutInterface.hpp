//
// Created by Administrator on 2023/12/13.
//

#pragma once
#include <QWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QSet>
#include <QMap>

namespace Nodes
{
    class AudioDeviceOutInterface : public QWidget {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，添加驱动类型筛选功能
         */
        explicit AudioDeviceOutInterface(QWidget* parent = nullptr) : QWidget(parent) {
            mainLayout = new QVBoxLayout(this);

            // 驱动选择器
            driverSelector = new QComboBox(this);
            driverSelector->setToolTip(tr("Select Audio Driver Type"));
            driverSelector->addItem("All Drivers", -1);  // 显示所有驱动
            driverSelector->addItem("WASAPI", 0);
            driverSelector->addItem("DirectSound", 1);
            driverSelector->addItem("ASIO", 2);
            driverSelector->addItem("MME", 3);
            driverSelector->setCurrentIndex(0);  // 默认显示所有驱动

            // 设备选择器
            deviceSelector = new QComboBox(this);
            deviceSelector->setToolTip(tr("Select Audio Output Device"));

            // 布局
            mainLayout->addWidget(new QLabel("Audio Driver:", this));
            mainLayout->addWidget(driverSelector);
            mainLayout->addWidget(new QLabel("Output Device:", this));
            mainLayout->addWidget(deviceSelector);
            setLayout(mainLayout);
        }
        
    public:
        QVBoxLayout* mainLayout;
        QComboBox* deviceSelector;
        QComboBox* driverSelector;
    };
}