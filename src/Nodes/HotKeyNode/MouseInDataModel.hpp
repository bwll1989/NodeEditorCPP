#pragma once

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <QtCore/qglobal.h>

#include "MouseInInterface.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * Mouse In：输出鼠标位置与左/右键。
     */
    class MouseInDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        MouseInDataModel()
        {
            InPortCount = 0;
            OutPortCount = MouseInInterface::PortCount;
            CaptionVisible = true;
            Caption = QStringLiteral("Mouse In");
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;

            connect(widget, &MouseInInterface::mouseUpdated,
                    this, &MouseInDataModel::onMouseUpdated);
        }

        ~MouseInDataModel() override = default;

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portType)
            Q_UNUSED(portIndex)
            return VariableData().type();
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType != PortType::Out) {
                return QString();
            }
            switch (static_cast<int>(portIndex)) {
            case MouseInInterface::Tx: return QStringLiteral("tx");
            case MouseInInterface::Ty: return QStringLiteral("ty");
            case MouseInInterface::Left: return QStringLiteral("left");
            case MouseInInterface::Right: return QStringLiteral("right");
            case MouseInInterface::Pos: return QStringLiteral("pos");
            default: return QString();
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            if (port == MouseInInterface::Pos) {
                return std::make_shared<VariableData>(QVariant(widget->posList()));
            }
            return std::make_shared<VariableData>(
                QVariant(widget->valueAt(static_cast<int>(port))));
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
            modelJson[QStringLiteral("coord")] = widget->coordCombo->currentData().toInt();
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            if (p.contains(QStringLiteral("active"))) {
                widget->activeCheck->setChecked(p[QStringLiteral("active")].toBool(true));
            }
            if (p.contains(QStringLiteral("coord"))) {
                widget->setCoordMode(p[QStringLiteral("coord")].toInt(0));
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

    private slots:
        void onMouseUpdated(int port)
        {
            if (port < 0 || static_cast<unsigned int>(port) >= OutPortCount) {
                return;
            }
            Q_EMIT dataUpdated(static_cast<PortIndex>(port));
        }

    private:
        MouseInInterface *widget = new MouseInInterface();
    };
}
