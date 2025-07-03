//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include <QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "Common/GUI/QPropertyBrowser/QPropertyBrowser.h"
#include <QSpinBox>
#include <QVariantMap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMetaType>
#include <QWidget>

namespace Nodes
{
    class OscOutInterface : public QWidget {
        Q_OBJECT
        public:
        explicit OscOutInterface(QWidget *parent = nullptr) : QWidget(parent) {
            // 初始化主布局
            main_layout = new QVBoxLayout(this);
            browser = new QPropertyBrowser(this);

            // 添加固定属性
            browser->addFixedProperties(QMetaType::Int, "Port", 6001);
            browser->addFixedProperties(QMetaType::QString, "Host", "127.0.0.1");
            // 布局设置
            main_layout->addWidget(browser, 0);
            main_layout->setContentsMargins(0, 0, 0, 0);
            // 信号槽连接
            connect(browser, &QPropertyBrowser::nodeItemValueChanged, this, &OscOutInterface::valueChanged);

            // 应用布局
            setLayout(main_layout);
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
        QPropertyBrowser *browser;    // 属性浏览器
    private:
        QVBoxLayout *main_layout;     // 主布局


    };
}
