//
// Created by Administrator on 2023/12/13.
//

#include <QCheckBox>
#include <QSlider>
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include <QFileDialog>  // 新增文件对话框支持


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

            // 初始化文件显示框（设为只读）
            fileDisplay->setAlignment(Qt::AlignCenter);

            main_layout->addWidget(fileDisplay, 0, 0, 2, 4);
            fileDisplay->setStyleSheet("background-color:rgb(145, 48, 48);");
            // 按钮布局
            main_layout->addWidget(fileSelectButton, 2, 0,1,4);

            main_layout->addWidget(playButton, 3, 0,1,2);

            main_layout->addWidget(stopButton, 3, 2,1,2);

            main_layout->addWidget(loopCheckBox, 4, 0,1,1);

            main_layout->addWidget(new QLabel("volume:"), 5, 0,1,1);

            main_layout->addWidget(volumeSlider, 5, 1,1,3);

            main_layout->setContentsMargins(4,2,4,4);
            volumeSlider->setRange(0, 100);
            volumeSlider->setValue(50);

            this->setLayout(main_layout);
            this->setMinimumSize(QSize(200,50));
        }

    public:
        void selectFile() {
            // 完善文件对话框参数
            QString path = QFileDialog::getOpenFileName(this, "选择音频文件", "",
                "音频文件 (*.mp3 *.wav *.flac *.aac)");

            if (!path.isEmpty()) {
                fileDisplay->setText(path);
                // ... 保持原有解码初始化逻辑 ...
            }
        }
    public:
        // 修正后的成员变量
        QGridLayout *main_layout;  // 统一使用网格布局
        QLabel *fileDisplay = new QLabel(this);
        QPushButton *fileSelectButton=new QPushButton("Select");
        QPushButton *playButton=new QPushButton("Play");
        QPushButton *stopButton=new QPushButton("Stop");
        QSlider *volumeSlider = new QSlider(Qt::Horizontal);
        QCheckBox *loopCheckBox = new QCheckBox("Loop");

    };
}


