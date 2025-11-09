//
// Created by Administrator on 2023/12/13.
//

#include <QCheckBox>
#include <QDoubleSpinBox>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include <QFileDialog>  // 新增文件对话框支持

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
///
namespace Nodes
{
    class AudioDecoderInterface: public QWidget{
    public:
        /*构造
         *
         */
        explicit AudioDecoderInterface(QWidget *parent = nullptr) {
            main_layout = new QGridLayout(this);  // 正确初始化网格布局




            main_layout->addWidget(fileSelectComboBox, 0, 0,2,2);

            main_layout->addWidget(playButton, 2, 0,1,2);

            main_layout->addWidget(stopButton, 3, 0,1,2);

            main_layout->addWidget(loopCheckBox, 4, 0,1,1);

            main_layout->addWidget(volumeSlider, 4, 1,1,1);

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
        SelectorComboBox *fileSelectComboBox = new SelectorComboBox(MediaLibrary::Category::Audio,this);
        QPushButton *playButton=new QPushButton("Play");
        QPushButton *stopButton=new QPushButton("Stop");
        QDoubleSpinBox *volumeSlider = new QDoubleSpinBox(this);
        QCheckBox *loopCheckBox = new QCheckBox("Loop");

    };
}


