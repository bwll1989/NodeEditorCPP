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
    class CutImageInterface: public QFrame{
    public:
        explicit CutImageInterface(QWidget *parent = nullptr){
            main_layout->addWidget(posX,0,0,1,1);
            main_layout->addWidget(pos_x,0,1,1,1);
            main_layout->addWidget(posY,1,0,1,1);
            main_layout->addWidget(pos_y,1,1,1,1);
            main_layout->addWidget(width,2,0,1,1);
            main_layout->addWidget(widthEdit,2,1,1,1);
            main_layout->addWidget(height,3,0,1,1);
            main_layout->addWidget(heightEdit,3,1,1,1);
            this->setLayout(main_layout);
        }

    public:
        QGridLayout *main_layout=new QGridLayout(this);
        QLineEdit *pos_x=new QLineEdit(this);
        QLineEdit *pos_y=new QLineEdit(this);
        QLineEdit *widthEdit=new QLineEdit(this);
        QLineEdit *heightEdit=new QLineEdit(this);

    private:
        QLabel *posX=new QLabel("topleft_X: ");
        QLabel *posY=new QLabel("topleft_Y: ");
        QLabel *width=new QLabel("width: ");
        QLabel *height=new QLabel("height: ");

    };
}