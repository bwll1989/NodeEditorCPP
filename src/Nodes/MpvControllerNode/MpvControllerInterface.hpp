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
// #include "Common/GUI/QPropertyBrowser/QPropertyBrowser.h"
using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class MpvControllerInterface: public QWidget{
        Q_OBJECT
        public:
        explicit MpvControllerInterface(QWidget *parent = nullptr){
            main_layout->addWidget(hostLabel,0,0,1,1);
            main_layout->addWidget(hostEdit,0,1,1,1);
            main_layout->addWidget(Play,1,0,1,2);
            main_layout->addWidget(playlist_prev,2,0,1,1);
            main_layout->addWidget(playlist_next,2,1,1,1);
            main_layout->addWidget(speedSub,3,0,1,1);
            main_layout->addWidget(speedAdd,3,1,1,1);
            main_layout->addWidget(speedReset,4,0,1,2);
            main_layout->addWidget(Fullscreen,5,0,1,2);

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
            QString host = hostEdit->text();
            emit hostChanged(host);
        }
    }
    public:

        QGridLayout *main_layout=new QGridLayout(this);
        QLabel *hostLabel=new QLabel("Host: ");
        QLineEdit *hostEdit=new QLineEdit("127.0.0.1");
        QPushButton *Play=new QPushButton("play");
        QPushButton *playlist_prev=new QPushButton("playlist_prev");
        QPushButton *playlist_next=new QPushButton("playlist_next");
        QPushButton *speedAdd=new QPushButton("speedAdd");
        QPushButton *speedSub=new QPushButton("speedSub");
        QPushButton *speedReset=new QPushButton("speedReset");
        QPushButton *Fullscreen=new QPushButton("fullscreen");


    };
}
