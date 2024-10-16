//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QDoubleSpinBox"
#include "Common/GUI/qt-value-slider/doubleslider.hpp"
using namespace ValueSliders;
using namespace std;
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

    DoubleSlider *floatDisplay = new DoubleSlider("", 0, -1, 1, BoundMode::UPPER_LOWER);
};

