#pragma once
//
// Created by Administrator on 2023/12/13.
//
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"
#include "QCheckBox"
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QVariant>
#include <QQuickWidget>
#include <QtWebEngineQuick/QtWebEngineQuick>
#include <QUrl>
using namespace std;
namespace Nodes
{
    class QMLInterface: public QFrame{
    public:
        // 函数级注释：构造界面，使用 QQuickWidget 加载 qml 的 main.qml，
        // 其中 WebEngineView 渲染 main.html，所有按钮与状态文本均由 QML 控件实现。
        explicit QMLInterface(QWidget *parent = nullptr){
            this->setLayout(main_layout);
            m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
            // m_quickWidget->setSource(QUrl("qrc:qml/content/dynamicview.qml"));
            m_quickWidget->setSource(QUrl("qrc:qml/content/main.qml"));
            main_layout->addWidget(m_quickWidget,0,0,1,1);
        }
    public:
        QGridLayout *main_layout=new QGridLayout(this);
        QQuickWidget *m_quickWidget=new QQuickWidget();
    };
}