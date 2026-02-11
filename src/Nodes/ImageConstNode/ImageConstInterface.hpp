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
            main_layout->addWidget(colorEditButton,4,0,4,2);
            main_layout->setColumnStretch(0,1);
            main_layout->setColumnStretch(1,1);
            this->setLayout(main_layout);

            /* 函数级注释：移除固定大小，采用最小尺寸 + 可扩展策略 */
            this->setFixedSize(150, 200);
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
