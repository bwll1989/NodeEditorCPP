/**
 * @file PosiStageNetClientDataModel.hpp
 * @brief PosiStageNet(PSN)舞台定位协议接收节点
 *
 * 功能：
 *   - 通过 UDP 组播接收 PSN 协议数据（默认组播 236.10.10.10:56565）
 *   - 解码追踪器的位置、速度、朝向、状态等信息
 *   - 输出端口：CONNECTED / TRACKER_COUNT / TRACKERS(JSON)
 */
#pragma once

#include <memory>
#include <map>

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QHostAddress>
#include <QUdpSocket>
#include <QSignalBlocker>
#include <QVariantMap>

#include <psn_defs.hpp>

namespace psn {
class psn_decoder;
}

#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "PosiStageNetClientInterface.hpp"
#include "PluginDefinition.hpp"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    class PosiStageNetClientDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        static constexpr int kReceivePollIntervalMs = 5;
        static constexpr int kConnectionTimeoutMs = 2000;
        static constexpr const char *kDefaultMulticastAddr = "236.10.10.10";
        static constexpr quint16 kDefaultPort = 56565;

        enum InputPort : PortIndex { EnablePort = 0 };
        enum OutputPort : PortIndex {
            ConnectedPort = 0,
            TrackerCountPort = 1,
            TrackersDataPort = 2
        };

        Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
        Q_PROPERTY(QString multicastAddress READ multicastAddress WRITE setMulticastAddress NOTIFY multicastAddressChanged)
        Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)

    public:
        PosiStageNetClientDataModel();
        ~PosiStageNetClientDataModel() override;

        NodeDataType dataType(PortType, PortIndex) const override;
        QString portCaption(PortType portType, PortIndex portIndex) const override;
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override;
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override;
        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex) const override;
        QJsonObject save() const override;
        void load(const QJsonObject &p) override;
        QWidget *embeddedWidget() override;

        bool enabled() const { return m_enabled; }
        bool connected() const { return m_connected; }
        QString multicastAddress() const { return m_multicastAddress; }
        int port() const { return m_port; }

        void setEnabled(bool enabled);
        void setMulticastAddress(const QString &address);
        void setPort(int port);
    protected:
        void afterModelReady() override;
    Q_SIGNALS:
        void enabledChanged(bool enabled);
        void connectedChanged(bool connected);
        void multicastAddressChanged(const QString &address);
        void portChanged(int port);

    private:
        /**
         * @brief 启动 UDP 组播接收（bind + joinMulticastGroup）
         * @return 成功返回 true
         */
        bool startReceiver();

        /**
         * @brief 停止 UDP 组播接收（leave + close）
         */
        void stopReceiver();

        /**
         * @brief 设置连接状态并更新 UI/输出
         */
        void setConnected(bool connected);

        /**
         * @brief 将当前 PSN 追踪器数据打包为 QVariantMap（用于输出端口）
         */
        QVariantMap buildTrackersVariantMap() const;

        /**
         * @brief 辅助：判断 VariableData 是否为“真值”
         */
        static bool isBoolTrue(const VariableData &data);

    private slots:
        /**
         * @brief 定时轮询接收 UDP 数据并解码
         */
        void onReceivePoll();

        /**
         * @brief 连接超时检测：超过 kConnectionTimeoutMs 未收到数据则标记为断连
         */
        void onConnectionWatchdog();
        void onGlobalEvent(const GlobalEvent &ev);
    private:
        PosiStageNetClientInterface *widget = nullptr;

        QUdpSocket *m_udpSocket = nullptr;
        QTimer *m_receiveTimer = nullptr;
        QTimer *m_watchdogTimer = nullptr;

        std::unique_ptr<::psn::psn_decoder> m_psnDecoder;
        uint8_t m_lastFrameId = 0;
        qint64 m_lastReceiveTimeMs = 0;
        QString m_multicastAddress = kDefaultMulticastAddr;
        int m_port = kDefaultPort;

        std::shared_ptr<VariableData> m_connectedOutput;
        std::shared_ptr<VariableData> m_trackerCountOutput;
        std::shared_ptr<VariableData> m_trackersDataOutput;

        bool m_enabled = false;
        bool m_connected = false;
        bool m_bound = false;
        bool m_destructing = false;
    };
}
