//
// Created by Administrator on 2023/12/13.
//
#pragma once

#include <QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include <QPushButton>
#include <QVariantMap>
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
namespace Nodes
{
class ImageConstInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit ImageConstInterface(QWidget *parent = nullptr){
            main_layout=new QGridLayout();
            main_layout->addWidget(display,0,0,2,2);
            main_layout->addWidget(sizeWidthLabel,2,0,1,1);
            main_layout->addWidget(widthEdit,2,1,1,1);
            main_layout->addWidget(sizeHeightLabel,3,0,1,1);
            main_layout->addWidget(heightEdit,3,1,1,1);
            main_layout->addWidget(colorEditButton,4,0,1,2);
            main_layout->setColumnStretch(0,1);
            main_layout->setColumnStretch(1,1);
            // 空行
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 5, 0, 1, 2);
            main_layout->setRowStretch(5, 1);

            this->setLayout(main_layout);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }

    public:
        QGridLayout *main_layout;
        IntDragValueWidget *widthEdit=new IntDragValueWidget();
        IntDragValueWidget *heightEdit=new IntDragValueWidget();
        QLabel *display=new QLabel();
        QPushButton *colorEditButton=new QPushButton("Color");
    private:
        QLabel *sizeWidthLabel=new QLabel("Width: ");
        QLabel *sizeHeightLabel=new QLabel("Height");


    };
}
