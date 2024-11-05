//
// Created by Administrator on 2023/12/13.
//

#include <QTreeWidget>
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
class AudioDecoderInterface: public QWidget{
public:
    explicit AudioDecoderInterface(QWidget *parent = nullptr){
//        frame->setLayout(layout);

//        this->setStyleSheet("QFrame{background-color:transparent}");

        main_layout->addWidget(button,1);
        main_layout->addWidget(button1,1);
        main_layout->addWidget(button2,1);
        main_layout->addWidget(treeWidget,1);
        main_layout->setContentsMargins(4,2,4,4);
        this->setLayout(main_layout);

    }
public:

    QVBoxLayout *main_layout=new QVBoxLayout(this);
    QPushButton *button=new QPushButton("select");
    QPushButton *button1=new QPushButton("play");
    QPushButton *button2=new QPushButton("stop");
    QTreeView *treeWidget=new QTreeView(this);
//    QHBoxLayout *sub_layout=new QHBoxLayout;

};

