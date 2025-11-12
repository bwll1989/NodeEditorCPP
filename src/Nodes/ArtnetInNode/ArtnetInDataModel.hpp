#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "Common/Devices/ArtnetReceiver/ArtnetReceiver.h"
#include "ArtnetInInterface.hpp"
#include <QVariantMap>
#include "QThread"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class ArtnetInDataModel : public NodeDelegateModel
{

public:
    ArtnetInDataModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Artnet Source";
        WidgetEmbeddable = false;
        Resizable = false;
        m_inData = std::make_shared<VariableData>();
        m_outData = std::make_shared<VariableData>();
        connect(artnet_Receiver, &ArtnetReceiver::receiveArtnet, this, &ArtnetInDataModel::getArtnet);
        connect(widget, &ArtnetInInterface::UniverseChanged, artnet_Receiver, &ArtnetReceiver::universeFilter);
        connect(widget, &ArtnetInInterface::ChannelsFilterChanged, this, &ArtnetInDataModel::setChannelsFilter);
    }

    ~ArtnetInDataModel() override {
//        artnet_Receiver->deleteLater();
        // delete artnet_Receiver;

    }

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        switch(portType)
        {
        case PortType::In:
            switch(portIndex)
            {
                case 0:
                    return "UNIVERSE";
                case 1:
                    return "CHANNELS";
                case 2:
                    return "FILTER";
                default:
                    return "";
            }
        case PortType::Out:
            switch(portIndex)
            {
            case 0:
                return "OUTPUT";
            default:
                return "";
            }
        default:
            return "";
        }

    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex)
        Q_UNUSED(portType)
        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return m_outData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex);
        if (data == nullptr){
            return;
        }
        switch(portIndex)
        {
            case 0:
                widget->universeEdit->setText(std::dynamic_pointer_cast<VariableData>(data)->value().toString());
                break;
            case 1:
               widget->channelsEdit->setText(std::dynamic_pointer_cast<VariableData>(data)->value().toString());
                break;
            case 2:
                widget->setChecked(std::dynamic_pointer_cast<VariableData>(data)->value().toBool());
                break;
            default:
                return;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["Universe"] = widget->universeEdit->text().toInt();
        modelJson1["Channels"] = widget->channelsEdit->text();
        modelJson1["Filter"] = widget->isChecked();
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = modelJson1;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            QJsonObject obj = v.toObject();
            widget->universeEdit->setText(QString::number(obj["Universe"].toInt()));
            widget->channelsEdit->setText(obj["Channels"].toString());
            widget->setChecked(obj["Filter"].toBool());

        }
    }
    
    QWidget *embeddedWidget() override {
        return widget;
    }
    
private Q_SLOTS:
    /**
     * 设置通道过滤器
     * @param channels 需要过滤的通道列表
     */
    void setChannelsFilter(const QList<int> &channels) {
        m_channelsFilter = channels;
    }
    
    /**
     * 处理接收到的Artnet数据
     * @param data Artnet数据包
     */
    void getArtnet(const QVariantMap &data) {
        // 如果GroupBox被选中（启用过滤）
        if (widget->isChecked()) {
            // 检查Universe是否匹配
            if (data.contains("universe") &&
                data["universe"].toInt() == widget->universeEdit->text().toInt()) {
                // 如果有通道过滤器
                if (!m_channelsFilter.isEmpty() && data.contains("default")) {
                    QByteArray dmxData = data["default"].toByteArray();
                    QVariantMap filteredData;
                    // 复制原始数据中的基本信息
                    filteredData["universe"] = data["universe"];
                    filteredData["host"] = data["host"];
                    // 提取过滤后的通道值并转换为十进制
                    for (int channel : m_channelsFilter) {
                        if (channel >= 0 && channel < dmxData.size()) {
                            // 通道编号从0开始，数组索引也从0开始
                            int value = static_cast<unsigned char>(dmxData.at(channel));
                            filteredData[QString::number(channel)] = value;
                        }
                    }
                    // 更新输出数据
                    m_outData = std::make_shared<VariableData>(filteredData);
                    Q_EMIT dataUpdated(0);
                }
            }
        } else {
            // 不过滤，直接传递所有数据
            m_outData = std::make_shared<VariableData>(data);
            Q_EMIT dataUpdated(0);
        }
    }
    
private:
    std::shared_ptr<VariableData> m_inData;
    std::shared_ptr<VariableData> m_outData;
    QList<int> m_channelsFilter; // 存储通道过滤器列表

    ArtnetReceiver * artnet_Receiver = new ArtnetReceiver();
    ArtnetInInterface * widget = new ArtnetInInterface();
};
