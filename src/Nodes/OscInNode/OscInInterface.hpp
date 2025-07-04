//
// Created by Administrator on 2023/12/13.
//

#include <QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "Common/GUI/QPropertyBrowser/QPropertyBrowser.h"
#include <QSpinBox>
#include <QVariantMap>
namespace Nodes
{
    class OscInInterface: public QWidget{
        Q_OBJECT
    public:
        explicit OscInInterface(QWidget *parent = nullptr){
            main_layout=new QGridLayout();
            portSpinBox->setRange(0,65536);
            portSpinBox->setValue(6000);
            main_layout->addWidget(portLabel,0,0);
            main_layout->addWidget(portSpinBox,0,1);
            main_layout->addWidget(addressLabel,1,0);
            main_layout->addWidget(addressEdit,1,1);
            main_layout->addWidget(valueLabel,2,0);
            main_layout->addWidget(valueEdit,2,1);
            main_layout->setContentsMargins(0,0,0,0);
            connect(portSpinBox,&QSpinBox::valueChanged,this,&OscInInterface::valueChanged);
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
        QSpinBox *portSpinBox=new QSpinBox();
        QLineEdit *valueEdit=new QLineEdit();
        QLineEdit *addressEdit=new QLineEdit();
    private:
        QLabel *portLabel=new QLabel("port: ");
        QLabel *valueLabel=new QLabel("value: ");
        QLabel *addressLabel=new QLabel("address: ");


    };
}

