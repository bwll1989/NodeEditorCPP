//
// Created by bwll1 on 2024/9/1.
//

#include "ExternalControler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QHostAddress>
#include <QJsonParseError>
#include "Widget/MainWindow/MainWindow.h"
#include "Common/Devices/OSCReceiver/OSCReceiver.h"
#include "tinyosc.h"
ExternalControler::ExternalControler(MainWindow *widget, quint16 port, QWidget *parent) {
    // 创建 UDP 套接字并绑定到指定端口
   OSC_Receiver=new OSCReceiver(port);
    initUI();
    // 连接 readyRead 信号到 processPendingDatagrams 函数
    connect(OSC_Receiver, &OSCReceiver::receiveOSC, this, &ExternalControler::hasOSC);
}

void ExternalControler::hasOSC(const QVariantMap &data) {
    qDebug() << "Receive Osc Message:"<<data;
    widget->buildPropertiesFromMap(data);
}

void ExternalControler::initUI() {
    layout = new QVBoxLayout(this);
    widget=new QPropertyBrowser();
    this->setLayout(layout);
    layout->addWidget(widget);

}