//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QLineEdit"

using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class ConditionInterface: public QWidget{
    public:
        explicit ConditionInterface(QWidget *parent = nullptr){
            main_layout->setContentsMargins(0,0,0,0);
            this->setLayout(main_layout);
            main_layout->addWidget(Editor);

        }
    public:
        QVBoxLayout *main_layout=new QVBoxLayout(this);
        QLineEdit *Editor=new QLineEdit("$input[\"default\"]");


    };
}