#pragma once

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QSignalBlocker>
#include <QtCore/qglobal.h>

#include "HotKeyInterface.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * Keyboard In：一行对应一个输出端口，由 UI 同步增减。
     */
    class HotKeyDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        HotKeyDataModel()
        {
            InPortCount = 1;
            OutPortCount = 4;
            CaptionVisible = true;
            Caption = QStringLiteral("Keyboard In");
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = false;

            widget->setRowCount(static_cast<int>(OutPortCount));

            connect(widget, &HotKeyInterface::keyStateChanged,
                    this, &HotKeyDataModel::onKeyStateChanged);
            connect(widget, &HotKeyInterface::listChanged,
                    this, &HotKeyDataModel::onListChanged);
            connect(widget, &HotKeyInterface::rowAppended,
                    this, &HotKeyDataModel::onRowAppended);
            connect(widget, &HotKeyInterface::rowRemoved,
                    this, &HotKeyDataModel::onRowRemoved);
        }

        ~HotKeyDataModel() override = default;

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                Q_UNUSED(portIndex)
                return QStringLiteral("ENABLE");
            case PortType::Out:
                return QStringLiteral("K%1").arg(portIndex);
            default:
                return {};
            }
        }



        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            return std::make_shared<VariableData>(
                QVariant(widget->isPortPressed(static_cast<int>(port)) ? 1 : 0));
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (portIndex != 0) {
                return;
            }

            // 与左侧 On 同一状态：有输入则同步复选框；断开连接保持当前 UI 状态
            if (data) {
                bool enabled = false;
                if (auto varData = std::dynamic_pointer_cast<VariableData>(data)) {
                    enabled = varData->asBool(QStringLiteral("default"), false);
                }
                widget->setActive(enabled);
            }

            for (unsigned int i = 0; i < OutPortCount; ++i) {
                Q_EMIT dataUpdated(i);
            }
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson.insert(QStringLiteral("active"), widget->isActive());
            modelJson.insert(QStringLiteral("modifiers"), widget->modifierModeValue());
            modelJson.insert(QStringLiteral("keys"), widget->exportKeys());
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QSignalBlocker widgetBlocker(widget);

            if (p.contains(QStringLiteral("active"))) {
                widget->setActive(p.value(QStringLiteral("active")).toBool(true));
            }
            if (p.contains(QStringLiteral("modifiers"))) {
                widget->setModifierMode(p.value(QStringLiteral("modifiers")).toInt(0));
            }

            if (p.contains(QStringLiteral("keys")) && p.value(QStringLiteral("keys")).isArray()) {
                widget->importKeys(p.value(QStringLiteral("keys")).toArray());
            } else {
                // 兼容旧版固定 5 路 hotkey1…hotkey5
                QJsonArray legacy;
                for (int i = 1; i <= 5; ++i) {
                    const QString key = QStringLiteral("hotkey%1").arg(i);
                    if (!p.contains(key) || !p.value(key).isObject()) {
                        continue;
                    }
                    QJsonObject obj = p.value(key).toObject();
                    obj.insert(QStringLiteral("port"), i - 1);
                    legacy.append(obj);
                }
                if (!legacy.isEmpty()) {
                    widget->importKeys(legacy);
                }
            }

            OutPortCount = static_cast<unsigned int>(qMax(1, widget->rowCount()));
            widget->setRowCount(static_cast<int>(OutPortCount));
            Q_EMIT embeddedWidgetSizeUpdated();
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

    private slots:
        void onKeyStateChanged(int port)
        {
            if (port < 0 || static_cast<unsigned int>(port) >= OutPortCount) {
                return;
            }
            Q_EMIT dataUpdated(static_cast<PortIndex>(port));
        }

        void onListChanged()
        {
            for (unsigned int i = 0; i < OutPortCount; ++i) {
                Q_EMIT dataUpdated(i);
            }
            Q_EMIT embeddedWidgetSizeUpdated();
        }

        void onRowAppended()
        {
            const unsigned int oldCount = OutPortCount;
            Q_EMIT portsAboutToBeInserted(PortType::Out, oldCount, oldCount);
            OutPortCount = oldCount + 1;
            Q_EMIT portsInserted();
            Q_EMIT embeddedWidgetSizeUpdated();
        }

        void onRowRemoved(int index)
        {
            if (index < 0 || OutPortCount <= 1) {
                return;
            }
            const auto portIndex = static_cast<PortIndex>(index);
            Q_EMIT portsAboutToBeDeleted(PortType::Out, portIndex, portIndex);
            OutPortCount -= 1;
            Q_EMIT portsDeleted();
            Q_EMIT embeddedWidgetSizeUpdated();

            for (unsigned int i = 0; i < OutPortCount; ++i) {
                Q_EMIT dataUpdated(i);
            }
        }

    private:
        HotKeyInterface *widget = new HotKeyInterface();
    };
}
