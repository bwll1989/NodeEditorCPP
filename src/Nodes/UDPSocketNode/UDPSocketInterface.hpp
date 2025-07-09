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
namespace Nodes
{
    class UDPSocketInterface: public QWidget{
        Q_OBJECT
        public:
        explicit UDPSocketInterface(QWidget *parent = nullptr){
            this->setParent(parent);
            main_layout->addWidget(listeningHostLabel, 0,0);
            main_layout->addWidget(listeningHostEdit, 0,1);
            main_layout->addWidget(listeningPortLabel, 1,0);
            main_layout->addWidget(listeningPortSpinBox, 1,1);
            main_layout->addWidget(targetHostLabel, 2,0);
            main_layout->addWidget(targetHostEdit, 2,1);
            main_layout->addWidget(targetPort, 3,0);
            main_layout->addWidget(targetPortSpinBox, 3,1);
            main_layout->addWidget(valueLabel, 4,0);
            main_layout->addWidget(valueEdit, 4,1);
            main_layout->addWidget(sendButton, 5,0,1,2);
            listeningHostEdit->setText("127.0.0.1");
            targetHostEdit->setText("127.0.0.1");
            targetPortSpinBox->setRange(0,65536);
            targetPortSpinBox->setValue(6011);
            listeningPortSpinBox->setRange(0,65536);
            listeningPortSpinBox->setValue(6000);
            main_layout->setContentsMargins(0, 0, 0, 0);
            connect(listeningPortSpinBox,&QSpinBox::valueChanged,this,&UDPSocketInterface::valueChanged);
            connect(listeningHostEdit,&QLineEdit::textChanged,this,&UDPSocketInterface::valueChanged);
            this->setLayout(main_layout);
        }

        signals:
            // 当 Host 或 Port 发生变化时触发
            void hostChanged(QString host, int port);
    public slots:
        // 处理属性值变化
        void valueChanged() {
            auto host = listeningHostEdit->text();
            auto port = listeningPortSpinBox->value();
            emit hostChanged(host, port);
        }

    public:

        QGridLayout *main_layout=new QGridLayout(this);
        QLineEdit *listeningHostEdit=new QLineEdit();
        QSpinBox *listeningPortSpinBox=new QSpinBox();
        QLineEdit *targetHostEdit=new QLineEdit();
        QSpinBox *targetPortSpinBox=new QSpinBox();
        QLineEdit *valueEdit=new QLineEdit();
        QPushButton *sendButton=new QPushButton("Send");
    private:
        QLabel *listeningHostLabel=new QLabel("listening host: ");
        QLabel *listeningPortLabel=new QLabel("listening port: ");
        QLabel *targetHostLabel=new QLabel("target host: ");
        QLabel *targetPort=new QLabel("target port: ");
        QLabel *valueLabel=new QLabel("value: ");

    };
}