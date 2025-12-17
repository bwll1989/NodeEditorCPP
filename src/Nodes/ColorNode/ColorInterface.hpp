//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include <QLineEdit>
#include <QComboBox>
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
            // main_layout->addWidget(colorFormatComboBox,1,0,1,2);
            main_layout->addWidget(colorEditButton,2,0,1,2);
            // main_layout->setColumnStretch(0,1);
            // main_layout->setColumnStretch(1,1);
            // main_layout->setColumnStretch(2,1);
            main_layout->setSpacing(0);
            main_layout->setContentsMargins(0,0,0,0);

            // colorFormatComboBox->addItems(colorFormatComboBoxItems);
            this->setLayout(main_layout);
            /* 函数级注释：移除固定大小，采用最小尺寸 + 可扩展策略 */
            this->setFixedSize(80, 120);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }

    public:
        QGridLayout *main_layout;
        QLabel *display=new QLabel();
        QPushButton *colorEditButton=new QPushButton("Edit Color");
        // QComboBox *colorFormatComboBox=new QComboBox();
        // QStringList colorFormatComboBoxItems={"float","uint8"};


    };
}

