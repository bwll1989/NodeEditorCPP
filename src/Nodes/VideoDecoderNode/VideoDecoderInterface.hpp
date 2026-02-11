//
// Created by Administrator on 2023/12/13.
//

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSlider>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include <QFileDialog>  // 新增文件对话框支持

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "Common/GUI/Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
///
namespace Nodes
{
    class VideoDecoderInterface: public QWidget{
    public:
        /*构造
         *
         */
        explicit VideoDecoderInterface(QWidget *parent = nullptr) {
            main_layout = new QGridLayout(this);  // 正确初始化网格布局




            main_layout->addWidget(fileSelectComboBox, 0, 0,2,2);

            main_layout->addWidget(playButton, 2, 0,1,2);

            main_layout->addWidget(stopButton, 3, 0,1,2);

            // 进度条和时间显示
            progressSlider->setOrientation(Qt::Horizontal);
            progressSlider->setRange(0, 1000); // 精度 0.1%
            main_layout->addWidget(progressSlider, 4, 0, 1, 2);
            
            timeLabel->setAlignment(Qt::AlignCenter);
            timeLabel->setText("00:00 / 00:00");
            main_layout->addWidget(timeLabel, 5, 0, 1, 2);

            main_layout->addWidget(loopCheckBox, 6, 0,1,1);

            main_layout->addWidget(volumeSlider, 6, 1,1,1);

            main_layout->setContentsMargins(4,2,4,4);
            volumeSlider->setRange(-40, 20);
            volumeSlider->setValue(0);
            volumeSlider->setSingleStep(0.5);
            volumeSlider->setSuffix(" dB");

            this->setLayout(main_layout);
            this->setMinimumSize(QSize(200,50));

        }

    public:
        // 修正后的成员变量
        QGridLayout *main_layout;  // 统一使用网格布局
        SelectorComboBox *fileSelectComboBox = new SelectorComboBox(MediaLibrary::Category::Video,this);
        QPushButton *playButton=new QPushButton("Play");
        QPushButton *stopButton=new QPushButton("Stop");
        QSlider *progressSlider = new QSlider(this);
        QLabel *timeLabel = new QLabel(this);
        FloatDragValueWidget *volumeSlider = new FloatDragValueWidget(this);
        QCheckBox *loopCheckBox = new QCheckBox("Loop");

    };
}


