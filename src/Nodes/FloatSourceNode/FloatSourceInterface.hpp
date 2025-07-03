//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLayout"
#include <QLineEdit>

using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class FloatSourceInterface: public QWidget{
    public:
        explicit FloatSourceInterface(QWidget *parent = nullptr){
            //        frame->setLayout(layout);
            //        this->setStyleSheet("QFrame{background-color:transparent}");
            main_layout->addWidget(floatDisplay,0);
            main_layout->setContentsMargins(0,0,0,0);
            this->setLayout(main_layout);
        }
    public:

        QVBoxLayout *main_layout=new QVBoxLayout(this);

        QLineEdit *floatDisplay = new QLineEdit();
    };
}
