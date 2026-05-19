//
// Created by Administrator on 2025/07/13.
//
#pragma once
#include <QComboBox>

#include "QWidget"
#include "QLayout"
#include <QGridLayout>
#include <QSpacerItem>
#include <QSpinBox>
#include <QVariantMap>
#include <QPushButton>

namespace Nodes
{
class YoloDetectionONNXInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit YoloDetectionONNXInterface(QWidget *parent = nullptr) {
            main_layout = new QGridLayout(this);
            main_layout->setContentsMargins(0, 0, 0, 0);
            main_layout->setSpacing(6);

            EnableBtn=new QPushButton("Enable");
            EnableBtn->setCheckable(true);
            main_layout->addWidget(EnableBtn, 0, 0, 1, 2);
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 0, 1, 2);
            main_layout->setRowStretch(1, 1);
            main_layout->setColumnStretch(0, 1);
            main_layout->setColumnStretch(1, 2);
            this->setLayout(main_layout);
            // this->setFixedSize(400,200);

        };

    public:
        QGridLayout *main_layout;
        QPushButton *EnableBtn;

    };
}

