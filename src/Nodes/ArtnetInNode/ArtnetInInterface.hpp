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
class ArtnetInInterface: public QWidget{
    Q_OBJECT
public:
    explicit ArtnetInInterface(QWidget *parent = nullptr){

        browser->addFixedProperties(QMetaType::Int,"Universe",-1);

        main_layout->addWidget(browser,0);
        main_layout->setContentsMargins(0,0,0,0);
        connect(browser,&QPropertyBrowser::nodeItemValueChanged,this,&ArtnetInInterface::valueChanged);

        this->setLayout(main_layout);
    }
signals:
    void UniverseChanged(const int &port);
public slots:
    void valueChanged(const QString& propertyName, const QVariant& value) {
        if (propertyName=="Universe") {
            emit UniverseChanged(value.toInt());
        }
    }
public:
    QVBoxLayout *main_layout=new QVBoxLayout(this);
    QPropertyBrowser *browser=new QPropertyBrowser(this);;



};

