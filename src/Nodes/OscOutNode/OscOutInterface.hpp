//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include <QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include <QGridLayout>
#include <QSpacerItem>
#include <QSpinBox>
#include <QVariantMap>
#include <QPushButton>
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"
#include <QComboBox>
#include <QCoreApplication>
namespace Nodes
{
    class OscOutInterface : public QWidget {
        Q_OBJECT
    public:
    explicit OscOutInterface(QWidget *parent = nullptr) : QWidget(parent) {
        // 初始化主布局
        main_layout = new QGridLayout(this);
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->setSpacing(6);

        // 布局设置
        main_layout->addWidget(hostlabel, 0, 0);
        main_layout->addWidget(hostLineEdit, 0, 1);
        main_layout->addWidget(portlabel, 1, 0);
        main_layout->addWidget(portSpinBox, 1, 1);
        main_layout->addWidget(addresslabel, 2, 0);
        main_layout->addWidget(addressLineEdit, 2, 1);
        main_layout->addWidget(typelabel, 3, 0);
        main_layout->addWidget(typeComboBox, 3, 1);
        typeComboBox->addItem(QCoreApplication::translate("Nodes::OscOutInterface", "整数"));
        typeComboBox->addItem(QCoreApplication::translate("Nodes::OscOutInterface", "浮点"));
        typeComboBox->addItem(QCoreApplication::translate("Nodes::OscOutInterface", "字符串"));
        main_layout->addWidget(valuelabel, 4, 0);
        main_layout->addWidget(valueLineEdit, 4, 1);
        main_layout->addWidget(sendButton, 5, 0, 1, 2);
        main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 6, 0, 1, 2);
        main_layout->setRowStretch(6, 1);
        main_layout->setColumnStretch(0, 1);
        main_layout->setColumnStretch(1, 2);
        portSpinBox->setRange(1000,66535);
        portSpinBox->setValue(8000);
        // 信号槽连接
        connect(hostLineEdit, &QLineEdit::editingFinished, this, &OscOutInterface::valueChanged);
        connect(portSpinBox, &IntDragValueWidget::valueChanged, this, &OscOutInterface::valueChanged);
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
        IntDragValueWidget *portSpinBox=new IntDragValueWidget();
        QLineEdit *addressLineEdit=new QLineEdit("/test");
        QComboBox *typeComboBox=new QComboBox();
        QLineEdit *valueLineEdit=new QLineEdit();
        QPushButton *sendButton = new QPushButton(QCoreApplication::translate("Nodes::OscOutInterface", "发送"));
    private:
        QGridLayout *main_layout;     // 主布局
        QLabel *hostlabel=new QLabel(QCoreApplication::translate("Nodes::OscOutInterface", "主机："));                // 标签
        QLabel *portlabel=new QLabel(QCoreApplication::translate("Nodes::OscOutInterface", "端口："));
        QLabel *addresslabel=new QLabel(QCoreApplication::translate("Nodes::OscOutInterface", "地址："));
        QLabel *typelabel=new QLabel(QCoreApplication::translate("Nodes::OscOutInterface", "类型："));
        QLabel *valuelabel=new QLabel(QCoreApplication::translate("Nodes::OscOutInterface", "值："));

    };
}
