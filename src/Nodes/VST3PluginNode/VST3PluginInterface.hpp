//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include "QWidget"
#include <QPushButton>
#include "QComboBox"
#include "QLayout"
#include "../ImageInfoNode/QPropertyBrowser.h"
#include <QThread>
using namespace std;
namespace Nodes
{
    class VST3PluginInterface: public QWidget{
        Q_OBJECT
    public:
        explicit VST3PluginInterface(QWidget *parent = nullptr){
            // browser=new QPropertyBrowser(this);
            // SelectVST=new QPushButton("Select");
            ShowController=new QPushButton("Controller");
            //        browser->addFixedProperties(QMetaType::Bool,"Select", false);
            //        connect(browser,&QPropertyBrowser::nodeItemValueChanged,this,&VST3PluginInterface::valueChanged);
            // main_layout->addWidget(SelectVST,0,0,1,1);

            main_layout->addWidget(ShowController,0,0,1,1);
            // main_layout->addWidget(browser,4);
            ShowController->setEnabled(false);
            //        main_layout->addWidget(frame,4);
            this->setLayout(main_layout);

        }
    public:
        QGridLayout *main_layout=new QGridLayout(this);
        // QPropertyBrowser *browser;
        QComboBox *VST3Selector;  //插件选择下拉框
        // QPushButton *SelectVST=new QPushButton("select");
        QPushButton *ShowController;
    };
}