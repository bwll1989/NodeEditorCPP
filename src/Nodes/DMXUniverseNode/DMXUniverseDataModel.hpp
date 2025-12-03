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
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
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
    class DMXUniverseDataModel : public NodeDelegateModel
    {
        Q_OBJECT

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
            Caption = "Artnet Universe";
            WidgetEmbeddable = false;
            Resizable = false;
            
            // 初始化512通道DMX数据（全部为0）
            dmxData.resize(512, 0);
            
            // 初始化输出数据
            universeData = std::make_shared<VariableData>();
            
            // 连接界面信号
            connect(widget->universeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
                    this, &DMXUniverseDataModel::onUniverseChanged);
            connect(widget->subnetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
                    this, &DMXUniverseDataModel::onSubnetChanged);
            connect(widget->netSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
                    this, &DMXUniverseDataModel::onNetChanged);
            
            // 连接清空按钮信号
            connect(widget, &Nodes::DMXUniverseInterface::clearDataClicked,
                    this, &DMXUniverseDataModel::onClearDataClicked);
            
            // 注册OSC控制
            NodeDelegateModel::registerOSCControl("/universe", widget->universeSpinBox);
            NodeDelegateModel::registerOSCControl("/subnet", widget->subnetSpinBox);
            NodeDelegateModel::registerOSCControl("/net", widget->netSpinBox);
            NodeDelegateModel::registerOSCControl("/clear", widget->clearButton);
            // 初始化输出数据
            updateUniverseData();
        }

        /**
         * @brief 析构函数
         */
        ~DMXUniverseDataModel() override {
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
            modelJson1["universe"] = widget->universeSpinBox->value();
            modelJson1["subnet"] = widget->subnetSpinBox->value();
            modelJson1["net"] = widget->netSpinBox->value();
            
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
                
                widget->universeSpinBox->setValue(settings["universe"].toInt(0));
                widget->subnetSpinBox->setValue(settings["subnet"].toInt(0));
                widget->netSpinBox->setValue(settings["net"].toInt(0));
                
                // 更新输出数据
                updateUniverseData();
                
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

        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }

    private slots:
        /**
         * @brief Universe改变时的处理
         * @param universe 新的Universe值
         */
        void onUniverseChanged(int universe) {
            Q_UNUSED(universe)
            updateUniverseData();
        }

        /**
         * @brief Subnet改变时的处理
         * @param subnet 新的Subnet值
         */
        void onSubnetChanged(int subnet) {
            Q_UNUSED(subnet)
            updateUniverseData();
        }

        /**
         * @brief Net改变时的处理
         * @param net 新的Net值
         */
        void onNetChanged(int net) {
            Q_UNUSED(net)
            updateUniverseData();
        }

        /**
         * @brief 清空数据按钮点击处理
         * 将当前Universe的所有512个DMX通道清零
         */
        void onClearDataClicked() {
            // 清空所有DMX数据
            dmxData.fill(0);
            
            // 更新输出数据
            updateUniverseData();
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
            int universe = widget->universeSpinBox->value();
            int subnet = widget->subnetSpinBox->value();
            int net = widget->netSpinBox->value();
            
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
    };
}