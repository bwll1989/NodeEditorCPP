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
    class TCPClientInterface: public QWidget{
    public:
        explicit TCPClientInterface(QWidget *parent = nullptr){

            portSpinBox->setRange(0,65535);
            portSpinBox->setValue(2001);

            main_layout->addWidget(hostLabel,0,0,1,1);
            main_layout->addWidget(hostEdit,0,1,1,1);
            main_layout->addWidget(portLabel,1,0,1,1);
            main_layout->addWidget(portSpinBox,1,1,1,1);
            main_layout->addWidget(valueLabel,2,0,1,1);
            main_layout->addWidget(valueEdit,2,1,1,1);
            main_layout->addWidget(format,3,0,1,2);
            main_layout->addWidget(statusButton,4,0,1,2);
            statusButton->setFlat(true);
            statusButton->setCheckable(true);
            statusButton->setEnabled(false);
            statusButton->setStyleSheet("color: red; font-weight: bold;");
            main_layout->addWidget(send,5,0,1,2);
            format->addItem("HEX");
            format->addItem("UTF-8");
            format->addItem("ASCII");
            this->setLayout(main_layout);
        }
    public:
        QGridLayout *main_layout=new QGridLayout(this);
        QLineEdit* valueEdit=new QLineEdit();
        QSpinBox* portSpinBox=new QSpinBox();
        QLineEdit* hostEdit=new QLineEdit("127.0.0.1");
        QPushButton* send=new QPushButton("Send");
        QComboBox* format=new QComboBox();
        QPushButton* statusButton=new QPushButton("Disconnect");
    private:
        QLabel *hostLabel=new QLabel("Host  ");
        QLabel *portLabel=new QLabel("Port  ");
        QLabel *valueLabel=new QLabel("Value    ");

    };
}
