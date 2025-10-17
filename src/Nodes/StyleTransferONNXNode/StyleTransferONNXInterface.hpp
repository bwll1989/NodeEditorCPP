//
// Created by Administrator on 2025/07/13.
//
#pragma once
#include <QComboBox>

#include "QWidget"
#include "QLayout"
#include <QSpinBox>
#include <QVariantMap>
#include <QPushButton>

namespace Nodes
{
class YoloDetectionONNXInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit YoloDetectionONNXInterface(QWidget *parent = nullptr) {
            main_layout=new QGridLayout();

            EnableBtn=new QPushButton("Enable");
            EnableBtn->setCheckable(true);
            main_layout->addWidget(EnableBtn,0,0,1,3);
            this->setLayout(main_layout);
            // this->setFixedSize(400,200);

        };

    public:
        QGridLayout *main_layout;
        QPushButton *EnableBtn;

    };
}

