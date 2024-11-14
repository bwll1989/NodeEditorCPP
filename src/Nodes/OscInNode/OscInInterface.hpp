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
class OscInInterface: public QWidget{
    Q_OBJECT
public:
    explicit OscInInterface(QWidget *parent = nullptr){
//        frame->setLayout(layout);
//        this->setStyleSheet("QFrame{background-color:transparent}");


        main_layout=new QVBoxLayout();
        browser=new QPropertyBrowser(this);
        browser->addFixedProperties(QVariant::Int,"Port",6000);
        main_layout->addWidget(browser,0);

        main_layout->setContentsMargins(0,0,0,0);
        connect(browser,&QPropertyBrowser::nodeItemValueChanged,this,&OscInInterface::valueChanged);
        this->setLayout(main_layout);
    }
signals:
    void portChanged(const int &port);
public slots:
    void valueChanged(const QString& propertyName, const QVariant& value) {
        if (propertyName=="Port") {
            emit portChanged(value.toInt());
        }
    }
public:
    QVBoxLayout *main_layout;
    QPropertyBrowser *browser;
    QtVariantProperty *item;


};

