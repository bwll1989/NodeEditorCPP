//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QLineEdit"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class CountInterface: public QWidget{
    public:
        explicit CountInterface(QWidget *parent = nullptr){
            main_layout->setContentsMargins(0,0,0,0);
            this->setLayout(main_layout);
            main_layout->addWidget(countLabel,0,0,1,1);
            main_layout->addWidget(countDisplay,0,1,1,3);
            main_layout->addWidget(ConditionLabel,1,0,1,1);
            main_layout->addWidget(Editor,1,1,1,3);
            main_layout->addWidget(Clear,2,0,1,4);
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0, 1, 2);
            main_layout->setRowStretch(3, 1);
            this->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        }
    private:
        QLabel *countLabel=new QLabel("计数:");
        QLabel *ConditionLabel=new QLabel("条件:");
    public:
        IntDragValueWidget* countDisplay=new IntDragValueWidget(this);
        QGridLayout *main_layout=new QGridLayout(this);
        QLineEdit *Editor=new QLineEdit("$input[\"default\"]");
        QPushButton *Clear=new QPushButton("clear");

    };
}