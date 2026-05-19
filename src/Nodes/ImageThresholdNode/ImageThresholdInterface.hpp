//
// Created by Administrator on 2025/07/13.
//
#pragma once
#include "QWidget"
#include "QLayout"
#include <QSpinBox>
#include <QVariantMap>
#include <QPushButton>
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
namespace Nodes
{
class ImageThresholdInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit ImageThresholdInterface(QWidget *parent = nullptr) {
            main_layout=new QGridLayout();
            methodEdit=new QComboBox();
            methodEdit->addItem("THRESH_BINARY");
            methodEdit->addItem("THRESH_BINARY_INV");
            methodEdit->addItem("THRESH_TRUNC");
            methodEdit->addItem("THRESH_TOZERO");
            methodEdit->addItem("THRESH_TOZERO_INV");
            threshEdit=new IntDragValueWidget();
            threshEdit->setRange(0,255);
            threshEdit->setValue(128);
            maxvalEdit=new IntDragValueWidget();
            maxvalEdit->setRange(0,255);
            maxvalEdit->setValue(255);
            main_layout->addWidget(new QLabel("Threshold"),0,0,1,1);
            main_layout->addWidget(threshEdit,0,1,1,1);
            main_layout->addWidget(new QLabel("MaxVal"),0,2,1,1);
            main_layout->addWidget(maxvalEdit,0,3,1,1);
            main_layout->addWidget(methodEdit,1,0,1,4);

            main_layout->setColumnStretch(0,1);
            main_layout->setColumnStretch(1,2);
            main_layout->setColumnStretch(2,1);
            main_layout->setColumnStretch(3,2);
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 4, 0, 1, 2);
            main_layout->setRowStretch(4, 1);
            this->setLayout(main_layout);
            // this->setFixedSize(400,200);

        };

    public:
        QGridLayout *main_layout;
        QComboBox *methodEdit;
        IntDragValueWidget *threshEdit;
        IntDragValueWidget *maxvalEdit;


    // 添加事件过滤器声明



    };
}

