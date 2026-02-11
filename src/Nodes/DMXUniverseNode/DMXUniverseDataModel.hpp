#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "DMXUniverseInterface.hpp"
#include <QVariantMap>
#include <QByteArray>
#include <QJsonArray>
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include <QSignalBlocker>

struct GlobalEvent;
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief Artnet Universe节点 - 合并多个ArtnetDevice输出为完整Universe数据包
     * 接收多个ArtnetDevice的输出，合并成带有Universe信息的512通道DMX数据包
     */
    class DMXUniverseDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int universe READ universe WRITE setUniverse NOTIFY universeChanged)
        Q_PROPERTY(int subnet READ subnet WRITE setSubnet NOTIFY subnetChanged)
        Q_PROPERTY(int net READ net WRITE setNet NOTIFY netChanged)

    public:
        /**
         * @brief 构造函数，初始化Artnet Universe节点
         */
        DMXUniverseDataModel()
        {
            InPortCount = 8;   // 默认8个输入端口，用于接收ArtnetDevice输出
            OutPortCount = 1;   // 一个Universe数据输出
            PortEditable = true;
            CaptionVisible = true;
            Caption = "DMX Universe";
            WidgetEmbeddable = false;
            Resizable = false;
            
            // 初始化512通道DMX数据（全部为0）
            dmxData.resize(512, 0);
            
            // 初始化输出数据
            universeData = std::make_shared<VariableData>();
            
            // 连接界面信号
            connect(widget->universeSpinBox, QOverload<int>::of(&IntDragValueWidget::valueChanged),
                    this, &DMXUniverseDataModel::setUniverse);
            connect(widget->subnetSpinBox, QOverload<int>::of(&IntDragValueWidget::valueChanged),
                    this, &DMXUniverseDataModel::setSubnet);
            connect(widget->netSpinBox, QOverload<int>::of(&IntDragValueWidget::valueChanged),
                    this, &DMXUniverseDataModel::setNet);
            
            // 连接清空按钮信号
            connect(widget, &Nodes::DMXUniverseInterface::clearDataClicked,
                    this, &DMXUniverseDataModel::onClearDataClicked);
            
            // 注册OSC控制
            AbstractDelegateModel::registerExternalControl("/universe", widget->universeSpinBox);
            AbstractDelegateModel::registerExternalControl("/subnet", widget->subnetSpinBox);
            AbstractDelegateModel::registerExternalControl("/net", widget->netSpinBox);
            AbstractDelegateModel::registerExternalControl("/clear", widget->clearButton);
            // 初始化输出数据
            updateUniverseData();
        }
        /**
                 * @brief 析构函数
                 */
        ~DMXUniverseDataModel() override {
        }
        int universe() const { return m_universe; }
        int subnet() const { return m_subnet; }
        int net() const { return m_net; }

    public slots:
        void setUniverse(int universe) {
            if (m_universe == universe) {
                return;
            }
            m_universe = universe;
            if (widget) {
                const QSignalBlocker blocker(widget->universeSpinBox);
                widget->universeSpinBox->setValue(universe);
            }
            updateUniverseData();
            Q_EMIT universeChanged(universe);
            AbstractDelegateModel::stateFeedBack("/universe", universe);
        }

        void setSubnet(int subnet) {
            if (m_subnet == subnet) {
                return;
            }
            m_subnet = subnet;
            if (widget) {
                const QSignalBlocker blocker(widget->subnetSpinBox);
                widget->subnetSpinBox->setValue(subnet);
            }
            updateUniverseData();
            Q_EMIT subnetChanged(subnet);
            AbstractDelegateModel::stateFeedBack("/subnet", subnet);
        }

        void setNet(int netValue) {
            if (m_net == netValue) {
                return;
            }
            m_net = netValue;
            if (widget) {
                const QSignalBlocker blocker(widget->netSpinBox);
                widget->netSpinBox->setValue(netValue);
            }
            updateUniverseData();
            Q_EMIT netChanged(netValue);
            AbstractDelegateModel::stateFeedBack("/net", netValue);
        }

        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addrUniverse = makeFullOscAddress("/universe");
            const QString addrSubnet = makeFullOscAddress("/subnet");
            const QString addrNet = makeFullOscAddress("/net");
            const QString addrClear = makeFullOscAddress("/clear");

            if (ev.address == addrUniverse) {
                setUniverse(ev.payload.toInt());
            } else if (ev.address == addrSubnet) {
                setSubnet(ev.payload.toInt());
            } else if (ev.address == addrNet) {
                setNet(ev.payload.toInt());
            } else if (ev.address == addrClear) {
                onClearDataClicked();
            }
        }



        /**
         * @brief 获取端口标题
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 端口标题
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                return QString("DEVICE%1").arg(portIndex + 1);  // DEVICE1, DEVICE2, ...
            } else {
                return "UNIVERSE";    // Universe数据输出
            }
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
         * @param port 端口索引
         * @return 输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port)
            return universeData;   // Universe数据
        }

        /**
         * @brief 处理输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }
            
            auto variableData = std::dynamic_pointer_cast<VariableData>(data);
            if (!variableData) {
                return;
            }

            // 解析ArtnetDevice的输出数据
            QVariant defaultValue = variableData->value("default");
            if (!defaultValue.isValid()) {
                return;
            }
            
            QVariantMap deviceData = defaultValue.toMap();
            if (deviceData.isEmpty()) {
                return;
            }

            // 获取设备的DMX地址映射
            QVariantMap addressMap = deviceData["addressMap"].toMap();
            if (!addressMap.isEmpty()) {
                // 将设备数据合并到Universe的512通道中
                for (auto it = addressMap.begin(); it != addressMap.end(); ++it) {
                    int dmxAddress = it.key().toInt();
                    int value = it.value().toInt();
                    
                    // 确保DMX地址在有效范围内（1-512）
                    if (dmxAddress >= 1 && dmxAddress <= 512) {
                        dmxData[dmxAddress - 1] = qBound(0, value, 255);  // 转换为0-511索引
                    }
                }
                
                // 更新输出数据
                updateUniverseData();
            }
        }

        /**
         * @brief 保存节点状态
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["universe"] = m_universe;
            modelJson1["subnet"] = m_subnet;
            modelJson1["net"] = m_net;
            
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["UniverseSettings"] = modelJson1;
            return modelJson;
        }

        /**
         * @brief 加载节点状态
         * @param p JSON对象
         */
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["UniverseSettings"];
            if (!v.isUndefined() && v.isObject()) {
                auto settings = v.toObject();

                setUniverse(settings["universe"].toInt(0));
                setSubnet(settings["subnet"].toInt(0));
                setNet(settings["net"].toInt(0));

                NodeDelegateModel::load(p);
            }
        }

        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
        QWidget *embeddedWidget() override {
            return widget;
        }


    private slots:
        /**
         * @brief 清空数据按钮点击处理
         * 将当前Universe的所有512个DMX通道清零
         */
        void onClearDataClicked() {
            dmxData.fill(0);
            updateUniverseData();
            AbstractDelegateModel::stateFeedBack("/clear", true);
        }

    Q_SIGNALS:
        void universeChanged(int universe);
        void subnetChanged(int subnet);
        void netChanged(int net);

    protected:
        void afterModelReady() override {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/universe"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/subnet"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/net"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/clear"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private:
        /**
         * @brief 更新Universe数据输出
         */
        void updateUniverseData() {
            universeData = std::make_shared<VariableData>();
            
            // 创建完整的Artnet Universe数据包
            QVariantMap artnetPacket;
            
            // Artnet协议头信息
            artnetPacket["protocol"] = "Art-Net";
            artnetPacket["version"] = 14;  // Artnet协议版本
            artnetPacket["opcode"] = 0x5000;  // ArtDMX操作码
            
            // Universe寻址信息
            int universe = m_universe;
            int subnet = m_subnet;
            int net = m_net;
            
            artnetPacket["universe"] = universe;
            artnetPacket["subnet"] = subnet;
            artnetPacket["net"] = net;
            
            // 计算完整的Universe地址
            int fullUniverse = (net << 8) | (subnet << 4) | universe;
            artnetPacket["fullUniverse"] = fullUniverse;
            
            // DMX数据
            QVariantList dmxList;
            for (int i = 0; i < 512; ++i) {
                dmxList.append(dmxData[i]);
            }
            artnetPacket["dmxData"] = dmxList;
            artnetPacket["dataLength"] = 512;
            
            // 统计信息
            int activeChannels = 0;
            int maxValue = 0;
            for (int i = 0; i < 512; ++i) {
                if (dmxData[i] > 0) {
                    activeChannels++;
                }
                maxValue = qMax(maxValue, dmxData[i]);
            }
            artnetPacket["activeChannels"] = activeChannels;
            artnetPacket["maxValue"] = maxValue;
            
            // 时间戳
            artnetPacket["timestamp"] = QDateTime::currentMSecsSinceEpoch();
            
            // 设置输出数据
            universeData->insert("default", artnetPacket);
            
            // 发出数据更新信号
            Q_EMIT dataUpdated(0);
        }

    private:
        std::shared_ptr<VariableData> universeData;  // Universe数据输出
        QVector<int> dmxData;                        // 512通道DMX数据
        
        Nodes::DMXUniverseInterface * widget = new Nodes::DMXUniverseInterface();
        int m_universe = 0;
        int m_subnet = 0;
        int m_net = 0;
    };
}
