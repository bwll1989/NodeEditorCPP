#pragma once

#include <QtCore/QObject>
#include <QJsonArray>
#include <QJsonObject>

#include <QtNodes/NodeDelegateModel>
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "NodeDataList.hpp"
#include "SnapshotInterface.hpp"
#include "StatusContainer/GlobalEventBus.hpp"
#include "GraphSnapshotBridge/GraphSnapshotBridge.hpp"

struct GlobalEvent;

using namespace NodeDataTypes;
using namespace QtNodes;

namespace Nodes
{
    struct SnapshotPreset {
        QString name;
        QJsonArray nodes;
    };

    /**
     * @brief 节点快照：捕获选中节点的 save() 状态，点击互斥预设按钮召回 load()
     */
    class SnapshotModel final : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int activeIndex READ activeIndex WRITE setActiveIndex NOTIFY activeIndexChanged)

    public:
        SnapshotModel();

        ~SnapshotModel() override = default;

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
        QString portCaption(PortType portType, PortIndex portIndex) const override;
        std::shared_ptr<NodeData> outData(PortIndex port) override;
        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex port) override;
        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override;
        void load(const QJsonObject &p) override;

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override;

        int activeIndex() const { return m_activeIndex; }

    public slots:
        void setActiveIndex(int index);
        void recallPreset(int index);
        void captureActivePreset();
        void addPreset();
        void removeActivePreset();
        void onGlobalEvent(const GlobalEvent &ev);

    signals:
        void activeIndexChanged(int index);

    protected:
        void afterModelReady() override;

    private:
        void ensureDefaultPreset();
        void syncUi();
        QString sceneTitle() const;
        QVector<NodeId> collectCaptureNodeIds() const;

        SnapshotInterface *widget = new SnapshotInterface();
        std::shared_ptr<VariableData> m_activeOutput;
        QVector<SnapshotPreset> m_presets;
        int m_activeIndex = 0;
        bool m_suppressRecall = false;
        bool m_recallActiveOnReady = false; ///< load 后是否自动召回活动预设
    };
}
