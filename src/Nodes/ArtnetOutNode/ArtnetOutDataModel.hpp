#pragma once

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QtCore/qglobal.h>
#include "ArtnetService.h"
#include "ArtnetFrame.h"
// #include "ArtnetOutInterface.hpp"
#include <QVariantMap>
#include "QThread"
#include <QTimer>
#include <QDateTime>
#include <QSignalBlocker>
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

struct GlobalEvent;

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
class ArtnetOutDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString targetHost READ targetHost WRITE setTargetHost NOTIFY targetHostChanged)
    Q_PROPERTY(bool isReady READ isReady NOTIFY isReadyChanged)

public:
    QString targetHost() const { return m_targetHost; }
    bool isReady() const { return m_isReady; }

    public Q_SLOTS:
    void setTargetHost(const QString &host) {
        if (m_targetHost == host) return;
        m_targetHost = host;
        Q_EMIT targetHostChanged(m_targetHost);
        // 目标地址变化：撤销旧 host 下本节点的全部状态桶，并按新 host 重推当前缓存数据
        refreshOutputs();
    }

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
        
        // 获取ArtnetService单例
        m_artnetService = ArtnetService::getInstance();
        setIsReady(m_artnetService != nullptr);

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "targetHost";
            AbstractDelegateModel::registerExternalBinding("/targetHost", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "isReady";
            AbstractDelegateModel::registerExternalBinding("/isReady", this, b);
        }

        // 连接ArtnetService信号
        if (m_artnetService) {
            connect(m_artnetService, &ArtnetService::frameSent, this, &ArtnetOutDataModel::onFrameSent);
            connect(m_artnetService, &ArtnetService::frameSendFailed, this, &ArtnetOutDataModel::onFrameSendFailed);
        }
        
        // 设置默认值
        setTargetHost("192.168.0.255");  // 默认广播地址
        
        // 初始化Universe数据缓存
        m_universeCache.clear();
    }

    /**
     * 析构函数 - 清理资源：从状态表撤销本节点名下的全部输出桶，
     * 避免悬空属主标签残留（默认保持最后状态可改用 releaseOwner(this, true) 归零）
     */
    ~ArtnetOutDataModel() override {
        if (m_artnetService) {
            m_artnetService->releaseOwner(this);
        }
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
            // 如果数据为空，移除对应的Universe缓存，
            // 并从状态表撤销本节点旧桶后按剩余缓存重推（幂等，无需新旧值 diff）
            m_universeCache.remove(portIndex);
            refreshOutputs();
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
        
        // 写入状态表（50Hz 心跳负责实际发送，这里只报最新值）
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
            setTargetHost(obj["TargetHost"].toString("192.168.0.255"));
        }
    }
    
    // /**
    //  * 获取嵌入式控件
    //  * @return 控件指针
    //  */
    // QWidget *embeddedWidget() override {
    //     /**
    //      * 函数级注释：懒加载创建界面控件
    //      * - 无头模式下 embeddedWidget 不会被调用，从而避免创建 QWidget 节省资源
    //      */
    //     if (!widget) {
    //         widget = new ArtnetOutInterface();
    //
    //         {
    //             NodeDelegateModel::ExternalBinding b;
    //             b.member = "targetHost";
    //             b.control = widget->targetHostEdit;
    //             AbstractDelegateModel::registerExternalBinding("/targetHost", this, b);
    //         }
    //
    //         {
    //             const QSignalBlocker blocker(widget->targetHostEdit);
    //             widget->setTargetHost(m_targetHost);
    //         }
    //
    //         if (!m_isReady) {
    //             widget->setStatus("ArtnetService未初始化", false);
    //         }
    //
    //         connect(widget, &ArtnetOutInterface::TargetHostChanged, this, &ArtnetOutDataModel::setTargetHost);
    //     }
    //     return widget;
    // }
    
protected:
    /**
     * 函数级注释：模型就绪后订阅全局事件总线，实现外部命令控制 targetHost
     */
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/targetHost"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

