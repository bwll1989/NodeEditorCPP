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
ExternalControler::ExternalControler(MainWindow *widget, quint16 port, QObject *parent) {
    // 创建 UDP 套接字并绑定到指定端口
    m_udpSocket = new QUdpSocket(this);
    if (!m_udpSocket->bind(QHostAddress::Any, port)) {
        qWarning() << "Failed to bind to port" << port;
        return;
    }

    // 连接 readyRead 信号到处理函数
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &ExternalControler::processPendingDatagrams);
}

void ExternalControler::changeBackgroundColor(const QColor &color)
{
    qDebug()<<"1";
}

void ExternalControler::showWidget()
{
    qDebug()<<"2";

}

void ExternalControler::hideWidget()
{
    qDebug()<<"3";

}

void ExternalControler::processPendingDatagrams()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(datagram.data(), datagram.size());

        // 解析接收到的JSON消息
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(datagram, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Failed to parse JSON:" << parseError.errorString();
            return;
        }

        if (jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            handleJsonMessage(jsonObj);  // 处理解析后的JSON对象
        }
    }

}
void ExternalControler::handleJsonMessage(const QJsonObject &json) {
    if (json.contains("id") && json.contains("port-type") && json.contains("index") &&
        json.contains("value-type") && json.contains("value")) {
        QtNodes::NodeId id=json["id"].toInt();
        QtNodes::PortType PortType;

        if (json["port-type"].toString() == "Input") {
            PortType= PortType::In;
        } else if (json["port-type"].toString() == "Output") {
            PortType= PortType::Out;
        } else {
            PortType= PortType::None;
        }
        int PortIndex=json["index"].toInt(0);

        QString valueType = json["value-type"].toString();
        if (valueType == "bool") {
            // 如果 value-type 是 bool，则解析为布尔值
            bool value = json["value"].toBool();
//            qDebug()<<id<<PortType<<PortIndex<<valueType<<value;

//            m_widget->NodeEditorWidget->model.setPortData(id,PortType,PortIndex,value);
        }
//        else if (valueType == "int") {
//            // 如果 value-type 是 int，则解析为整数
//            int value = json["value"].toInt();
//            // 假设根据 value 的范围来设置颜色（示例用）
//            m_widget->NodeEditorWidget->model.setPortData(id,PortType,PortIndex,value);
//        } else if (valueType == "double") {
//            // 如果 value-type 是 double，则解析为双精度浮点数
//            double value = json["value"].toDouble();
//            // 根据 value 值设置颜色（示例用）
//            m_widget->NodeEditorWidget->model.setPortData(id,PortType,PortIndex,value);
//        } else if (valueType == "string") {
//            // 如果 value-type 是 string，则解析为字符串
//            QString value = json["value"].toString();
//            // 假设根据字符串内容来设置颜色（示例用）
//            m_widget->NodeEditorWidget->model.setPortData(id,PortType,PortIndex,value);
//        } else {
//            qWarning() << "Unknown value-type:" << valueType;
//        }
    } else {
        qWarning() << "Invalid JSON format:" << json;
    }
}