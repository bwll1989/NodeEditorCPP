#pragma once

#include <QtCore/QObject>
#include <QtCore/QSignalBlocker>

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "MergeInterface.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

#include <unordered_map>

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;

namespace Nodes
{
    class MergeDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        MergeDataModel()
        {
            InPortCount = 4;
            OutPortCount = 1;
            Caption = "Merge";
            CaptionVisible = true;
            WidgetEmbeddable = false;
            PortEditable = false;
            Resizable = true;

            m_propertyData = std::make_shared<VariableData>("");
            widget->setRowCount(static_cast<int>(InPortCount));

            connect(widget, &MergeInterface::tableChanged, this, &MergeDataModel::processAll);
            connect(widget, &MergeInterface::rowChanged, this, &MergeDataModel::processRow);
            connect(widget, &MergeInterface::rowAppended, this, &MergeDataModel::onRowAppended);
            connect(widget, &MergeInterface::rowRemoved, this, &MergeDataModel::onRowRemoved);
        }

        ~MergeDataModel() override = default;

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return m_propertyData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                in_dictionary.erase(portIndex);
                processAll();
                return;
            }

            auto varData = std::dynamic_pointer_cast<VariableData>(data);
            if (!varData) {
                return;
            }
            in_dictionary[portIndex] = varData;
            processAll();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return QStringLiteral("INPUT %1").arg(portIndex);
            case PortType::Out:
                return QStringLiteral("OUTPUT %1").arg(portIndex);
            default:
                return {};
            }
        }

        void processAll()
        {
            m_propertyData = std::make_shared<VariableData>("");
            for (int i = 0; i < widget->rowCount(); ++i) {
                mergeRow(static_cast<PortIndex>(i));
            }
            outDataSlot();
        }

        void processRow(int row)
        {
            if (row < 0 || row >= widget->rowCount()) {
                return;
            }
            // 单行变更仍整表重算，避免 Rename 改键后残留旧键
            processAll();
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1.insert(QStringLiteral("key"), widget->exportToJson());
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson.insert(QStringLiteral("values"), modelJson1);
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QSignalBlocker widgetBlocker(widget);
            const QJsonValue v = p.value(QStringLiteral("values"));
            if (!v.isUndefined() && v.isObject()) {
                widget->importFromJson(v.toObject().value(QStringLiteral("key")).toObject());
            }
            InPortCount = static_cast<unsigned int>(qMax(1, widget->rowCount()));
            widget->setRowCount(static_cast<int>(InPortCount));
            Q_EMIT embeddedWidgetSizeUpdated();
            processAll();
        }

        QWidget *embeddedWidget() override { return widget; }

    private slots:
        void outDataSlot()
        {
            for (unsigned int i = 0; i < OutPortCount; ++i) {
                Q_EMIT dataUpdated(i);
            }
        }

        void onRowAppended()
        {
            const unsigned int oldCount = InPortCount;
            Q_EMIT portsAboutToBeInserted(PortType::In, oldCount, oldCount);
            InPortCount = oldCount + 1;
            Q_EMIT portsInserted();
            Q_EMIT embeddedWidgetSizeUpdated();
        }

        void onRowRemoved(int index)
        {
            if (index < 0 || InPortCount <= 1) {
                return;
            }
            const auto portIndex = static_cast<PortIndex>(index);
            Q_EMIT portsAboutToBeDeleted(PortType::In, portIndex, portIndex);
            InPortCount -= 1;
            Q_EMIT portsDeleted();
            Q_EMIT embeddedWidgetSizeUpdated();

            // 输入缓存按端口索引上移，与连线补位一致
            std::unordered_map<unsigned int, std::shared_ptr<VariableData>> shifted;
            for (const auto &kv : in_dictionary) {
                if (kv.first == portIndex) {
                    continue;
                }
                const unsigned int newKey = kv.first > portIndex ? kv.first - 1 : kv.first;
                shifted[newKey] = kv.second;
            }
            in_dictionary = std::move(shifted);
            processAll();
        }

    private:
        void mergeRow(PortIndex portIndex)
        {
            const auto it = in_dictionary.find(portIndex);
            if (it == in_dictionary.end() || !it->second) {
                return;
            }

            const QString name = widget->nameAt(static_cast<int>(portIndex));
            if (name.isEmpty()) {
                return;
            }

            const QString rename = widget->renameAt(static_cast<int>(portIndex));
            const QString outKey = rename.isEmpty() ? name : rename;
            m_propertyData->insert(outKey, it->second->value(name));
        }

        MergeInterface *widget = new MergeInterface();
        std::shared_ptr<VariableData> m_propertyData;
        std::unordered_map<unsigned int, std::shared_ptr<VariableData>> in_dictionary;
    };
}
