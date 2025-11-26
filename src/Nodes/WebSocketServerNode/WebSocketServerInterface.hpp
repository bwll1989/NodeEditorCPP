//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"
#include "QVariant"
using namespace std;
namespace Nodes
{
    class WebSocketServerInterface: public QWidget{
        Q_OBJECT
    public:
        explicit WebSocketServerInterface(QWidget *parent = nullptr){
            format->addItem("HEX");
            format->addItem("UTF-8");
            format->addItem("ASCII");
            messageType->addItem("TEXT");
            messageType->addItem("BINARY");
            main_layout->addWidget(portLabel, 1,0);
            main_layout->addWidget(portSpinBox, 1,1);
            main_layout->addWidget(valueLabel, 2,0);
            main_layout->addWidget(valueEdit, 2,1);
            main_layout->addWidget(messageLabel, 3,0,1,1);
            main_layout->addWidget(messageType, 3,1,1,1);
            main_layout->addWidget(format, 4,0,1,2);
            main_layout->addWidget(sendButton, 5,0,1,2);
            portSpinBox->setRange(0,65536);
            portSpinBox->setValue(2003);
            connect(portSpinBox,&QSpinBox::valueChanged,this,&WebSocketServerInterface::valueChanged);
            this->setLayout(main_layout);
        }
        signals:
            // 当 Host 或 Port 发生变化时触发
            void hostChanged(int port);
    public slots:
        // 处理属性值变化
        void valueChanged() {

            auto port = portSpinBox->value();
            emit hostChanged(port);
        }

    public:

        QGridLayout *main_layout=new QGridLayout(this);
        QSpinBox *portSpinBox=new QSpinBox();
        QLineEdit *valueEdit=new QLineEdit();
        QPushButton *sendButton=new QPushButton("Send");
        QComboBox* format=new QComboBox();
        QComboBox* messageType=new QComboBox();
    private:
        QLabel *portLabel=new QLabel("Port: ");
        QLabel *valueLabel=new QLabel("Value: ");
        QLabel *messageLabel=new QLabel("Message Type    ");

        
    };
}
