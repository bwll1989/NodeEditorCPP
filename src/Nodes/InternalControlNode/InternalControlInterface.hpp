//
// Created by Administrator on 2025/07/13.
//
#ifndef INTERNALCONTROLINTERFACE_HPP
#define INTERNALCONTROLINTERFACE_HPP
#include "QWidget"
#include "QLayout"
#include <QSpinBox>
#include <QVariantMap>
#include <QPushButton>
#include "OscListWidget/OSCMessageListWidget.hpp"
#include <QEvent>
#include <QKeyEvent>
#include <QContextMenuEvent>
namespace Nodes
{
class InternalControlInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit InternalControlInterface(bool OnlyInternal,QWidget *parent = nullptr) {
            main_layout=new QGridLayout();
            m_listWidget=new OSCMessageListWidget(OnlyInternal,this);
            main_layout->addWidget(m_listWidget,0,0,1,1);
            // 添加右键菜单支持
            main_layout->addWidget(testButton,1,0,1,1);
            this->setLayout(main_layout);
            this->setFixedSize(400,200);

        };

    public:
        QGridLayout *main_layout;
        OSCMessageListWidget* m_listWidget;
        QPushButton *testButton=new QPushButton("Trigger");

    // 添加事件过滤器声明



    };
}
#endif
