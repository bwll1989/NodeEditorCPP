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
class TCPServerInterface: public QWidget{
    Q_OBJECT
public:
    explicit TCPServerInterface(QWidget *parent = nullptr){

//        Port->setValue(12345);
//        Port->setRange(2000,65535);
//
//        receiveBox->setStyleSheet("QTextBrowser { background-color: black; color: white; border: 1px solid gray; }"
//                                  "QTextBrowser QTextDocument { margin-left: 10px; }");

        browser=new QPropertyBrowser(this);
        browser->addFixedProperties(QMetaType::Int,"Port",2001);
        browser->addFixedProperties(QMetaType::QString,"Host","0.0.0.0");
        main_layout->addWidget(browser);
//        main_layout->addWidget(receiveBox,0,0,1,2);
//        main_layout->addWidget(Port,1,0,1,2);
//        main_layout->addWidget(clientList,2,0,1,1);
//        main_layout->addWidget(sendBox,2,1,1,1);
//        main_layout->addWidget(send,3,0,1,2);
//        main_layout->setColumnStretch(0,1);
//        main_layout->setColumnStretch(1,1);
//        main_layout->setContentsMargins(4,2,4,4);
        connect(browser,&QPropertyBrowser::nodeItemValueChanged,this,&TCPServerInterface::valueChanged);
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

    QGridLayout *main_layout=new QGridLayout(this);
//    QTextBrowser *receiveBox=new QTextBrowser();
//    QLineEdit* sendBox=new QLineEdit();
//    QSpinBox* Port=new QSpinBox();
//    QComboBox* clientList=new QComboBox();
//    QPushButton* send=new QPushButton("Send");
    QPropertyBrowser *browser;



};

