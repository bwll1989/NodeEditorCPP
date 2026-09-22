//
// Created by Administrator on 2023/12/13.
//

#include <QCheckBox>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/AudioWaveformWidget/AudioWaveformWidget.hpp"
namespace Nodes
{
    class AudioDecoderInterface: public QWidget{
    public:
        /*构造
         *
         */
        explicit AudioDecoderInterface(QWidget *parent = nullptr) {
            main_layout = new QGridLayout(this);

            // 将播放/停止合并为一个可勾选按钮
            playButton->setCheckable(true);

            main_layout->addWidget(fileSelectComboBox, 0, 0, 2, 2);

            main_layout->addWidget(playButton, 2, 0, 1, 2);

           

            main_layout->addWidget(volumeSlider, 3, 0, 1, 1);
            main_layout->addWidget(loopCheckBox, 3, 1, 1, 1);

            main_layout->setContentsMargins(4, 2, 4, 4);
            volumeSlider->setRange(-100, 20);
            volumeSlider->setValue(0);
            volumeSlider->setSingleStep(0.5);
            volumeSlider->setSuffix(" dB");
             // 音频波形（取代原 progressSlider + timeLabel 两行）
            waveformWidget->setMinimumHeight(90);
            waveformWidget->setPlaceholderText(QStringLiteral("请先选择音频文件"));
            main_layout->addWidget(waveformWidget, 4, 0, 1, 2);
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 5, 0, 1, 2);
            main_layout->setRowStretch(5, 1);
            this->setLayout(main_layout);
            this->setMinimumSize(QSize(220, 200));

        }

    public:
        // 修正后的成员变量
        QGridLayout *main_layout;
        SelectorComboBox *fileSelectComboBox = new SelectorComboBox(MediaLibrary::Category::Audio, this);
        QPushButton *playButton = new QPushButton("Play");

        AudioWaveformWidget *waveformWidget = new AudioWaveformWidget(this); ///< 音频波形，同时承担进度条+时间显示
        FloatDragValueWidget *volumeSlider = new FloatDragValueWidget(this);
        QCheckBox *loopCheckBox = new QCheckBox("Loop");

    };
}


