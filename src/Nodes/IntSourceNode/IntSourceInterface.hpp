//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "Common/GUI/qt-value-slider/intslider.hpp"
using namespace std;
using namespace ValueSliders;
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

    IntSlider *intDisplay = new IntSlider("", 0, -1000, 1000, BoundMode::UPPER_LOWER);

};

