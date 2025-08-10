#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QtCore/qglobal.h>
#include "ArtnetTransmitter.h"
#include "ArtnetFrame.h"
#include "ArtnetOutInterface.hpp"
#include <QVariantMap>
#include "QThread"
#include <QTimer>
#include <QDateTime>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

/**
 * ArtnetOutDataModel - Artnet数据发送节点模型（简化版）
 * 接收ArtnetUniverseNode的输出数据并自动发送到Art-Net网络
 */
class ArtnetOutDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * 构造函数 - 初始化Artnet发送节点
     */
    ArtnetOutDataModel()
    {
        InPortCount = 4;  // 支持4个Universe输入端口
        OutPortCount = 1; // 发送状态输出
        CaptionVisible = true;
        Caption = "Artnet Out";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable=true;
        m_outData = std::make_shared<VariableData>();
        
        // 获取ArtnetTransmitter单例
        m_artnetTransmitter = ArtnetTransmitter::getInstance();
        
        // 连接界面信号（只保留目标主机变化）
        connect(widget, &ArtnetOutInterface::TargetHostChanged, this, &ArtnetOutDataModel::onTargetHostChanged);
        
        // 连接ArtnetTransmitter信号
        connect(m_artnetTransmitter, &ArtnetTransmitter::frameSent, this, &ArtnetOutDataModel::onFrameSent);
        connect(m_artnetTransmitter, &ArtnetTransmitter::frameSendFailed, this, &ArtnetOutDataModel::onFrameSendFailed);
        
        // 设置默认值
        m_targetHost = "192.168.1.255";  // 默认广播地址
        
        widget->setTargetHost(m_targetHost);
        
        // 初始化Universe数据缓存
        m_universeCache.clear();
    }

    /**
     * 析构函数 - 清理资源
     */
    ~ArtnetOutDataModel() override {
        delete widget;
    }

    /**
     * 获取端口标题
     * @param portType 端口类型（输入/输出）
     * @param portIndex 端口索引
     * @return 端口标题字符串
     */
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        switch(portType)
        {
        case PortType::In:
            return QString("UNIVERSE DATA");
        case PortType::Out:
            switch(portIndex)
            {
            case 0:
                return "STATUS";  // 发送状态输出
            default:
                return "";
            }
        default:
            return "";
        }
    }

    /**
     * 获取端口数据类型
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
     * 获取输出数据
     * @param port 端口索引
     * @return 输出数据
     */
    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return m_outData;
    }

    /**
     * 设置输入数据 - 接收ArtnetUniverseNode的输出并自动发送
     * @param data 输入数据（来自ArtnetUniverseNode）
     * @param portIndex 端口索引
     */
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (data == nullptr) {
            // 如果数据为空，移除对应的Universe缓存
            m_universeCache.remove(portIndex);
            return;
        }
        
        auto variableData = std::dynamic_pointer_cast<VariableData>(data);
        if (!variableData) {
            return;
        }
        
        // 获取ArtnetUniverseNode的输出数据
        QVariant defaultValue = variableData->value("default");
        if (!defaultValue.isValid()) {
            return;
        }
        
        QVariantMap artnetPacket = defaultValue.toMap();
        if (artnetPacket.isEmpty()) {
            return;
        }
        
        // 验证是否为有效的Art-Net数据包
        if (artnetPacket["protocol"].toString() != "Art-Net") {
            return;
        }
        
        // 缓存Universe数据
        m_universeCache[portIndex] = artnetPacket;
        
        // 自动发送该Universe的数据
        sendUniverseData(artnetPacket);
    }

    /**
     * 保存节点配置
     * @return JSON配置对象
     */
    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["TargetHost"] = m_targetHost;
        
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = modelJson1;
        return modelJson;
    }

    /**
     * 加载节点配置
     * @param p JSON配置对象
     */
    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            QJsonObject obj = v.toObject();
            m_targetHost = obj["TargetHost"].toString("192.168.1.255");
            
            widget->setTargetHost(m_targetHost);
        }
    }
    
    /**
     * 获取嵌入式控件
     * @return 控件指针
     */
    QWidget *embeddedWidget() override {
        return widget;
    }
    
