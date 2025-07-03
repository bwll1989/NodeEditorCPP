//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include "QWidget"
#include <QPushButton>
#include "QComboBox"
#include "QLayout"
#include "Common/GUI/QPropertyBrowser/QPropertyBrowser.h"
using namespace std;
namespace Nodes
{
    class VST3PluginInterface: public QWidget{
        Q_OBJECT
    public:
        explicit VST3PluginInterface(QWidget *parent = nullptr){
            browser=new QPropertyBrowser(this);
            SelectVST=new QPushButton("Select");
            ShowController=new QPushButton("Controller");
            LoadPreset=new QComboBox();
            //        browser->addFixedProperties(QMetaType::Bool,"Select", false);
            //        connect(browser,&QPropertyBrowser::nodeItemValueChanged,this,&VST3PluginInterface::valueChanged);
            main_layout->addWidget(SelectVST,1);
            main_layout->addWidget(ShowController,1);
            main_layout->addWidget(LoadPreset,1);
            main_layout->addWidget(browser,4);
            ShowController->setEnabled(false);
            //        main_layout->addWidget(frame,4);
            this->setLayout(main_layout);
        }
        Q_SIGNALS:
            // 当 Host 或 Port 发生变化时触发
            void VST3Selected();
    public slots:
        // 处理属性值变化
        void valueChanged(const QString &propertyName, const QVariant &value) {

        if (propertyName == "Select") {
            emit VST3Selected();
        }
    }
    public:
        QVBoxLayout *main_layout=new QVBoxLayout(this);
        QPropertyBrowser *browser;
        QPushButton *SelectVST;
        QPushButton *ShowController;
        QComboBox *LoadPreset;
    };
}