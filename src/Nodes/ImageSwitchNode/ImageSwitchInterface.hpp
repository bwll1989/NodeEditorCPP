//
// Created by Administrator on 2025/07/13.
//
#ifndef IMAGESWITCHINTERFACEFACE_HPP
#define IMAGESWITCHINTERFACEFACE_HPP
#include "QWidget"
#include "QLayout"
#include <QSpinBox>
#include <QVariantMap>
#include <QPushButton>
namespace Nodes
{
class ImageSwitchInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit ImageSwitchInterface(QWidget *parent = nullptr) {
            main_layout=new QGridLayout();

            IndexEdit=new QSpinBox();
            main_layout->addWidget(IndexEdit,0,0,1,1);

            main_layout->setColumnStretch(0,1);
            this->setLayout(main_layout);
            // this->setFixedSize(400,200);

        };

    public:
        QGridLayout *main_layout;
        QSpinBox *IndexEdit;

    };
}
#endif
