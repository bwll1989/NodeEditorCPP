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
     * @brief NDV控制节点界面 - 简化版，只包含网络配置
     */
    class NDVServerInterface: public QWidget{
        Q_OBJECT
    public:
        explicit NDVServerInterface(QWidget *parent = nullptr){
            // 网络配置
            main_layout->addWidget(new QLabel("IP:", this), 0, 0, 1, 1);
            main_layout->addWidget(IP, 0, 1, 1, 1);
            main_layout->addWidget(new QLabel("Port:", this), 1, 0, 1, 1);
            main_layout->addWidget(Port, 1, 1, 1, 1);
            
        

            Port->setRange(1000, 65535);
            Port->setValue(9008);
            
            main_layout->setColumnStretch(0, 1);
            main_layout->setColumnStretch(1, 2);
            main_layout->setSpacing(5);
            main_layout->setContentsMargins(5, 5, 5, 5);

            this->setLayout(main_layout);
        }

    signals:
        void AddressChanged(const int &port);

    public:
        QGridLayout *main_layout = new QGridLayout(this);
        QLineEdit *IP = new QLineEdit("0.0.0.0", this);
        QSpinBox *Port = new QSpinBox(this);

    };
}
