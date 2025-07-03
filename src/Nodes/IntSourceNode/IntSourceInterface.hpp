//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include <QLineEdit>
using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class IntSourceInterface: public QWidget{
    public:
        explicit IntSourceInterface(QWidget *parent = nullptr){
            //        frame->setLayout(layout);
            //        this->setStyleSheet("QFrame{background-color:transparent}");

            main_layout->addWidget(intDisplay,0);
            main_layout->setContentsMargins(0,0,0,0);
            this->setLayout(main_layout);
        }
    public:

        QVBoxLayout *main_layout=new QVBoxLayout(this);

        QLineEdit *intDisplay = new QLineEdit();

    };
}