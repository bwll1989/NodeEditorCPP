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
     * @brief 通道设备节点 - 多通道数据处理节点
     * 支持设置起始DMX地址、通道数，每个通道的值，并通过输入和输出端口接收和发送值
     */
    class DMXDeviceDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int channelCount READ channelCount WRITE setChannelCount NOTIFY channelCountChanged)
        Q_PROPERTY(int startAddress READ startAddress WRITE setStartAddress NOTIFY startAddressChanged)
        Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

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
            AbstractDelegateModel::registerExternalControl("/channels", widget->channelCountSpinBox);
            AbstractDelegateModel::registerExternalControl("/startAddress", widget->startAddressSpinBox);
            AbstractDelegateModel::registerExternalControl("/enable", widget->enableCheckBox);
            connect(widget->channelCountSpinBox, QOverload<int>::of(&IntDragValueWidget::valueChanged),
                    this, &DMXDeviceDataModel::setChannelCount);
            connect(widget->startAddressSpinBox, QOverload<int>::of(&IntDragValueWidget::valueChanged),
                    this, &DMXDeviceDataModel::setStartAddress);
            connect(widget->enableCheckBox, &QCheckBox::clicked, this, &DMXDeviceDataModel::setEnabled);
            
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

        int channelCount() const { return m_channelCount; }
        int startAddress() const { return m_startAddress; }
        bool enabled() const { return m_enabled; }

    public slots:
        void setChannelCount(int count) {
            if (m_channelCount == count) return;
            
            int oldSize = m_channelCount;
            m_channelCount = count;
            // 更新订阅
            updateChannelSubscriptions(oldSize, m_channelCount);

            channelValues.resize(m_channelCount, 0);
            for (int i = oldSize; i < m_channelCount; ++i) {
                channelValues[i] = 0;
            }

            if (widget) {
                // 防止递归调用，并在控件数量更新后统一注册和绑定滑块
                const QSignalBlocker blocker(widget->channelCountSpinBox);
                widget->channelCountSpinBox->setValue(count);
                widget->updateChannelControls(count);
             
            }

            // 在通道控件更新完成后，统一为所有滑块做外部控制注册与信号连接
            connectChannelSliders();
            updateAddressRangeDisplay();
            updateChannelSliders();
            updateChannelData();
            Q_EMIT channelCountChanged(count);
            AbstractDelegateModel::stateFeedBack("/channels", count);

        }

        void setStartAddress(int addr) {
             if (m_startAddress == addr) return;
             m_startAddress = addr;
             if(widget) {
                 const QSignalBlocker blocker(widget->startAddressSpinBox);
                 widget->startAddressSpinBox->setValue(addr);
             }
             updateAddressRangeDisplay();
             updateChannelData();
             Q_EMIT startAddressChanged(addr);
             AbstractDelegateModel::stateFeedBack("/startAddress", addr);
        }

        void setEnabled(bool enable) {
            if (m_enabled == enable) return;
            m_enabled = enable;
             if(widget) {
                 const QSignalBlocker blocker(widget->enableCheckBox);
                 widget->enableCheckBox->setChecked(enable);
             }
            updateChannelData();
            Q_EMIT enabledChanged(enable);
            AbstractDelegateModel::stateFeedBack("/enable", enable);
        }

        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;

            const QString addrChannels = makeFullOscAddress("/channels");
            const QString addrStart = makeFullOscAddress("/startAddress");
            const QString addrEnable = makeFullOscAddress("/enable");

            if (ev.address == addrChannels) {
                setChannelCount(ev.payload.toInt());
            } else if (ev.address == addrStart) {
                setStartAddress(ev.payload.toInt());
            } else if (ev.address == addrEnable) {
                setEnabled(ev.payload.toBool());
            } else if (ev.address.startsWith(addrChannels + "/")) {
                // 处理 /channels/N
                QString suffix = ev.address.mid(addrChannels.length() + 1);
                bool ok;
                int idx = suffix.toInt(&ok);
                if (ok) {
                    updateChannelState(idx, ev.payload.toInt());
                }
            }
        }
    
    Q_SIGNALS:
        void channelCountChanged(int count);
        void startAddressChanged(int addr);
        void enabledChanged(bool enable);

    protected:
        void afterModelReady() override {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/channels"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/startAddress"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
            
            // 初始订阅通道
            updateChannelSubscriptions(0, m_channelCount);
        }

    private:
        void updateChannelSubscriptions(int oldIdx, int newIdx) {
            // 增加订阅
            for (int i = oldIdx; i < newIdx; ++i) {
                QString addr = makeFullOscAddress(QString("/channels/%1").arg(i));
                GlobalEventBus::instance()->subscribe(addr, this, SLOT(onGlobalEvent(GlobalEvent)));
            }
            // 减少订阅
            for (int i = newIdx; i < oldIdx; ++i) {
                QString addr = makeFullOscAddress(QString("/channels/%1").arg(i));
                GlobalEventBus::instance()->unsubscribe(addr, this);
            }
        }
        
        void updateChannelState(int index, int value) {
             if (index >= 0 && index < channelValues.size()) {
                 setChannelValue(index, value);
                 if (widget) {
                     IntDragValueWidget* slider = widget->getChannelSlider(index);
                     if (slider) {
                         const QSignalBlocker blocker(slider);
                         slider->setValue(value);
                     }
                 }
             }
        }
        /**
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
            if (portIndex < m_channelCount) {
                int channelValue = qBound(0, variableData->value().toInt(), 255);
                updateChannelState(portIndex, channelValue);
            }
        }

        /**
         * @brief 保存节点状态
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["channelCount"] = m_channelCount;
            modelJson1["startAddress"] = m_startAddress;
            modelJson1["enabled"] = m_enabled;
            
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
                
                setChannelCount(settings["channelCount"].toInt(16));
                setStartAddress(settings["startAddress"].toInt(1));
                setEnabled(settings["enabled"].toBool(true));
                
                // 加载通道值
                QJsonArray channelArray = settings["channelValues"].toArray();
                for (int i = 0; i < channelArray.size() && i < channelValues.size(); ++i) {
                    channelValues[i] = channelArray[i].toInt();
                }
                
                // 更新界面
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

    private:
        /**
         * @brief 连接通道滑块信号，并将每个滑块注册到外部控制映射
         */
        void connectChannelSliders() {
            int channelCount = widget->getChannelCount();
            for (int i = 0; i < channelCount; ++i) {
                IntDragValueWidget* slider = widget->getChannelSlider(i);
                if (slider) {
                   
                    connect(
                        slider,
                        &IntDragValueWidget::valueChanged,
                        this,
                        [this, i](int value) {
                            setChannelValue(i, value);
                        }
                    );
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
                int newVal = qBound(0, value, 255);
                if (channelValues[channel] == newVal) return;

                channelValues[channel] = newVal;
                updateChannelData();
                AbstractDelegateModel::stateFeedBack(QString("/channels/%1").arg(channel), newVal);
            }
        }

        /**
         * @brief 更新通道数据输出
         */
        void updateChannelData() {
            channelData = std::make_shared<VariableData>();
            
            // 创建包含DMX地址映射的数据
            QVariantMap dmxData;
            int startAddr = m_startAddress;
            int channelCount = m_channelCount;
            bool enabled = m_enabled;
            
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
            int startAddr = m_startAddress;
            int channelCount = m_channelCount;
            widget->updateAddressRangeLabel(startAddr, channelCount);
        }

        /**
         * @brief 更新通道滑块显示
         */
        void updateChannelSliders() {
            int channelCount = widget->getChannelCount();
            
            for (int i = 0; i < channelCount; ++i) {
                IntDragValueWidget* slider = widget->getChannelSlider(i);
                if (slider) {
                    slider->setValue(channelValues[i]);
                }
            }
        }

    private:
        std::shared_ptr<VariableData> channelData;  // 通道数据输出
        QVector<int> channelValues;                 // 通道值数组
        
        Nodes::DMXDeviceInterface * widget = new Nodes::DMXDeviceInterface();
        int m_channelCount = 5;
        int m_startAddress = 1;
        bool m_enabled = true;
    };
}