private slots:
    /**
     * 函数级注释：处理来自全局事件总线的命令，更新 targetHost 属性
     */
    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address != makeFullOscAddress("/targetHost")) {
            return;
        }
        setTargetHost(ev.payload.toString());
    }
    
    /**
     * 数据帧发送成功处理
     *
     * 50Hz 固定重发模式下本槽会高频触发（且单例 frameSent 是广播，
     * 所有 ArtnetOutNode 实例都会收到所有帧事件），
     * 因此成功状态最多每 500ms 汇总推送一次，避免下游图被刷爆；
     * 失败事件不受节流，立即推送。
     * @param frame 发送成功的数据帧
     * @param bytesWritten 发送的字节数
     */
    void onFrameSent(const ArtnetFrame &frame, qint64 bytesWritten) {
        setIsReady(true);

        // 状态推送节流：距上次推送不足 500ms 则丢弃本次成功事件
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - m_lastStatusEmitMs < 500) {
            return;
        }
        m_lastStatusEmitMs = now;

        QVariantMap statusData;
        statusData["status"] = "success";
        statusData["universe"] = frame.universe;
        statusData["host"] = frame.host;
        statusData["bytesWritten"] = bytesWritten;
        statusData["timestamp"] = QDateTime::currentDateTime().toString();

        m_outData = std::make_shared<VariableData>(statusData);
        Q_EMIT dataUpdated(0);
    }
    
    /**
     * 数据帧发送失败处理（不受节流，立即推送，便于及时发现网络问题）
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

        m_lastStatusEmitMs = QDateTime::currentMSecsSinceEpoch(); // 失败后重置节流窗口，后续成功状态可及时反馈恢复
        setIsReady(false);
    }
    
private:
    /**
     * 刷新本节点在状态表中的全部输出：先撤销旧桶，再按当前缓存重推
     * 幂等套路：用于 targetHost 变化、端口断开等配置变更场景，无需新旧值 diff
     */
    void refreshOutputs() {
        if (!m_artnetService) {
            return;
        }
        m_artnetService->releaseOwner(this);
        for (auto it = m_universeCache.constBegin(); it != m_universeCache.constEnd(); ++it) {
            sendUniverseData(it.value());
        }
    }

    /**
     * 将指定的Universe数据写入状态表（由 50Hz 心跳负责实际发送）
     * @param artnetPacket ArtnetUniverseNode输出的数据包
     */
    void sendUniverseData(const QVariantMap &artnetPacket) {
        if (!m_artnetService) {
            setIsReady(false);
            // if (widget) {
            //     widget->setStatus("ArtnetService未初始化", false);
            // }
            return;
        }
        
        // 创建ArtnetFrame
        ArtnetFrame frame;
        frame.host = m_targetHost;
        frame.sequence = 0;
        frame.timestamp = QDateTime::currentMSecsSinceEpoch();
        
        // 从ArtnetUniverseNode的数据包中提取Universe信息
        frame.universe = static_cast<quint16>(artnetPacket["fullUniverse"].toInt());
        
        // 转换DMX数据
        QByteArray dmxData = convertArtnetPacketToDmx(artnetPacket);
        frame.setDmxData(dmxData);
        
        // 写入状态表（owner = 本节点，供多源合并与释放时定位归属）
        m_artnetService->updateFrame(frame, this);

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
signals:
    void targetHostChanged(const QString &host);
    void isReadyChanged(bool ready);

private:
    void setIsReady(bool ready)
    {
        if (m_isReady == ready) {
            return;
        }
        m_isReady = ready;
        Q_EMIT isReadyChanged(m_isReady);
    }

private:
    std::shared_ptr<VariableData> m_outData;
    
    ArtnetService* m_artnetService;  // ArtnetService单例
    qint64 m_lastStatusEmitMs = 0;           // 上次 STATUS 状态推送时间（节流用，避免 50Hz 刷爆下游）
    // ArtnetOutInterface* widget = nullptr;  // 界面控件（懒加载）
    
    // 配置参数
    QString m_targetHost;     // 目标主机地址
    bool m_isReady = false;
    
    // Universe数据缓存 - 键为端口索引，值为ArtnetUniverseNode的输出数据包
    QMap<int, QVariantMap> m_universeCache;
};
