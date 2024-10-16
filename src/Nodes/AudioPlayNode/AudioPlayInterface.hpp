//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
class AudioPlayInterface: public QFrame{
public:
    explicit AudioPlayInterface(QWidget *parent = nullptr){
//        frame->setLayout(layout);

//        this->setStyleSheet("QFrame{background-color:transparent}");
        device_selector->addItem("NO DEVICES");
        namelabel->setAlignment(Qt::AlignCenter);
        main_layout->addWidget(namelabel,0,0,1,4);
        timeLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setAlignment(Qt::AlignCenter);
        main_layout->addWidget(statusLabel,1,0,1,4);
        main_layout->addWidget(timeLabel,2,0,1,4);
        during->setOrientation(Qt::Horizontal);
        main_layout->addWidget(during,3,0,1,4);
        main_layout->addWidget(device_selector,4,0,1,4);
        main_layout->addWidget(button,5,0,1,1);
        main_layout->addWidget(button1,5,1,1,1);
        main_layout->addWidget(button2,5,2,1,1);
        main_layout->addWidget(button3,5,3,1,1);
        main_layout->setContentsMargins(8,8,8,10);
        this->setLayout(main_layout);



    }
public:

    QGridLayout *main_layout=new QGridLayout(this);
    QLabel *namelabel=new QLabel("");
    QLabel *statusLabel=new QLabel("");
    QSlider *during=new QSlider();
    QComboBox *device_selector=new QComboBox();
    QPushButton *button=new QPushButton("select");
    QPushButton *button1=new QPushButton("play");
    QPushButton *button2=new QPushButton("stop");
    QPushButton *button3=new QPushButton("pause");
    QLabel *timeLabel=new QLabel("00:00");
//    QHBoxLayout *sub_layout=new QHBoxLayout;

};

