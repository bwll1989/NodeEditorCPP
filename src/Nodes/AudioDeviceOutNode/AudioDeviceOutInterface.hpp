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
        treeWidget->setHeaderLabels(headers);
        namelabel->setAlignment(Qt::AlignCenter);
        device_selector->addItem("NO DEVICES");
        layout->addWidget(namelabel);
        layout->addWidget(device_selector);
        layout->addWidget(treeWidget);
        treeWidget->setColumnCount(2);

//        layout->addWidget(button);
//        button->setFlat(true);
    }
public:

    QLayout *layout=new QVBoxLayout(this);
    QLabel *namelabel=new QLabel("");
    QTreeWidget *treeWidget=new QTreeWidget(this);
    QComboBox *device_selector=new QComboBox();
//    QPushButton *button=new QPushButton("test");

};