private slots:
    /**
     * 目标主机变化处理
     * @param host 新的目标主机地址
     */
    void onTargetHostChanged(const QString &host) {
        m_targetHost = host;
    }
    
    /**
     * 数据帧发送成功处理
     * @param frame 发送成功的数据帧
     * @param bytesWritten 发送的字节数
     */
    void onFrameSent(const ArtnetFrame &frame, qint64 bytesWritten) {
        QVariantMap statusData;
        statusData["status"] = "success";
        statusData["universe"] = frame.universe;
        statusData["host"] = frame.host;
        statusData["bytesWritten"] = bytesWritten;
        statusData["timestamp"] = QDateTime::currentDateTime().toString();
        
        m_outData = std::make_shared<VariableData>(statusData);
        Q_EMIT dataUpdated(0);
        
        widget->setStatus(QString("Universe %1 发送成功").arg(frame.universe), true);
    }
    
    /**
     * 数据帧发送失败处理
     * @param frame 发送失败的数据帧
     * @param error 错误信息
     */
    void onFrameSendFailed(const ArtnetFrame &frame, const QString &error) {
        QVariantMap statusData;
        statusData["status"] = "failed";
        statusData["error"] = error;
        statusData["universe"] = frame.universe;
        statusData["host"] = frame.host;
        statusData["timestamp"] = QDateTime::currentDateTime().toString();
        
        m_outData = std::make_shared<VariableData>(statusData);
        Q_EMIT dataUpdated(0);
        
        widget->setStatus(QString("Universe %1 发送失败: %2").arg(frame.universe).arg(error), false);
    }
    
private:
    /**
     * 发送指定的Universe数据
     * @param artnetPacket ArtnetUniverseNode输出的数据包
     */
    void sendUniverseData(const QVariantMap &artnetPacket) {
        if (!m_artnetTransmitter) {
            widget->setStatus("ArtnetTransmitter未初始化", false);
            return;
        }
        
        // 创建ArtnetFrame
        ArtnetFrame frame;
        frame.host = m_targetHost;
        frame.port = 6454;  // 标准Art-Net端口
        frame.sequence = 0;
        frame.timestamp = QDateTime::currentMSecsSinceEpoch();
        
        // 从ArtnetUniverseNode的数据包中提取Universe信息
        frame.universe = static_cast<quint16>(artnetPacket["fullUniverse"].toInt());
        
        // 转换DMX数据
        QByteArray dmxData = convertArtnetPacketToDmx(artnetPacket);
        frame.setDmxData(dmxData);
        
        // 发送数据帧
        bool success = m_artnetTransmitter->sendFrame(frame);
        if (!success) {
            widget->setStatus(QString("Universe %1 发送失败").arg(frame.universe), false);
        } else {
            widget->setStatus(QString("Universe %1 正在发送...").arg(frame.universe), true);
        }
    }
    
    /**
     * 将ArtnetUniverseNode的数据包转换为DMX数据
     * @param artnetPacket ArtnetUniverseNode输出的数据包
     * @return DMX字节数组（512字节）
     */
    QByteArray convertArtnetPacketToDmx(const QVariantMap &artnetPacket) {
        QByteArray dmxData(512, 0);  // 初始化512个通道为0
        
        // 从数据包中提取DMX数据
        QVariantList dmxList = artnetPacket["dmxData"].toList();
        
        // 确保数据长度正确
        int dataLength = qMin(dmxList.size(), 512);
        for (int i = 0; i < dataLength; ++i) {
            int value = dmxList[i].toInt();
            value = qBound(0, value, 255);  // 限制在0-255范围内
            dmxData[i] = static_cast<char>(value);
        }
        
        return dmxData;
    }
    
private:
    std::shared_ptr<VariableData> m_outData;
    
    ArtnetTransmitter* m_artnetTransmitter;  // ArtnetTransmitter单例
    ArtnetOutInterface* widget = new ArtnetOutInterface();  // 界面控件
    
    // 配置参数
    QString m_targetHost;     // 目标主机地址
    
    // Universe数据缓存 - 键为端口索引，值为ArtnetUniverseNode的输出数据包
    QMap<int, QVariantMap> m_universeCache;
};
