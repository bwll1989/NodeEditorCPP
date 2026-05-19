//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include <QLineEdit>
#include "QWidget"
#include "QLabel"
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QPushButton>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief NDV播放器节点界面
     */
    class NDVPlayerInterface: public QWidget{
        Q_OBJECT
    public:
        explicit NDVPlayerInterface(QWidget *parent = nullptr){

            auto *layout = new QVBoxLayout(this);
            layout->setContentsMargins(5, 5, 5, 5);
            layout->setSpacing(5);

            const auto addRow = [this, layout](const QString& labelText, QWidget* editor) {
                auto *row = new QWidget(this);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);

                auto *label = new QLabel(labelText, row);
                rowLayout->addWidget(label, 0);
                rowLayout->addWidget(editor, 1);

                layout->addWidget(row);
            };

            addRow("Player ID:", PlayerID);
            PlayerID->setRange(1, 99);
            PlayerID->setValue(1);

            // 文件控制
            addRow("File Index:", FileIndex);
            FileIndex->setRange(0, 100);
            FileIndex->setValue(0);

            // 播放控制按钮
            layout->addWidget(Play);
            layout->addWidget(LoopPlay);
            layout->addWidget(Stop);
            
            // 文件导航
            auto *navRow = new QWidget(this);
            auto *navLayout = new QHBoxLayout(navRow);
            navLayout->setContentsMargins(0, 0, 0, 0);
            navLayout->setSpacing(8);
            navLayout->addWidget(Prev);
            navLayout->addWidget(Next);
            layout->addWidget(navRow);

            layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        }

    public:
        IntDragValueWidget *PlayerID = new IntDragValueWidget(this);
        IntDragValueWidget *FileIndex = new IntDragValueWidget(this);
        QPushButton *Play = new QPushButton("Play", this);
        QPushButton *LoopPlay = new QPushButton("Loop Play", this);
        QPushButton *Stop = new QPushButton("Stop", this);
        QPushButton *Next = new QPushButton("Next", this);
        QPushButton *Prev = new QPushButton("Prev", this);
    };
}
