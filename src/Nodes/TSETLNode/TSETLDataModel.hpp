#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "TSETLInterface.hpp"
#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QThread>
#include "Common/Devices/TcpClient/TcpClient.h"
#include "QMutex"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    /**
     * @brief TSETL触发事件收集系统节点数据模型
     * 
     * 实现TSETL协议解析，支持：
     * - 接收TSETL消息帧
     * - 解析消息头和JSON数据
     * - 提取SignalID和相关数据
     * - 以VariableData形式输出解析结果
     * - 每5秒发送心跳包维持连接
     */
    class TSETLDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，初始化TSETL节点
         */
        TSETLDataModel(){
            // 设置端口数量和基本属性
            InPortCount = 2;  // 输入端口：HOST, PORT
            OutPortCount = 3; // 输出端口：SIGNAL_ID, JSON_DATA, CONNECTION
            CaptionVisible = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;
            
            // 初始化数据
            m_inData = std::make_shared<VariableData>();
            m_signalIdData = std::make_shared<VariableData>("");
            m_jsonData = std::make_shared<VariableData>();
            m_connectionStatus = std::make_shared<VariableData>();
            
            // 初始化心跳定时器
            m_heartbeatTimer = new QTimer(this);
            m_heartbeatTimer->setInterval(5000); // 5秒间隔
            connect(m_heartbeatTimer, &QTimer::timeout, this, &TSETLDataModel::sendHeartbeat);
            
            // 注册OSC控制
            NodeDelegateModel::registerOSCControl("/host", widget->hostEdit);
            NodeDelegateModel::registerOSCControl("/port", widget->portSpinBox);
            
            // 连接信号和槽
            connect(this, &TSETLDataModel::connectTCPServer, client, &TcpClient::connectToServer, Qt::QueuedConnection);
            connect(client, &TcpClient::isReady, this, &TSETLDataModel::onConnectionStatusChanged, Qt::QueuedConnection);
            connect(client, &TcpClient::recMsg, this, &TSETLDataModel::recMsg, Qt::QueuedConnection);
            connect(widget->hostEdit, &QLineEdit::editingFinished, this, &TSETLDataModel::hostChange, Qt::QueuedConnection);
            connect(widget->portSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &TSETLDataModel::hostChange, Qt::QueuedConnection);
            connect(this, &TSETLDataModel::stopTCPClient, client, &TcpClient::stopTimer, Qt::QueuedConnection);
            
            // 自动连接
            hostChange();
        }
        
        /**
         * @brief 析构函数，清理资源
         */
        ~TSETLDataModel(){
            // 停止心跳定时器
            if (m_heartbeatTimer) {
                m_heartbeatTimer->stop();
            }
            
            client->disconnectFromServer();
            delete client;
            widget->deleteLater();
        }

    public:
        /**
         * @brief 获取端口标题
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 端口标题
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0:
                    return "HOST";
                case 1:
                    return "PORT";
                default:
                    break;
                }
            case PortType::Out:
                switch (portIndex)
                {
                case 0:
                    return "SIGNAL_ID";     // 信号ID输出
                case 1:
                    return "JSON_DATA";     // JSON数据输出
                case 2:
                    return "CONNECTION";    // 连接状态输出
                default:
                    break;
                }
            default:
                break;
            }
            return "";
        }

        /**
         * @brief 获取端口数据类型
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

       /**
         * @brief 获取输出数据
         * @param portIndex 端口索引
         * @return 输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            switch (portIndex)
            {
            case 0:
                return m_signalIdData;      // SignalID
            case 1:
                return m_jsonData;          // JSON数据
            case 2:
                return m_connectionStatus;  // 连接状态
            default:
                return nullptr;
            }
        }

        /**
         * @brief 设置输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }
            m_inData = std::dynamic_pointer_cast<VariableData>(data);
            switch (portIndex) {
            case 0:
                widget->hostEdit->setText(m_inData->value().toString());
                hostChange();
                break;
            case 1:
                widget->portSpinBox->setValue(m_inData->value().toInt());
                hostChange();
                break;
            default:
                break;
            }
        }

        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
        QWidget *embeddedWidget() override
        {
            return widget;
        }

        /**
         * @brief 保存节点配置
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["Port"] = widget->portSpinBox->value();
            modelJson1["IP"] = widget->hostEdit->text();
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = modelJson1;
            return modelJson;
        }

        /**
         * @brief 加载节点配置
         * @param p JSON对象
         */
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                widget->hostEdit->setText(v["IP"].toString());
                widget->portSpinBox->setValue(v["Port"].toInt());
                hostChange();
            }
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override {
            auto result = ConnectionPolicy::One;
            switch (portType) {
                case PortType::In:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::Out:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::None:
                    break;
            }

            return result;
        }

    public slots:
        /**
         * @brief 接收TCP消息并解析TSETL协议
         * @param msg 接收到的消息数据
         */
        void recMsg(const QVariantMap &msg)
        {
            // 解析TSETL协议
            if (msg.contains("default")) {

                QByteArray rawData = msg["default"].toByteArray();
                parseTSETLMessage(rawData);
            }
            

        }

        /**
         * @brief 连接状态改变处理
         * @param isReady 连接状态
         */
        void onConnectionStatusChanged(const bool &isReady)
        {
            m_connectionStatus = std::make_shared<VariableData>(isReady);
            widget->updateConnectionStatus(isReady);
            
            // 根据连接状态控制心跳定时器
            if (isReady) {
                m_heartbeatTimer->start();
                qDebug() << "TSETL服务连接成功";
            } else {
                m_heartbeatTimer->stop();
            }
            
            Q_EMIT dataUpdated(2);
        }

    public slots:
        /**
         * @brief 主机或端口改变时重新连接
         */
        void hostChange()
        {
            emit connectTCPServer(widget->hostEdit->text(), widget->portSpinBox->value());
        }

    signals:
        /**
         * @brief 停止TCP客户端信号
         */
        void stopTCPClient();
        
        /**
         * @brief 连接TCP服务器信号
         * @param host 主机地址
         * @param port 端口号
         */
        void connectTCPServer(const QString &host, int port);

    private:
        /**
         * @brief 解析TSETL消息协议
         * @param data 原始消息数据
         */
        void parseTSETLMessage(const QByteArray &data)
        {

            if (data.size() < 4) {
                qDebug() << "无效数据,TSETL消息长度不足";
                return;
            }
            
            try {
                // 解析消息头
                QDataStream stream(data);
                stream.setByteOrder(QDataStream::LittleEndian);
                
                // 读取包头 (4字节，十六进制 0xFBFBFBFB)
                quint32 header;
                stream >> header;
                
                if (header != 0xFBFBFBFB) {
                    qDebug() << "无效数据,TSETL消息头不正确:" ;
                    return;
                }
                
                // 读取数据段长度 (2字节)
                quint16 dataLength;
                stream >> dataLength;
                
                if (data.size() < 6 + dataLength + 2) {
                    qDebug() << "无效数据,TSETL消息数据长度不匹配";
                    return;
                }
                
                // 读取JSON数据
                QByteArray jsonData = data.mid(6, dataLength);
                // 读取CRC16校验 (2字节)
                quint16 crc16;
                stream.device()->seek(6 + dataLength);
                stream >> crc16;
                quint16 calculatedCrc = calculateModbusCRC16(jsonData);
                if (!validateCRC16(jsonData, crc16)) {
                    qDebug() << "JSON数据长度:" << jsonData.size();
                    qDebug() << "JSON数据(十六进制):" << jsonData.toHex(' ');
                    qDebug() << "接收到的CRC16:" << QString::number(crc16, 16).toUpper();
                    qDebug() << "期望CRC16:" << QString::number(crc16, 16).toUpper();
                    qDebug() << "实际CRC16:" << QString::number(calculatedCrc, 16).toUpper();
                    // 可以选择继续处理或返回
                    return; // 如果要严格校验，可以取消注释这行
                } 
                // 解析JSON数据
                parseJSONData(jsonData);
                
            } catch (const std::exception &e) {
                qDebug() << "TSETL消息解析异常:" << e.what();
            }
        }
        
        /**
         * @brief 解析JSON数据部分 - 处理SignalID和HeartBeat消息
         * @param jsonData JSON数据字节数组
         */
        void parseJSONData(const QByteArray &jsonData)
        {
            // 清理JSON数据，移除末尾的空字符和其他无效字符
            QByteArray cleanedJsonData = jsonData;
            
            // 移除末尾的空字符（\0）和其他控制字符
            while (!cleanedJsonData.isEmpty() && 
                   (cleanedJsonData.at(cleanedJsonData.size() - 1) == '\0' ||
                    cleanedJsonData.at(cleanedJsonData.size() - 1) < 32)) {
                cleanedJsonData.chop(1);
            }
            
            // // 打印收到的原始JSON数据用于调试
            // qDebug() << "=== 收到的JSON数据 ===";
            // qDebug() << "原始字节数组长度:" << jsonData.size();
            // qDebug() << "清理后字节数组长度:" << cleanedJsonData.size();
            // qDebug() << "原始字节数组(十六进制):" << jsonData.toHex(' ');
            // qDebug() << "清理后字节数组(十六进制):" << cleanedJsonData.toHex(' ');
            // qDebug() << "清理后JSON字符串:" << QString::fromUtf8(cleanedJsonData);
            // qDebug() << "========================";
            
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(cleanedJsonData, &error);
            
            if (error.error != QJsonParseError::NoError) {
                qDebug() << "JSON解析错误:" << error.errorString();
                qDebug() << "错误位置:" << error.offset;
                qDebug() << "清理后JSON数据内容:" << QString::fromUtf8(cleanedJsonData);
                qDebug() << "清理后JSON数据十六进制:" << cleanedJsonData.toHex(' ');
                return;
            }
            
            if (!doc.isObject()) {
                qDebug() << "JSON不是对象格式";
                return;
            }
            
            QJsonObject jsonObj = doc.object();
            
            // 提取MsgID
            QString msgId = jsonObj["MsgID"].toString();
            // qDebug() << "收到消息类型:" << msgId;
            
            // 处理不同类型的消息
            if (msgId == "SignalID") {
                // 处理SignalID消息
                // qDebug() << "=== 处理SignalID消息 ===";
                
                // 解析Datas部分
                QJsonObject datasObj = jsonObj["Datas"].toObject();
                
                // 提取SignalID
                QString signalId = datasObj["SignalID"].toString();
                QString dateTime = datasObj["DateTime"].toString();
                
                // qDebug() << "提取到的SignalID:" << signalId;
                // qDebug() << "时间戳:" << dateTime;
                
                // 设置SignalID输出
                m_signalIdData = std::make_shared<VariableData>(signalId);
                
                // 设置JSON数据输出（完整的JSON对象）
                QVariantMap jsonVariantMap = jsonObj.toVariantMap();
                m_jsonData = std::make_shared<VariableData>(jsonVariantMap);
                
                // 更新界面显示
                widget->updateLastSignal(signalId, dateTime);
                // 只发出相关端口的数据更新信号
                Q_EMIT dataUpdated(0);  // SIGNAL_ID
                Q_EMIT dataUpdated(1);  // JSON_DATA
                Q_EMIT dataUpdated(2);  // CONNECTION
                // qDebug() << "=== SignalID消息处理完成 ===";
            }
            else if (msgId == "HeartBeat" || msgId == "HeartBeatReply") {
                // 处理心跳包消息（包括心跳包和心跳包回复）
                // qDebug() << "=== 收到心跳包消息 ===";
                QString fromObject = jsonObj["FromObject"].toString();
                QString toObject = jsonObj["ToObject"].toString();
                // qDebug() << "消息类型:" << msgId;
                // qDebug() << "来源:" << fromObject << " 目标:" << toObject;
                
                // 如果是服务器回应的心跳包
                if (msgId == "HeartBeatReply" && fromObject == "TSETL" && toObject == "QSC") {
                    // qDebug() << "收到服务器心跳包回复 - 连接正常";
                } else if (msgId == "HeartBeat" && fromObject == "QSC" && toObject == "TSETL") {
                    // qDebug() << "收到自己发送的心跳包回显";
                }
                // qDebug() << "=== 心跳包消息处理完成 ===";
            }
            else {
                qDebug() << "忽略未知消息类型 - MsgID:" << msgId;
            }
        }
        
        /**
         * @brief 验证CRC16校验码
         * @param jsonData JSON数据（用于计算CRC16）
         * @param expectedCrc 期望的CRC值
         * @return 校验是否通过
         */
        bool validateCRC16(const QByteArray &jsonData, quint16 expectedCrc)
        {
            quint16 calculatedCrc = calculateModbusCRC16(jsonData);
            // qDebug() << "CRC16校验详情:";
            // qDebug() << "  数据长度:" << jsonData.size() << "字节";
            // qDebug() << "  期望CRC16: 0x" << QString::number(expectedCrc, 16).toUpper();
            // qDebug() << "  计算CRC16: 0x" << QString::number(calculatedCrc, 16).toUpper();
            // qDebug() << "  校验结果:" << (calculatedCrc == expectedCrc ? "通过" : "失败");
            
            return calculatedCrc == expectedCrc;
        }

        /**
         * @brief 计算Modbus CRC16校验码
         * @param data 数据
         * @return CRC16校验码
         */
        quint16 calculateModbusCRC16(const QByteArray &data)
        {
            static const unsigned char auchCRCHi[] = { 
                0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
                0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
                0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
                0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
                0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
                0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
                0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
                0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
                0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
                0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
                0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
                0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
                0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
                0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
                0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
                0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
                0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
                0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
                0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
                0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
                0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
                0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
                0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
                0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
                0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
                0x80, 0x41, 0x00, 0xC1, 0x81, 0x40 
            }; 
            
            static const unsigned char auchCRCLo[] = { 
                0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 
                0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 
                0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 
                0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 
                0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 
                0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 
                0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 
                0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 
                0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 
                0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 
                0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 
                0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 
                0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 
                0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 
                0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 
                0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 
                0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 
                0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5, 
                0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 
                0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 
                0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 
                0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 
                0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 
                0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C, 
                0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 
                0x43, 0x83, 0x41, 0x81, 0x80, 0x40 
            }; 
            
            unsigned char ucCRCHi = 0xFF; 
            unsigned char ucCRCLo = 0xFF; 
            int iIndex = 0x0000; 
            
            for (int i = 0; i < data.size(); ++i) {
                iIndex = (int)(ucCRCLo ^ (unsigned char)data[i]); 
                ucCRCLo = (unsigned char)(ucCRCHi ^ auchCRCHi[iIndex]); 
                ucCRCHi = auchCRCLo[iIndex]; 
            }
            
            return (ucCRCHi << 8 | ucCRCLo); 
        }
    private:
        /**
         * @brief 生成心跳包JSON数据
         * @return JSON字符串
         */
        QString generateHeartbeatJson()
        {
            QJsonObject data;
            QJsonObject datas;
            
            // 设置当前时间
            QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            datas["DateTime"] = currentTime;
            
            // 设置消息内容
            data["datas"] = datas;
            data["MsgID"] = "HeartBeat";
            data["FromObject"] = "QSC";
            data["ToObject"] = "TSETL";
            
            // 转换为JSON字符串
            QJsonDocument doc(data);
            return doc.toJson(QJsonDocument::Compact);
        }
        
        /**
         * @brief 创建TSETL协议数据包
         * @param jsonData JSON数据
         * @return 完整的TSETL数据包
         */
        QByteArray createTSETLPacket(const QByteArray &jsonData)
        {
            QByteArray packet;
            QDataStream stream(&packet, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);
            
            // 1. 写入包头 (4字节，0xFBFBFBFB)
            quint32 header = 0xFBFBFBFB;
            stream << header;
            
            // 2. 写入数据长度 (2字节)
            quint16 dataLength = static_cast<quint16>(jsonData.size());
            stream << dataLength;
            
            // 3. 写入JSON数据 - 使用stream写入而不是append
            stream.writeRawData(jsonData.data(), jsonData.size());
            
            // 4. 计算并写入CRC16校验码 (2字节)
            quint16 crc16 = calculateModbusCRC16(jsonData);
            stream << crc16;
            
            return packet;
        }

        /**
         * @brief 发送心跳包
         */
        void sendHeartbeat()
        {
            // 生成心跳包JSON数据
            QString heartbeatJson = generateHeartbeatJson();
            
            // 封装为TSETL协议数据包
            QByteArray tsetlPacket = createTSETLPacket(heartbeatJson.toUtf8());
            
            // 通过TCP客户端发送（使用HEX格式）
            QString hexString = tsetlPacket.toHex().toUpper();
            client->sendMessage(hexString, 0); // 0表示HEX格式
            
            // 添加详细的调试信息
            // qDebug() << "=== 心跳包发送详情 ===";
            // qDebug() << "JSON数据:" << heartbeatJson;
            // qDebug() << "JSON数据长度:" << heartbeatJson.toUtf8().size() << "字节";
            // qDebug() << "完整数据包长度:" << tsetlPacket.size() << "字节";
            // qDebug() << "数据包结构:";
            // qDebug() << "  包头(4字节):" << tsetlPacket.mid(0, 4).toHex().toUpper();
            // qDebug() << "  长度(2字节):" << tsetlPacket.mid(4, 2).toHex().toUpper();
            // qDebug() << "  JSON数据(" << heartbeatJson.toUtf8().size() << "字节):" << tsetlPacket.mid(6, heartbeatJson.toUtf8().size()).toHex().toUpper();
            // qDebug() << "  CRC16(2字节):" << tsetlPacket.mid(6 + heartbeatJson.toUtf8().size(), 2).toHex().toUpper();
            // qDebug() << "完整HEX数据包:" << hexString;
            // qDebug() << "===================";
        }

    private:
        TSETLInterface *widget = new TSETLInterface();
        TcpClient *client = new TcpClient();
        QTimer *m_heartbeatTimer;  // 心跳定时器

        // 数据成员 - 只保留需要的3个输出
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_signalIdData;      // SignalID输出
        std::shared_ptr<VariableData> m_jsonData;          // JSON数据输出
        std::shared_ptr<VariableData> m_connectionStatus;  // 连接状态输出
    };

}