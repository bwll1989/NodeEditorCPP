//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include <QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "../ImageInfoNode/QPropertyBrowser.h"
#include <QSpinBox>
#include <QVariantMap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMetaType>
#include <QWidget>
#include <QComboBox>
namespace Nodes
{
    class OscOutInterface : public QWidget {
        Q_OBJECT
    public:
    explicit OscOutInterface(QWidget *parent = nullptr) : QWidget(parent) {
        // 初始化主布局
        main_layout = new QGridLayout(this);

        // 布局设置
        main_layout->addWidget(hostlabel, 0,0);
        main_layout->addWidget(hostLineEdit, 0,1);
        main_layout->addWidget(portlabel, 1,0);
        main_layout->addWidget(portSpinBox, 1,1);
        main_layout->addWidget(addresslabel, 2,0);
        main_layout->addWidget(addressLineEdit, 2,1);
        main_layout->addWidget(typelabel, 3,0);
        main_layout->addWidget(typeComboBox, 3,1);
        typeComboBox->addItem("Int");
        typeComboBox->addItem("Float");
        typeComboBox->addItem("String");
        main_layout->addWidget(valuelabel, 4,0);
        main_layout->addWidget(valueLineEdit, 4,1);
        main_layout->addWidget(sendButton, 5,0,1,2);
        portSpinBox->setRange(1000,66535);
        portSpinBox->setValue(6001);
        main_layout->setContentsMargins(0, 0, 0, 0);
        // 信号槽连接
        connect(hostLineEdit, &QLineEdit::editingFinished, this, &OscOutInterface::valueChanged);
        connect(portSpinBox, &QSpinBox::editingFinished, this, &OscOutInterface::valueChanged);
        // 应用布局
        setLayout(main_layout);
    }

    signals:
        // 当 Host 或 Port 发生变化时触发
        void hostChanged(QString host, int port);
    public slots:
        // 处理属性值变化
        void valueChanged() {
            QString host = hostLineEdit->text();
            int port = portSpinBox->value();
            emit hostChanged(host, port);
    }
    public:
        QLineEdit *hostLineEdit=new QLineEdit("127.0.0.1");
        QSpinBox *portSpinBox=new QSpinBox();
        QLineEdit *addressLineEdit=new QLineEdit("/test");
        QComboBox *typeComboBox=new QComboBox();
        QLineEdit *valueLineEdit=new QLineEdit();
        QPushButton *sendButton = new QPushButton("Send");
    private:
        QGridLayout *main_layout;     // 主布局
        QLabel *hostlabel=new QLabel("Host: ");                // 标签
        QLabel *portlabel=new QLabel("Port: ");
        QLabel *addresslabel=new QLabel("Address: ");
        QLabel *typelabel=new QLabel("Type: ");
        QLabel *valuelabel=new QLabel("Value: ");

    };
}
