//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QGridLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"

using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class DelayInterface: public QFrame{
    public:
        explicit DelayInterface(QWidget *parent = nullptr){

            auto label=new QLabel("ms");
            // la->setAlignment(Qt::AlignCenter );
            //        this->setStyleSheet("QFrame{background-color:transparent}");
            main_layout->addWidget(new QLabel("delay: "),0,0);
            main_layout->addWidget(value,0,1);
            main_layout->addWidget(label,0,2);

            main_layout->setContentsMargins(4,2,4,4);
            this->setLayout(main_layout);
        }
    public:
        QGridLayout *main_layout=new QGridLayout(this);
        QLineEdit *value=new QLineEdit(this);
    };
}