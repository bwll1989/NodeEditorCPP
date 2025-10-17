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
class YoloPoseONNXInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit YoloPoseONNXInterface(QWidget *parent = nullptr) {
            main_layout=new QGridLayout();


            main_layout->setColumnStretch(0,1);
            this->setLayout(main_layout);
            // this->setFixedSize(400,200);

        };

    public:
        QGridLayout *main_layout;


    };
}

