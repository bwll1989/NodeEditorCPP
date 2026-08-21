#pragma once

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/qglobal.h>

#include "HotKeyInterface.hpp"
#include "HotKeyItem.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * Keyboard In：列表行与输出端口数量相互独立，均由用户自行配置（同 Delay）。
     */
    class HotKeyDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        HotKeyDataModel()
        {
            InPortCount = 0;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = QStringLiteral("Keyboard In");
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = true;

            connect(widget, &HotKeyInterface::keyStateChanged,
                    this, &HotKeyDataModel::onKeyStateChanged);
            connect(widget, &HotKeyInterface::listChanged,
                    this, &HotKeyDataModel::onListChanged);
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
            if (portType != PortType::Out) {
                return QString();
            }
            return QStringLiteral("K%1").arg(portIndex);
        }

        bool portCaptionVisible(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            return portType == PortType::Out;
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            return std::make_shared<VariableData>(
                QVariant(widget->isPortPressed(static_cast<int>(port)) ? 1 : 0));
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(data)
            Q_UNUSED(portIndex)
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson[QStringLiteral("active")] = widget->activeCheck->isChecked();
            modelJson[QStringLiteral("modifiers")] = widget->modifierCombo->currentData().toInt();
            modelJson[QStringLiteral("keys")] = widget->exportKeys();
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            if (p.contains(QStringLiteral("active"))) {
                widget->activeCheck->setChecked(p[QStringLiteral("active")].toBool(true));
            }
            if (p.contains(QStringLiteral("modifiers"))) {
                widget->setModifierMode(p[QStringLiteral("modifiers")].toInt(0));
            }

            if (p.contains(QStringLiteral("keys")) && p[QStringLiteral("keys")].isArray()) {
                widget->importKeys(p[QStringLiteral("keys")].toArray());
            } else {
                // 兼容旧版固定 5 路 hotkey1…hotkey5
                QJsonArray legacy;
                for (int i = 1; i <= 5; ++i) {
                    const QString key = QStringLiteral("hotkey%1").arg(i);
                    if (!p.contains(key) || !p[key].isObject()) {
                        continue;
                    }
                    QJsonObject obj = p[key].toObject();
                    obj[QStringLiteral("port")] = i - 1;
                    legacy.append(obj);
                }
                if (!legacy.isEmpty()) {
                    widget->importKeys(legacy);
                }
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

    private slots:
        void onKeyStateChanged(int port)
        {
            // 与 Delay 一致：端口是否存在由用户配置；越界则忽略
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

    private:
        HotKeyInterface *widget = new HotKeyInterface();
    };
}
