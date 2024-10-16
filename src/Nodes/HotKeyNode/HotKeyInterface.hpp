//
// Created by Administrator on 2023/12/13.
//

#include <QCheckBox>
#include <QLCDNumber>
#include <QKeySequenceEdit>
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"
#include "QHotkey"
#include "HotKeyItem.hpp"
#include <QtCore/QObject>
class HotKeyInterface: public QWidget{
public:
    explicit HotKeyInterface(QWidget *parent = nullptr){

//        this->setStyleSheet("QFrame{background-color:transparent}");
        main_layout->addWidget(item_1);
        main_layout->addWidget(item_2);
        main_layout->addWidget(item_3);
        main_layout->addWidget(item_4);
        main_layout->addWidget(item_5);
        main_layout->setContentsMargins(4,2,4,4);
        this->setLayout(main_layout);
    }
public:
    QVBoxLayout *main_layout=new QVBoxLayout(this);
    HotKeyItem *item_1=new HotKeyItem(this);
    HotKeyItem *item_2=new HotKeyItem(this);
    HotKeyItem *item_3=new HotKeyItem(this);
    HotKeyItem *item_4=new HotKeyItem(this);
    HotKeyItem *item_5=new HotKeyItem(this);
};

