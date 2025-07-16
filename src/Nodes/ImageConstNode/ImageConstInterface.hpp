//
// Created by Administrator on 2023/12/13.
//
#ifndef IMAGECONSTINTERFACE_HPP
#define IMAGECONSTINTERFACE_HPP
#include <QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include <QSpinBox>
#include <QVariantMap>

namespace Nodes
{
class ImageConstInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit ImageConstInterface(QWidget *parent = nullptr){
            main_layout=new QGridLayout();
            main_layout->addWidget(display,0,0,1,2);
            main_layout->addWidget(sizeWidthLabel,1,0,1,1);
            main_layout->addWidget(widthEdit,1,1,1,1);
            main_layout->addWidget(sizeHeightLabel,2,0,1,1);
            main_layout->addWidget(heightEdit,2,1,1,1);
            main_layout->addWidget(colorRedLabel,3,0,1,1);
            main_layout->addWidget(colorRedEdit,3,1,1,1);
            main_layout->addWidget(colorGreenLabel,4,0,1,1);
            main_layout->addWidget(colorGreenEdit,4,1,1,1);
            main_layout->addWidget(colorBlueLabel,5,0,1,1);
            main_layout->addWidget(colorBlueEdit,5,1,1,1);
            main_layout->addWidget(colorAlphaLabel,6,0,1,1);
            main_layout->addWidget(colorAlphaEdit,6,1,1,1);
            main_layout->setColumnStretch(0,1);
            main_layout->setColumnStretch(1,1);

            this->setLayout(main_layout);
            this->setFixedSize(200,300);
        }

    public:
        QGridLayout *main_layout;
        QLineEdit *widthEdit=new QLineEdit("100");
        QLineEdit *heightEdit=new QLineEdit("100");
        QLineEdit *colorRedEdit=new QLineEdit("255");
        QLineEdit *colorGreenEdit=new QLineEdit("255");
        QLineEdit *colorBlueEdit=new QLineEdit("255");
        QLineEdit *colorAlphaEdit=new QLineEdit("255");
        QLabel *display=new QLabel();
    private:
        QLabel *sizeWidthLabel=new QLabel("Width: ");
        QLabel *sizeHeightLabel=new QLabel("Height");
        QLabel *colorRedLabel=new QLabel("R: ");
        QLabel *colorGreenLabel=new QLabel("G: ");
        QLabel *colorBlueLabel=new QLabel("B: ");
        QLabel *colorAlphaLabel=new QLabel("A: ");

    };
}
#endif
