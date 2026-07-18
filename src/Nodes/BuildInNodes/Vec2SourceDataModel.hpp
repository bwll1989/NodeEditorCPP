#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <QtGui/QVector2D>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "Elements/VectorDragValueWidget/VectorDragValueWidget.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;

namespace Nodes
{
/**
 * @brief 二维向量编辑节点：input + X/Y + VectorDragValueWidget
 * - 输出：统一 VecData（长度 2）；端口类型 id 为 "vec"
 * - input：兼容 VecData / VariableData，按需截取前 2 分量（缺省 0）
 * - 反馈：{ "x","y","values" }
 */
class Vec2SourceDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

public:
    Vec2SourceDataModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Vec2 Source";
        WidgetEmbeddable = true;
        Resizable = false;

        widget->setComponents(VectorDragValueWidget::Vec2);
        widget->setMinimumWidth(120);
        widget->setSingleStep(0.01);
        widget->setDecimals(3);
        widget->setVector2D(m_value);

        connect(widget, &VectorDragValueWidget::valueChanged, this, [this](const QVariant &v) {
            applyVector2D(VecData::fromVariant(v, 2).toVector2D());
        });

        connect(this, &Vec2SourceDataModel::valueChanged, this, [this](const QVariant &) {
            widget->blockSignals(true);
            widget->setComponents(VectorDragValueWidget::Vec2);
            widget->setVector2D(m_value);
            widget->blockSignals(false);
        });

        NodeDelegateModel::ExternalBinding binding;
        binding.member = "value";
        binding.control = widget;
        AbstractDelegateModel::registerExternalBinding("/vec2", this, binding);
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            switch (portIndex) {
            case 0: return QStringLiteral("input");
            case 1: return QStringLiteral("X");
            case 2: return QStringLiteral("Y");
            default: break;
            }
        }
        if (portType == PortType::Out) {
            return QStringLiteral("Vec2");
        }
        return {};
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex)
        if (portType == PortType::Out) {
            return VecData().type();
        }
        // 输入口保持 VariableData，以复用图中「万能输入」连接规则；setInData 仍可收 VecData
        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        return std::make_shared<VecData>(m_value);
    }

    QVariant value() const
    {
        return VecData(m_value).toVariantMap();
    }

    void setValue(const QVariant &v)
    {
        applyVector2D(VecData::fromVariant(v, 2).toVector2D());
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (portIndex > 2) {
            return;
        }

        if (portIndex == 0) {
            if (!data) {
                applyVector2D(QVector2D());
                return;
            }
            if (auto vec = std::dynamic_pointer_cast<VecData>(data)) {
                applyVector2D(vec->toVector2D());
                return;
            }
            if (auto varData = std::dynamic_pointer_cast<VariableData>(data)) {
                applyVector2D(VecData::fromVariableData(varData, 2).toVector2D());
            }
            return;
        }

        float component = 0.0f;
        if (data) {
            if (auto varData = std::dynamic_pointer_cast<VariableData>(data)) {
                component = float(varData->value(QStringLiteral("default")).toDouble());
            } else if (auto vec = std::dynamic_pointer_cast<VecData>(data)) {
                component = (portIndex == 1) ? vec->x() : vec->y();
            }
        }

        QVector2D next = m_value;
        if (portIndex == 1) next.setX(component);
        else if (portIndex == 2) next.setY(component);
        applyVector2D(next);
    }

    QJsonObject save() const override
    {
        QJsonObject values = VecData(m_value).toJsonObject();
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        const QJsonValue v = p["values"];
        if (!v.isObject()) {
            return;
        }
        applyVector2D(VecData::fromVariant(v.toObject().toVariantMap(), 2).toVector2D());
    }

    QWidget *embeddedWidget() override
    {
        return widget;
    }

signals:
    void valueChanged(const QVariant &value);

protected:
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(
            makeFullOscAddress("/vec2"),
            this,
            SLOT(onGlobalEvent(GlobalEvent)));
    }

public Q_SLOTS:
    void onGlobalEvent(const GlobalEvent &ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address != makeFullOscAddress("/vec2")) {
            return;
        }
        applyVector2D(VecData::fromVariant(ev.payload, 2).toVector2D());
    }

private:
    static bool fuzzyEqual(const QVector2D &a, const QVector2D &b)
    {
        return qFuzzyCompare(a.x() + 1.0f, b.x() + 1.0f)
            && qFuzzyCompare(a.y() + 1.0f, b.y() + 1.0f);
    }

    void applyVector2D(const QVector2D &v)
    {
        if (fuzzyEqual(m_value, v)) {
            return;
        }
        m_value = v;
        Q_EMIT dataUpdated(0);
        const QVariant mapped = VecData(m_value).toVariantMap();
        Q_EMIT valueChanged(mapped);
    }

    VectorDragValueWidget *widget = new VectorDragValueWidget(VectorDragValueWidget::Vec2);
    QVector2D m_value {0.0f, 0.0f};
};
}
