//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"

using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class ExtractInterface: public QWidget{
    public:
        explicit ExtractInterface(QWidget *parent = nullptr){
            main_layout->setContentsMargins(0,0,0,0);
            this->setLayout(main_layout);
            main_layout->addWidget(Selector);
        }
    public:
        QVBoxLayout *main_layout=new QVBoxLayout(this);
        QComboBox *Selector=new QComboBox(this);

    };
}