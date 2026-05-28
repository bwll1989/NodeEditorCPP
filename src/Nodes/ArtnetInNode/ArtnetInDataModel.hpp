#pragma once

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QtCore/qglobal.h>
#include "Common/Devices/ArtnetReceiver/ArtnetReceiver.h"
#include <QVariantMap>
#include "QThread"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include <algorithm>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class ArtnetInDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(int universe READ universe WRITE setUniverse NOTIFY universeChanged)
    Q_PROPERTY(QString channels READ channels WRITE setChannels NOTIFY channelsChanged)
    Q_PROPERTY(bool filterEnabled READ filterEnabled WRITE setFilterEnabled NOTIFY filterEnabledChanged)

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

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "universe";
            AbstractDelegateModel::registerExternalBinding("/universe", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "channels";
            AbstractDelegateModel::registerExternalBinding("/channels", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "filterEnabled";
            AbstractDelegateModel::registerExternalBinding("/filter", this, b);
        }

        connect(artnet_Receiver, &ArtnetReceiver::receiveArtnet, this, &ArtnetInDataModel::getArtnet);
        connect(this, &ArtnetInDataModel::universeChanged, artnet_Receiver, &ArtnetReceiver::universeFilter);
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
                setUniverse(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                break;
            case 1:
                setChannels(std::dynamic_pointer_cast<VariableData>(data)->value().toString());
                break;
            case 2:
                setFilterEnabled(std::dynamic_pointer_cast<VariableData>(data)->value().toBool());
                break;
            default:
                return;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["Universe"] = universe();
        modelJson1["Channels"] = channels();
        modelJson1["Filter"] = filterEnabled();
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = modelJson1;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            QJsonObject obj = v.toObject();
            setUniverse(obj["Universe"].toInt());
            setChannels(obj["Channels"].toString());
            setFilterEnabled(obj["Filter"].toBool());
        }
    }

    
public:
    /**
     * 函数级注释：获取当前 Universe 属性值
     */
    int universe() const { return m_universe; }

    /**
     * 函数级注释：获取当前通道过滤文本（例如：1,2,5-10）
     */
    QString channels() const { return m_channels; }

    /**
     * 函数级注释：获取过滤开关状态（true: 启用过滤）
     */
    bool filterEnabled() const { return m_filterEnabled; }

public Q_SLOTS:
    /**
     * 函数级注释：设置 Universe 属性，并同步到 UI
     */
    void setUniverse(int universe)
    {
        if (m_universe == universe) {
            return;
        }
        m_universe = universe;
        Q_EMIT universeChanged(m_universe);
    }

    /**
     * 函数级注释：设置通道过滤文本属性，并同步解析过滤列表
     */
    void setChannels(const QString& text)
    {
        const QString trimmed = text.trimmed();
        if (m_channels == trimmed) {
            return;
        }
        m_channels = trimmed;
        m_channelsFilter = parseChannelsFilterText(m_channels);
        Q_EMIT channelsChanged(m_channels);
    }

    /**
     * 函数级注释：设置过滤开关属性，并同步到 UI
     */
    void setFilterEnabled(bool enabled)
    {
        if (m_filterEnabled == enabled) {
            return;
        }
        m_filterEnabled = enabled;
        Q_EMIT filterEnabledChanged(m_filterEnabled);
    }

protected:
    /**
     * 函数级注释：模型就绪后订阅全局事件总线，实现外部命令控制
     */
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/universe"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/channels"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/filter"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

private Q_SLOTS:
    /**
     * 函数级注释：处理来自全局事件总线的命令，更新对应属性
     */
    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }

        const QString addrUniverse = makeFullOscAddress("/universe");
        const QString addrChannels = makeFullOscAddress("/channels");
        const QString addrFilter = makeFullOscAddress("/filter");

        if (ev.address == addrUniverse) {
            setUniverse(ev.payload.toInt());
        } else if (ev.address == addrChannels) {
            setChannels(ev.payload.toString());
        } else if (ev.address == addrFilter) {
            setFilterEnabled(ev.payload.toBool());
        }
    }

    /**
     * 函数级注释：处理接收到的 Artnet 数据，根据当前属性进行过滤并输出
     */
    void getArtnet(const QVariantMap &data) {
        if (m_filterEnabled) {
            if (data.contains("universe") && data["universe"].toInt() == m_universe) {
                if (!m_channelsFilter.isEmpty() && data.contains("default")) {
                    QByteArray dmxData = data["default"].toByteArray();
                    QVariantMap filteredData;
                    filteredData["universe"] = data["universe"];
                    filteredData["host"] = data["host"];
                    for (int channel : m_channelsFilter) {
                        if (channel >= 0 && channel < dmxData.size()) {
                            int value = static_cast<unsigned char>(dmxData.at(channel));
                            filteredData[QString::number(channel)] = value;
                        }
                    }
                    m_outData = std::make_shared<VariableData>(filteredData);
                    Q_EMIT dataUpdated(0);
                }
            }
        } else {
            m_outData = std::make_shared<VariableData>(data);
            Q_EMIT dataUpdated(0);
        }
    }

Q_SIGNALS:
    /**
     * 函数级注释：Universe 属性变化通知
     */
    void universeChanged(int universe);

    /**
     * 函数级注释：通道过滤文本变化通知
     */
    void channelsChanged(const QString& channels);

    /**
     * 函数级注释：过滤开关变化通知
     */
    void filterEnabledChanged(bool enabled);
    
private:
    /**
     * 函数级注释：解析通道过滤器文本为通道索引列表
     * - 支持格式："1,2,5-10,15"（通道号范围 0-512）
     */
    static QList<int> parseChannelsFilterText(const QString& input)
    {
        QList<int> channels;
        const QString text = input.trimmed();
        if (text.isEmpty()) {
            return channels;
        }

        const QStringList parts = text.split(',', Qt::SkipEmptyParts);
        for (const QString& part : parts) {
            const QString p = part.trimmed();
            if (p.contains('-')) {
                const QStringList range = p.split('-', Qt::SkipEmptyParts);
                if (range.size() == 2) {
                    bool okStart = false;
                    bool okEnd = false;
                    const int start = range[0].trimmed().toInt(&okStart);
                    const int end = range[1].trimmed().toInt(&okEnd);
                    if (okStart && okEnd && start <= end) {
                        for (int i = start; i <= end; ++i) {
                            if (i >= 0 && i <= 512 && !channels.contains(i)) {
                                channels.append(i);
                            }
                        }
                    }
                }
            } else {
                bool ok = false;
                const int channel = p.toInt(&ok);
                if (ok && channel >= 0 && channel <= 512 && !channels.contains(channel)) {
                    channels.append(channel);
                }
            }
        }

        std::sort(channels.begin(), channels.end());
        return channels;
    }

private:
    std::shared_ptr<VariableData> m_inData;
    std::shared_ptr<VariableData> m_outData;

    int m_universe = 0;
    QString m_channels;
    bool m_filterEnabled = false;
    QList<int> m_channelsFilter;

    ArtnetReceiver * artnet_Receiver = new ArtnetReceiver();
};
