//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
class AudioDecoderInterface: public QWidget{
public:
    explicit AudioDecoderInterface(QWidget *parent = nullptr){
//        frame->setLayout(layout);

//        this->setStyleSheet("QFrame{background-color:transparent}");

        namelabel->setAlignment(Qt::AlignCenter);
        main_layout->addWidget(namelabel,0,0,1,3);
        during->setOrientation(Qt::Horizontal);
        main_layout->addWidget(during,2,0,1,3);
        main_layout->addWidget(button,3,0,1,1);
        main_layout->addWidget(button1,3,1,1,1);
        main_layout->addWidget(button2,3,2,1,1);
        main_layout->setContentsMargins(4,2,4,4);
        this->setLayout(main_layout);

    }
public:

    QGridLayout *main_layout=new QGridLayout(this);
    QLabel *namelabel=new QLabel("");
    QSlider *during=new QSlider();
    QPushButton *button=new QPushButton("select");
    QPushButton *button1=new QPushButton("play");
    QPushButton *button2=new QPushButton("stop");

//    QHBoxLayout *sub_layout=new QHBoxLayout;

};

