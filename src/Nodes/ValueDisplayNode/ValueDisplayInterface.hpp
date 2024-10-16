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
#include "Common/GUI/qt-value-slider/doubleslider.hpp"
#include "Common/GUI/qt-value-slider/intslider.hpp"
using namespace std;
using namespace ValueSliders;
class ValueDisplayInterface: public QFrame{
public:
    explicit ValueDisplayInterface(QWidget *parent = nullptr){
        boolDisplay->setCheckable(true);
        boolDisplay->setStyleSheet(boolDisplay->isChecked() ? "QPushButton{background-color: #00FF00;}" : "QPushButton{background-color: #FF0000;}");
        main_layout->addWidget(boolDisplay,1);
        main_layout->addWidget(floatDisplay,1);
        main_layout->addWidget(intDisplay,1);
        main_layout->setContentsMargins(4,2,4,4);

        this->setLayout(main_layout);
    }
public:

    QVBoxLayout *main_layout=new QVBoxLayout(this);
    QPushButton *boolDisplay=new QPushButton(" ");
    DoubleSlider *floatDisplay =new DoubleSlider("Double", 0.0, -1, 1, BoundMode::UPPER_LOWER);
    IntSlider *intDisplay = new IntSlider("Int", 0, -1000, 1000, BoundMode::UPPER_LOWER);

};

