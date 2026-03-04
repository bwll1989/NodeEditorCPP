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
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class CutImageInterface: public QFrame{
    public:
        explicit CutImageInterface(QWidget *parent = nullptr){
            main_layout->addWidget(posX,0,0,1,1);
            main_layout->addWidget(pos_x,0,1,1,1);
            pos_x->setMinimum(0);
            main_layout->addWidget(posY,1,0,1,1);
            main_layout->addWidget(pos_y,1,1,1,1);
            pos_y->setMinimum(0);
            main_layout->addWidget(width,2,0,1,1);
            main_layout->addWidget(widthEdit,2,1,1,1);
            widthEdit->setMinimum(0);
            main_layout->addWidget(height,3,0,1,1);
            main_layout->addWidget(heightEdit,3,1,1,1);
            heightEdit->setMinimum(0);
            this->setLayout(main_layout);
        }

    public:
        QGridLayout *main_layout=new QGridLayout(this);
        IntDragValueWidget *pos_x=new IntDragValueWidget(this);
        IntDragValueWidget *pos_y=new IntDragValueWidget(this);
        IntDragValueWidget *widthEdit=new IntDragValueWidget(this);
        IntDragValueWidget *heightEdit=new IntDragValueWidget(this);

    private:
        QLabel *posX=new QLabel("topleft_X: ");
        QLabel *posY=new QLabel("topleft_Y: ");
        QLabel *width=new QLabel("width: ");
        QLabel *height=new QLabel("height: ");

    };
}