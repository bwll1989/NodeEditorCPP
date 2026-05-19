#pragma once
//
// Created by Administrator on 2023/12/13.
//

#include <QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include <QGridLayout>
#include <QSpacerItem>
#include <QSpinBox>
#include <QVariantMap>
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"
namespace Nodes
{
    class OscInInterface: public QWidget{
        Q_OBJECT
    public:
        explicit OscInInterface(QWidget *parent = nullptr){
            main_layout = new QGridLayout(this);
            main_layout->setContentsMargins(0, 0, 0, 0);
            main_layout->setSpacing(6);
            portSpinBox->setRange(0,65536);
            portSpinBox->setValue(6000);
            main_layout->addWidget(portLabel,0,0);
            main_layout->addWidget(portSpinBox,0,1);
            main_layout->addWidget(addressLabel,1,0);
            main_layout->addWidget(addressEdit,1,1);
            main_layout->addWidget(valueLabel,2,0);
            main_layout->addWidget(valueEdit,2,1);
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0, 1, 2);
            main_layout->setRowStretch(3, 1);
            main_layout->setColumnStretch(0, 1);
            main_layout->setColumnStretch(1, 2);
            connect(portSpinBox,&IntDragValueWidget::valueChanged,this,&OscInInterface::valueChanged);
            this->setLayout(main_layout);
        }
        signals:
            void portChanged(const int &port);
    public slots:
        void valueChanged() {

            emit portChanged(portSpinBox->value());
        }

    public:
        QGridLayout *main_layout;
        IntDragValueWidget *portSpinBox=new IntDragValueWidget();
        QLineEdit *valueEdit=new QLineEdit();
        QLineEdit *addressEdit=new QLineEdit();
    private:
        QLabel *portLabel=new QLabel("port: ");
        QLabel *valueLabel=new QLabel("value: ");
        QLabel *addressLabel=new QLabel("address: ");


    };
}
