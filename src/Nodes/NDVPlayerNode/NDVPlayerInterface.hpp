//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include <QLineEdit>
#include "QWidget"
#include "QLabel"
#include "QGridLayout"
#include <QSpinBox>
#include <QPushButton>

namespace Nodes
{
    /**
     * @brief NDV播放器节点界面
     */
    class NDVPlayerInterface: public QWidget{
        Q_OBJECT
    public:
        explicit NDVPlayerInterface(QWidget *parent = nullptr){

            main_layout->addWidget(new QLabel("Player ID:", this), 0, 0, 1, 1);
            main_layout->addWidget(PlayerID, 0, 1, 1, 1);
            PlayerID->setRange(1, 99);
            PlayerID->setValue(1);

            // 文件控制
            main_layout->addWidget(new QLabel("File Index:", this), 1, 0, 1, 1);
            main_layout->addWidget(FileIndex, 1, 1, 1, 1);
            FileIndex->setRange(0, 100);
            FileIndex->setValue(0);

            // 播放控制按钮
            main_layout->addWidget(Play, 2, 0, 1, 2);
            main_layout->addWidget(LoopPlay, 3, 0, 1, 2);
            main_layout->addWidget(Stop, 4, 0, 1, 2);
            
            // 文件导航
            main_layout->addWidget(Prev, 5, 0, 1, 1);
            main_layout->addWidget(Next, 5, 1, 1, 1);

            main_layout->setColumnStretch(0, 1);
            main_layout->setColumnStretch(1, 1);
            main_layout->setSpacing(5);
            main_layout->setContentsMargins(5, 5, 5, 5);

            this->setLayout(main_layout);
        }

    public:
        QGridLayout *main_layout = new QGridLayout(this);
        QSpinBox *PlayerID = new QSpinBox(this);
        QSpinBox *FileIndex = new QSpinBox(this);
        QPushButton *Play = new QPushButton("Play", this);
        QPushButton *LoopPlay = new QPushButton("Loop Play", this);
        QPushButton *Stop = new QPushButton("Stop", this);
        QPushButton *Next = new QPushButton("Next", this);
        QPushButton *Prev = new QPushButton("Prev", this);
    };
}
