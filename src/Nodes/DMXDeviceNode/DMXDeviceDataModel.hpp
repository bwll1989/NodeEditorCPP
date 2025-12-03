#pragma once

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "DMXDeviceInterface.hpp"
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
     * @brief 通道设备节点 - 多通道数据处理节点
     * 支持设置起始DMX地址、通道数，每个通道的值，并通过输入和输出端口接收和发送值
     */
    class DMXDeviceDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，初始化通道设备节点
         */
        DMXDeviceDataModel()
        {
            InPortCount = 5;   // 默认5个输入端口
            OutPortCount = 1;   // 只有一个通道数据输出
            PortEditable=true;
            CaptionVisible = true;
            Caption = "DMX Device";
            WidgetEmbeddable = false;
            Resizable = false;
            
            // 初始化数据
            channelData = std::make_shared<VariableData>();
            
            // 初始化通道值数组（默认5个通道，值为0）
            channelValues.resize(InPortCount, 0);
            
            // 注册OSC控制
            NodeDelegateModel::registerOSCControl("/channels", widget->channelCountSpinBox);
            NodeDelegateModel::registerOSCControl("/startAddress", widget->startAddressSpinBox);
            NodeDelegateModel::registerOSCControl("/enable", widget->enableCheckBox);
            connect(widget->channelCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
                    this, &DMXDeviceDataModel::onChannelCountChanged);
            connect(widget->startAddressSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
                    this, &DMXDeviceDataModel::onStartAddressChanged);
            
            // 连接初始通道滑块信号
            connectChannelSliders();
            
            // 初始化状态
            updateAddressRangeDisplay();
            updateChannelData();
        }

        /**
         * @brief 析构函数
         */
        ~DMXDeviceDataModel() override {

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
                return QString("CH%1").arg(portIndex);  // 从CH0开始，对应设备通道
            } else {
                return "CHANNELS";    // 通道数据输出
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
            return channelData;   // 通道数据
        }

        /**
         * @brief 处理输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr || portIndex >= InPortCount) {
                return;
            }
            
            auto variableData = std::dynamic_pointer_cast<VariableData>(data);
            if (!variableData) {
                return;
            }

            // 设置对应通道的值（如果通道索引在当前通道数范围内）
            int channelCount = widget->channelCountSpinBox->value();
            if (portIndex < channelCount) {
                int channelValue = qBound(0, variableData->value().toInt(), 255);
                setChannelValue(portIndex, channelValue);
                
                // 更新界面滑块（如果控件存在）
                QSlider* slider = widget->getChannelSlider(portIndex);
                QLabel* label = widget->getChannelLabel(portIndex);
                if (slider && label) {
                    slider->setValue(channelValue);

                }
            }
        }

        /**
         * @brief 保存节点状态
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["channelCount"] = widget->channelCountSpinBox->value();
            modelJson1["startAddress"] = widget->startAddressSpinBox->value();
            modelJson1["enabled"] = widget->enableCheckBox->isChecked();
            
            // 保存通道值
            QJsonArray channelArray;
            for (int i = 0; i < channelValues.size(); ++i) {
                channelArray.append(channelValues[i]);
            }
            modelJson1["channelValues"] = channelArray;
            
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["ChannelSettings"] = modelJson1;
            return modelJson;
        }

        /**
         * @brief 加载节点状态
         * @param p JSON对象
         */
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["ChannelSettings"];
            if (!v.isUndefined() && v.isObject()) {
                auto settings = v.toObject();
                
                widget->channelCountSpinBox->setValue(settings["channelCount"].toInt(16));
                widget->startAddressSpinBox->setValue(settings["startAddress"].toInt(1));
                widget->enableCheckBox->setChecked(settings["enabled"].toBool(true));
                
                // 加载通道值
                QJsonArray channelArray = settings["channelValues"].toArray();
                for (int i = 0; i < channelArray.size() && i < channelValues.size(); ++i) {
                    channelValues[i] = channelArray[i].toInt();
                }
                
                // 更新界面
                updateAddressRangeDisplay();
                updateChannelSliders();
                updateChannelData();
                
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
         * @brief 通道数量改变时的处理
         * @param channelCount 新的通道数量
         */
        void onChannelCountChanged(int channelCount) {
            // 调整通道值数组大小
            int oldSize = channelValues.size();
            channelValues.resize(channelCount, 0);
            
            // 如果增加了通道，新通道值初始化为0
            for (int i = oldSize; i < channelCount; ++i) {
                channelValues[i] = 0;
            }
            
            // 更新界面控件数量
            widget->updateChannelControls(channelCount);
            
            // 重新连接信号
            connectChannelSliders();
            
            // 更新界面显示
            updateAddressRangeDisplay();
            updateChannelSliders();
            updateChannelData();
        }

        /**
         * @brief 起始地址改变时的处理
         * @param startAddr 新的起始地址
         */
        void onStartAddressChanged(int startAddr) {
            Q_UNUSED(startAddr)
            // 更新界面显示
            updateAddressRangeDisplay();
            updateChannelData();
        }

    private:
        /**
         * @brief 连接通道滑块信号
         */
        void connectChannelSliders() {
            int channelCount = widget->getChannelCount();
            for (int i = 0; i < channelCount; ++i) {
                
                QSlider* slider = widget->getChannelSlider(i);
                 NodeDelegateModel::registerOSCControl("/channels" + QString::number(i), slider);
                if (slider) {
                    // 断开之前的连接（如果有）
                    // disconnect(slider, &QSlider::valueChanged, nullptr, nullptr);
                    
                    // 连接新的信号
                    connect(slider, &QSlider::valueChanged, 
                            this, [this, i](int value) {
                        setChannelValue(i, value);
                    });
                }
            }
        }

        /**
         * @brief 设置指定通道的值
         * @param channel 通道索引
         * @param value 通道值（0-255）
         */
        void setChannelValue(int channel, int value) {
            if (channel >= 0 && channel < channelValues.size()) {
                channelValues[channel] = qBound(0, value, 255);
                updateChannelData();
            }
        }

        /**
         * @brief 更新通道数据输出
         */
        void updateChannelData() {
            channelData = std::make_shared<VariableData>();
            
            // 创建包含DMX地址映射的数据
            QVariantMap dmxData;
            int startAddr = widget->startAddressSpinBox->value();
            int channelCount = widget->channelCountSpinBox->value();
            bool enabled = widget->enableCheckBox->isChecked();
            
            // 添加基本信息
            dmxData["startAddress"] = startAddr;
            dmxData["channelCount"] = channelCount;
            dmxData["endAddress"] = startAddr + channelCount - 1;
            dmxData["enabled"] = enabled;
            
            // 创建通道数据列表（只包含当前设置的通道数量）
            QVariantList channelList;
            QVariantMap addressMap;  // DMX地址到值的映射
            
            for (int i = 0; i < channelCount; ++i) {
                // 根据启用状态决定发送的值
                int outputValue = enabled ? channelValues[i] : 0;
                channelList.append(outputValue);
                addressMap[QString::number(startAddr + i)] = outputValue;
            }
            
            dmxData["channels"] = channelList;
            dmxData["addressMap"] = addressMap;
            
            // 设置输出数据
            channelData->insert("default", dmxData);
            
            // 发出数据更新信号
            Q_EMIT dataUpdated(0);
        }

        /**
         * @brief 更新地址范围显示
         */
        void updateAddressRangeDisplay() {
            int startAddr = widget->startAddressSpinBox->value();
            int channelCount = widget->channelCountSpinBox->value();
            widget->updateAddressRangeLabel(startAddr, channelCount);
        }

        /**
         * @brief 更新通道标签显示
         */


        /**
         * @brief 更新通道滑块显示
         */
        void updateChannelSliders() {
            int channelCount = widget->getChannelCount();
            
            for (int i = 0; i < channelCount; ++i) {
                QSlider* slider = widget->getChannelSlider(i);
                if (slider) {
                    slider->setValue(channelValues[i]);
                }
            }
        }

    private:
        std::shared_ptr<VariableData> channelData;  // 通道数据输出
        QVector<int> channelValues;                 // 通道值数组
        
        Nodes::DMXDeviceInterface * widget = new Nodes::DMXDeviceInterface();
    };
}