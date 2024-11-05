//
// Created by Administrator on 2023/12/13.
//

#include <QTreeWidget>
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
class AudioDeviceOutInterface: public QWidget{
public:
    explicit AudioDeviceOutInterface(QWidget *parent = nullptr){
//        frame->setLayout(layout);

        QStringList headers = {"Property", "Value"};

        namelabel->setAlignment(Qt::AlignCenter);
        device_selector->addItem("Default DEVICES");
        layout->addWidget(device_selector,1);
        layout->addWidget(treeWidget,3);


//        layout->addWidget(button);
//        button->setFlat(true);
    }
public:

    QVBoxLayout *layout=new QVBoxLayout(this);
    QLabel *namelabel=new QLabel("");
     QTreeView *treeWidget=new  QTreeView(this);
    QComboBox *device_selector=new QComboBox();
//    QPushButton *button=new QPushButton("test");

};

