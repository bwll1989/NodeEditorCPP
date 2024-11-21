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
#include "Common/GUI/QPropertyBrowser/QPropertyBrowser.h"
using namespace std;
class UDPSocketInterface: public QWidget{
Q_OBJECT
public:
    explicit UDPSocketInterface(QWidget *parent = nullptr){

//        Port->setValue(12345);
//        Port->setRange(2001,65535);


//        receiveBox->setStyleSheet("QTextBrowser { background-color: black; color: white; border: 1px solid gray; }"
//                                  "QTextBrowser QTextDocument { margin-left: 10px; }");

//        main_layout->addWidget(receiveBox,4);
//        main_layout->addWidget(sendBox,1);
//        main_layout->addWidget(send,1);
        browser=new QPropertyBrowser(this);
        browser->addFixedProperties(QMetaType::Int,"Port",2001);
        browser->addFixedProperties(QMetaType::QString,"Host","0.0.0.0");
        connect(browser,&QPropertyBrowser::nodeItemValueChanged,this,&UDPSocketInterface::valueChanged);
        main_layout->addWidget(browser,4);
        this->setLayout(main_layout);
    }

signals:
    // 当 Host 或 Port 发生变化时触发
    void hostChanged(QString host, int port);
public slots:
    // 处理属性值变化
    void valueChanged(const QString &propertyName, const QVariant &value) {

        if (propertyName == "Port" || propertyName == "Host") {
            // 获取 Host 和 Port 的最新值
            QString host = browser->getProperties("Host").toString();
            int port = browser->getProperties("Port").toInt();
            emit hostChanged(host, port);
        }
    }
public:

    QVBoxLayout *main_layout=new QVBoxLayout(this);
//    QTextBrowser *receiveBox=new QTextBrowser();
//    QLineEdit* sendBox=new QLineEdit();
//    QPushButton* send=new QPushButton("Send");
    QPropertyBrowser *browser;

};

