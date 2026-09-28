#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <array>
#include <memory>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>

#include "NodeDataList.hpp"
#include "AJTRelayInterface.hpp"
#include "AJTProtocol.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace QtNodes;

namespace Nodes {

/**
 * @brief AJT Relay Node：设备 ID + 12 路继电器，经 DATA 口输出给 AJT Gateway
 *
 * 输入：CH1～CH12 + 全开/全关；输出：DATA（kind=relay, id/enable/channels）
 */
class AJTRelayDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(int deviceId READ getDeviceId WRITE setDeviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(bool enable READ getEnable WRITE setEnable NOTIFY enableChanged)

public:
    static constexpr int kChannelCount = AJTProtocol::kRelayChannelCount;
    static constexpr int kEnablePort = kChannelCount;

    AJTRelayDataModel();
    ~AJTRelayDataModel() override;

    int getDeviceId() const { return _device.id; }
    void setDeviceId(int id);

    bool getEnable() const { return _device.enable; }
    void setEnable(bool enable);

    void afterModelReady() override;

    QJsonObject save() const override;
    void load(QJsonObject const &p) override;
    ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override;
    QString portCaption(PortType portType, PortIndex portIndex) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex port) override;
    QWidget *embeddedWidget() override { return _interface; }

signals:
    void deviceIdChanged(int id);
    void enableChanged(bool enable);

private slots:
    void onGlobalEvent(const GlobalEvent &ev);
    void flushPendingUi();

private:
    AJTRelayInterface *_interface = nullptr;
    QTimer *_uiDebounce = nullptr;
    AJTProtocol::DeviceState _device;
    std::shared_ptr<NodeDataTypes::VariableData> _deviceOut;
    quint16 _uiDirtyMask = 0;
    bool _loading = false;

    void setChannelState(int index, int on);
    void scheduleUiUpdate(int index);
    void publishDevice();
};

} // namespace Nodes
