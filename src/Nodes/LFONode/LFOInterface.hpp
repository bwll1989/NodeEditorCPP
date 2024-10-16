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
#include "QCheckBox"
using namespace std;
enum WaveType {
    SineWave,
    SquareWave,
    TriangleWave
};
class LFOInterface: public QFrame{
public:
    explicit LFOInterface(QWidget *parent = nullptr){
        method->addItem("Sine Wave", SineWave);
        method->addItem("Square Wave", SquareWave);
        method->addItem("Triangle Wave", TriangleWave);
        auto la=new QLabel("ms");
        la->setAlignment(Qt::AlignCenter );
//        this->setStyleSheet("QFrame{background-color:transparent}");

        auto label1=new QLabel("波形");
        label1->setAlignment(Qt::AlignHCenter);
        main_layout->addWidget(label1,0,0,1,1);
        main_layout->addWidget(method,0,1,1,2);

        frequency->setValue(5);
        frequency->setRange(0,60);
        auto label2=new QLabel("频率");
        label2->setAlignment(Qt::AlignHCenter);
        main_layout->addWidget(label2,1,0,1,1);
        main_layout->addWidget(frequency,1,1,1,2);

        auto label3=new QLabel("振幅");
        label3->setAlignment(Qt::AlignHCenter);
        amplitude->setValue(10);
        amplitude->setRange(0,10);
        main_layout->addWidget(label3,2,0,1,1);
        main_layout->addWidget(amplitude,2,1,1,2);

        auto label4=new QLabel("相位");
        label4->setAlignment(Qt::AlignHCenter);
        phase->setValue(0);
        phase->setRange(0,1);
        main_layout->addWidget(label4,3,0,1,1);
        main_layout->addWidget(phase,3,1,1,2);

        auto label5=new QLabel("采样频率");
        label5->setAlignment(Qt::AlignHCenter);
        sampleRate->setValue(10);
        sampleRate->setRange(0,100);
        main_layout->addWidget(label5,4,0,1,1);
        main_layout->addWidget(sampleRate,4,1,1,2);

        main_layout->addWidget(loop,5,1,1,2);
        main_layout->setContentsMargins(4,2,4,4);
        this->setLayout(main_layout);
    }
public:

    QGridLayout *main_layout=new QGridLayout(this);
    QComboBox *method=new QComboBox();
    QDoubleSpinBox *frequency=new QDoubleSpinBox();
    QDoubleSpinBox *amplitude=new QDoubleSpinBox();
    QDoubleSpinBox *phase=new QDoubleSpinBox();
    QDoubleSpinBox *sampleRate=new QDoubleSpinBox();

    QCheckBox *loop=new QCheckBox("loop");

};

