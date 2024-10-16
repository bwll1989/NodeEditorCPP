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
class DelayInterface: public QFrame{
public:
    explicit DelayInterface(QWidget *parent = nullptr){

        value->setValue(0);
        value->setRange(0,600000);
        value->setSingleStep(100);
        auto la=new QLabel("ms");
        la->setAlignment(Qt::AlignCenter );
//        this->setStyleSheet("QFrame{background-color:transparent}");

        main_layout->addWidget(value,1);
        main_layout->addWidget(la,1);

        main_layout->setContentsMargins(4,2,4,4);
        this->setLayout(main_layout);
    }
public:

    QHBoxLayout *main_layout=new QHBoxLayout(this);
    QSpinBox *value=new QSpinBox();





};

