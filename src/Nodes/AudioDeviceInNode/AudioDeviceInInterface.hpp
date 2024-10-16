//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
class AudioDeviceInInterface: public QFrame{
public:
    explicit AudioDeviceInInterface(QWidget *parent = nullptr){
//        frame->setLayout(layout);

        this->setStyleSheet("QFrame{background-color:transparent}");
        namelabel->setAlignment(Qt::AlignCenter);
        device_selector->addItem("NO DEVICES");
        layout->addWidget(namelabel);
        layout->addWidget(device_selector);
        layout->addWidget(button);
        button->setFlat(true);
    }
public:

    QLayout *layout=new QVBoxLayout(this);
    QLabel *namelabel=new QLabel("");
    QComboBox *device_selector=new QComboBox();
    QPushButton *button=new QPushButton("");

};

