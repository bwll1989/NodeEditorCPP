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

namespace Nodes
{
    class WebSocketClientInterface: public QWidget{
    public:
        explicit WebSocketClientInterface(QWidget *parent = nullptr){
            main_layout->addWidget(hostLabel,0,0,1,1);
            main_layout->addWidget(hostUrlEdit,0,1,1,1);
            main_layout->addWidget(valueLabel,1,0,1,1);
            main_layout->addWidget(valueEdit,1,1,1,1);
            main_layout->addWidget(messageLabel,2,0,1,1);
            main_layout->addWidget(messageType,2,1,1,1);
            main_layout->addWidget(format,3,0,1,2);
            main_layout->addWidget(statusButton,4,0,1,2);
            statusButton->setFlat(true);
            statusButton->setCheckable(true);
            statusButton->setEnabled(false);
            main_layout->addWidget(send,5,0,1,2);
            send->setEnabled(false);
            format->addItem("HEX");
            format->addItem("UTF-8");
            format->addItem("ASCII");
            messageType->addItem("TEXT");
            messageType->addItem("BINARY");
            this->setLayout(main_layout);
        }
    public:

        QGridLayout *main_layout=new QGridLayout(this);
        QLineEdit* valueEdit=new QLineEdit();
        QLineEdit* hostUrlEdit=new QLineEdit("ws://127.0.0.1:2003");
        QPushButton* send=new QPushButton("Send");
        QComboBox* format=new QComboBox();
        QComboBox* messageType=new QComboBox();
        QPushButton* statusButton=new QPushButton("Disconnected");
    private:
        QLabel *hostLabel=new QLabel("URL  ");
        QLabel *valueLabel=new QLabel("Value    ");
        QLabel *messageLabel=new QLabel("Message Type    ");

    };
}
