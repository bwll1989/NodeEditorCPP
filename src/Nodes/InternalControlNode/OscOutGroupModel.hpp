#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "InternalControlInterface.hpp"
#include "DataTypes/NodeDataList.hpp"
#include "Common/AppConfig/ConfigManager.h"
#include "StatusContainer/GlobalEventBus.hpp"

struct GlobalEvent;
using namespace NodeDataTypes;
using namespace Nodes;
using namespace QtNodes;
namespace Nodes
{
    class OscOutGroupModel final : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(bool trigger READ trigger WRITE setTrigger NOTIFY triggerChanged)

    public:
        OscOutGroupModel();

        ~OscOutGroupModel() override = default;

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override;

        void load(const QJsonObject &p) override;

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override;

        bool trigger() const { return m_trigger; }

    public Q_SLOTS:
        void setTrigger(bool value);
        void onGlobalEvent(const GlobalEvent& ev);

    Q_SIGNALS:
        void triggerChanged(bool value);

    protected:
        void afterModelReady() override;

    private:
        InternalControlInterface *widget = new InternalControlInterface(false);
        bool m_trigger = false;
    };
}
