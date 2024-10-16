//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"

using namespace std;
class TCPServerInterface: public QWidget{
public:
    explicit TCPServerInterface(QWidget *parent = nullptr){

        Port->setValue(12345);
        Port->setRange(2000,65535);

        receiveBox->setStyleSheet("QTextBrowser { background-color: black; color: white; border: 1px solid gray; }"
                                  "QTextBrowser QTextDocument { margin-left: 10px; }");


        main_layout->addWidget(receiveBox,0,0,1,2);
        main_layout->addWidget(Port,1,0,1,2);
        main_layout->addWidget(clientList,2,0,1,1);
        main_layout->addWidget(sendBox,2,1,1,1);
        main_layout->addWidget(send,3,0,1,2);
        main_layout->setColumnStretch(0,1);
        main_layout->setColumnStretch(1,1);
        main_layout->setContentsMargins(4,2,4,4);
        this->setLayout(main_layout);
    }
public:

    QGridLayout *main_layout=new QGridLayout(this);
    QTextBrowser *receiveBox=new QTextBrowser();
    QLineEdit* sendBox=new QLineEdit();
    QSpinBox* Port=new QSpinBox();
    QComboBox* clientList=new QComboBox();
    QPushButton* send=new QPushButton("Send");



};

