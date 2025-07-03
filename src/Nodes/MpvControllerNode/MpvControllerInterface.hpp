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
using namespace NodeDataTypes;
namespace Nodes
{
    class MpvControllerInterface: public QWidget{
        Q_OBJECT
        public:
        explicit MpvControllerInterface(QWidget *parent = nullptr){

            //        Port->setValue(12345);
            //        Port->setRange(2001,65535);


            //        receiveBox->setStyleSheet("QTextBrowser { background-color: black; color: white; border: 1px solid gray; }"
            //                                  "QTextBrowser QTextDocument { margin-left: 10px; }");

            //        main_layout->addWidget(receiveBox,4);
            //        main_layout->addWidget(sendBox,1);
            //        main_layout->addWidget(send,1);
            browser=new QPropertyBrowser(this);
            browser->addFixedProperties(QMetaType::QString,"Host","127.0.0.1");
            connect(browser,&QPropertyBrowser::nodeItemValueChanged,this,&MpvControllerInterface::valueChanged);
            main_layout->addWidget(browser,4);
            main_layout->addWidget(Play);
            main_layout->addWidget(Fullscreen);
            this->setLayout(main_layout);
        }

        signals:
            // 当 Host 或 Port 发生变化时触发
            void hostChanged(QString host);
    public slots:
        // 处理属性值变化
        void valueChanged(const QString &propertyName, const QVariant &value) {

        if ( propertyName == "Host") {
            // 获取 Host 和 Port 的最新值
            QString host = browser->getProperties("Host").toString();
            emit hostChanged(host);
        }
    }
    public:

        QVBoxLayout *main_layout=new QVBoxLayout(this);
        QPushButton *Play=new QPushButton("play");
        QPushButton *Fullscreen=new QPushButton("fullscreen");
        QPropertyBrowser *browser;

    };
}
