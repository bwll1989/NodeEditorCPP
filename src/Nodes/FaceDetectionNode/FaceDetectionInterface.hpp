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
class FaceDetectionInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit FaceDetectionInterface(QWidget *parent = nullptr) {
            main_layout=new QGridLayout();

            // 文本类别输入

            // 置信度阈值
            confSpin = new QDoubleSpinBox();
            confSpin->setMinimum(0.0);
            confSpin->setMaximum(1.0);
            confSpin->setSingleStep(0.05);
            confSpin->setDecimals(2);
            confSpin->setValue(0.25);
            main_layout->addWidget(new QLabel("置信度阈值:"),0,0,1,1);
            main_layout->addWidget(confSpin,0,1,1,2);

            // NMS 阈值
            nmsSpin = new QDoubleSpinBox();
            nmsSpin->setMinimum(0.0);
            nmsSpin->setMaximum(1.0);
            nmsSpin->setSingleStep(0.05);
            nmsSpin->setDecimals(2);
            nmsSpin->setValue(0.45);
            main_layout->addWidget(new QLabel("NMS阈值:"),1,0,1,1);
            main_layout->addWidget(nmsSpin,1,1,1,2);

            main_layout->setColumnStretch(0,1);
            this->setLayout(main_layout);
            // this->setFixedSize(400,200);

        };

    public:
        QGridLayout *main_layout;

        QDoubleSpinBox *confSpin;
        QDoubleSpinBox *nmsSpin;


    };
}

