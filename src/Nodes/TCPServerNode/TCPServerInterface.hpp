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
#include "QPropertyBrowser/QPropertyBrowser.h"
#include "QVariant"
using namespace std;
namespace Nodes
{
    class TCPServerInterface: public QWidget{
        Q_OBJECT
    public:
        explicit TCPServerInterface(QWidget *parent = nullptr){
            format->addItem("HEX");
            format->addItem("UTF-8");
            format->addItem("ASCII");
            main_layout->addWidget(hostLabel, 0,0);
            main_layout->addWidget(hostLineEdit, 0,1);
            main_layout->addWidget(portLabel, 1,0);
            main_layout->addWidget(portSpinBox, 1,1);
            main_layout->addWidget(valueLabel, 2,0);
            main_layout->addWidget(valueEdit, 2,1);
            main_layout->addWidget(format, 3,0,1,2);
            main_layout->addWidget(sendButton, 4,0,1,2);
            hostLineEdit->setText("0.0.0.0");
            portSpinBox->setRange(0,65536);
            portSpinBox->setValue(2001);
            connect(portSpinBox,&QSpinBox::valueChanged,this,&TCPServerInterface::valueChanged);
            connect(hostLineEdit,&QLineEdit::textChanged,this,&TCPServerInterface::valueChanged);
            this->setLayout(main_layout);
        }
        signals:
            // 当 Host 或 Port 发生变化时触发
            void hostChanged(QString host, int port);
    public slots:
        // 处理属性值变化
        void valueChanged() {
            auto host = hostLineEdit->text();
            auto port = portSpinBox->value();
            emit hostChanged(host, port);
        }

    public:

        QGridLayout *main_layout=new QGridLayout(this);
        QLineEdit *hostLineEdit=new QLineEdit();
        QSpinBox *portSpinBox=new QSpinBox();
        QLineEdit *valueEdit=new QLineEdit();
        QPushButton *sendButton=new QPushButton("Send");
        QComboBox* format=new QComboBox();
    private:
        QLabel *hostLabel=new QLabel("host: ");
        QLabel *portLabel=new QLabel("port: ");
        QLabel *valueLabel=new QLabel("value: ");


        
    };
}
