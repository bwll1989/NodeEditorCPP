//
// Created by bwll1 on 2024/9/1.
//

#include "OSCReceiver.h"
#include <QJsonObject>
#include <QByteArray>
#include <QHostAddress>
#include "Common/Devices/UdpSocket/UdpSocket.h"
#include "tinyosc.h"
OSCReceiver::OSCReceiver( quint16 port, QObject *parent) {
    // 创建 UDP 套接字并绑定到指定端口

    m_udpSocket = new UdpSocket("127.0.0.1",port);
    // 连接 readyRead 信号到 processPendingDatagrams 函数
    connect(m_udpSocket, &UdpSocket::arrayMsg, this, &OSCReceiver::processPendingDatagrams);
}


OSCReceiver::~OSCReceiver() {
    m_udpSocket->close();
    delete m_udpSocket;
}
void OSCReceiver::processPendingDatagrams(QByteArray datagram) {
        // 使用 tinyosc 解析 OSC 数据
        tosc_message oscMessage;
        if (tosc_parseMessage(&oscMessage, datagram.data(), datagram.size()) == 0) {
            // 获取 OSC 地址
            result=new QVariantMap();
            const char *address = tosc_getAddress(&oscMessage);
            result->insert("address",address);
            // 获取参数格式字符串
            const char *format = tosc_getFormat(&oscMessage);

            // 根据格式解析参数
            for (int i = 0; format[i] != '\0'; i++) {
                const char type = format[i];
                if (type == 'f') {
                    double value = tosc_getNextFloat(&oscMessage);
                    result->insert("type","Float");
                    result->insert("default",static_cast<double>(value));
                } else if (type == 'i') {
                    int32_t value = tosc_getNextInt32(&oscMessage);
                    result->insert("type","Int");
                    result->insert("default",QVariant::fromValue(value));
                } else if (type == 's') {
                    const char *value = tosc_getNextString(&oscMessage);
                    result->insert("type","String");
                    result->insert("default",value);
                }
            }
            emit receiveOSC(*result);

        }

}

void OSCReceiver::setPort(const int &port) {
    qDebug()<<3;
    m_udpSocket->startSocket("0.0.0.0",port);
}