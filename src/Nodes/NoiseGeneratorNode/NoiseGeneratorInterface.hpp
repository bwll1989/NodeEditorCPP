//
// Created by Administrator on 2023/12/13.
//

#include <QCheckBox>
#include "Common/GUI/Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include <QComboBox>
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include <QGridLayout>
#include <QSpacerItem>

/// 噪音生成器界面类
/// 提供噪音类型选择、音量控制、开始/停止按钮等功能
namespace Nodes
{
    class NoiseGeneratorInterface: public QWidget{
    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit NoiseGeneratorInterface(QWidget *parent = nullptr) {
            main_layout = new QGridLayout(this);
            main_layout->setContentsMargins(4, 2, 4, 4);
            main_layout->setSpacing(6);

            // 初始化噪音类型选择
            noiseTypeCombo->addItem("白噪音");
            noiseTypeCombo->addItem("粉噪音");
            noiseTypeCombo->setCurrentIndex(0);
            // 布局设置（竖向 2*n：左侧标签、右侧控件）
            main_layout->addWidget(new QLabel("噪音类型:"), 0, 0);
            main_layout->addWidget(noiseTypeCombo, 0, 1);
            main_layout->addWidget(new QLabel("音量(dB):"), 1, 0);
            main_layout->addWidget(volumeSlider, 1, 1);
            main_layout->addWidget(startButton, 2, 0, 1, 2);
            startButton->setCheckable(true);
            // main_layout->addWidget(stopButton, 3, 0, 1, 2);
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 4, 0, 1, 2);
            main_layout->setRowStretch(4, 1);
            main_layout->setColumnStretch(0, 1);
            main_layout->setColumnStretch(1, 2);
            
            // 音量滑块设置
            volumeSlider->setRange(-40, 20);
            volumeSlider->setValue(0);
            volumeSlider->setSingleStep(0.5);
            volumeSlider->setSuffix(" dB");
            
            // 按钮初始状态
            // stopButton->setEnabled(false);
            
            this->setLayout(main_layout);
            this->setMinimumSize(QSize(220, 120));
        }

    public:
        // 界面组件
        QGridLayout *main_layout;
        QComboBox *noiseTypeCombo = new QComboBox(this);
        QPushButton *startButton = new QPushButton("启动");
        // QPushButton *stopButton = new QPushButton("停止生成");
        FloatDragValueWidget *volumeSlider = new FloatDragValueWidget(this);
    };
}


