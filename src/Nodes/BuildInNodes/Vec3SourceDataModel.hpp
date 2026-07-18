#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <QtGui/QVector3D>

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
 * @brief 三维向量编辑节点：input + X/Y/Z + VectorDragValueWidget
 * - 输出：统一 VecData（长度 3）；端口类型 id 为 "vec"
 * - input：兼容 VecData / VariableData，按需截取前 3 分量（缺省 0）
 * - 反馈：{ "x","y","z","values" }
 */
class Vec3SourceDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

public:
    Vec3SourceDataModel()
    {
        InPortCount = 4;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Vec3 Source";
        WidgetEmbeddable = true;
        Resizable = false;

        widget->setComponents(VectorDragValueWidget::Vec3);
        widget->setMinimumWidth(120);
        widget->setSingleStep(0.01);
        widget->setDecimals(3);
        widget->setVector3D(m_value);

        connect(widget, &VectorDragValueWidget::valueChanged, this, [this](const QVariant &v) {
            applyVector3D(VecData::fromVariant(v, 3).toVector3D());
        });

        connect(this, &Vec3SourceDataModel::valueChanged, this, [this](const QVariant &) {
            widget->blockSignals(true);
            widget->setComponents(VectorDragValueWidget::Vec3);
            widget->setVector3D(m_value);
            widget->blockSignals(false);
        });

        NodeDelegateModel::ExternalBinding binding;
        binding.member = "value";
        binding.control = widget;
        AbstractDelegateModel::registerExternalBinding("/vec3", this, binding);
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            switch (portIndex) {
            case 0: return QStringLiteral("input");
            case 1: return QStringLiteral("X");
            case 2: return QStringLiteral("Y");
            case 3: return QStringLiteral("Z");
            default: break;
            }
        }
        if (portType == PortType::Out) {
            return QStringLiteral("Vec3");
        }
        return {};
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex)
        if (portType == PortType::Out) {
            return VecData().type();
        }
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
        applyVector3D(VecData::fromVariant(v, 3).toVector3D());
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (portIndex > 3) {
            return;
        }

        if (portIndex == 0) {
            if (!data) {
                applyVector3D(QVector3D());
                return;
            }
            if (auto vec = std::dynamic_pointer_cast<VecData>(data)) {
                applyVector3D(vec->toVector3D());
                return;
            }
            if (auto varData = std::dynamic_pointer_cast<VariableData>(data)) {
                applyVector3D(VecData::fromVariableData(varData, 3).toVector3D());
            }
            return;
        }

        float component = 0.0f;
        if (data) {
            if (auto varData = std::dynamic_pointer_cast<VariableData>(data)) {
                component = float(varData->value(QStringLiteral("default")).toDouble());
            } else if (auto vec = std::dynamic_pointer_cast<VecData>(data)) {
                switch (portIndex) {
                case 1: component = vec->x(); break;
                case 2: component = vec->y(); break;
                case 3: component = vec->z(); break;
                default: break;
                }
            }
        }

        QVector3D next = m_value;
        switch (portIndex) {
        case 1: next.setX(component); break;
        case 2: next.setY(component); break;
        case 3: next.setZ(component); break;
        default: return;
        }
        applyVector3D(next);
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
        applyVector3D(VecData::fromVariant(v.toObject().toVariantMap(), 3).toVector3D());
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
            makeFullOscAddress("/vec3"),
            this,
            SLOT(onGlobalEvent(GlobalEvent)));
    }

public Q_SLOTS:
    void onGlobalEvent(const GlobalEvent &ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address != makeFullOscAddress("/vec3")) {
            return;
        }
        applyVector3D(VecData::fromVariant(ev.payload, 3).toVector3D());
    }

private:
    static bool fuzzyEqual(const QVector3D &a, const QVector3D &b)
    {
        return qFuzzyCompare(a.x() + 1.0f, b.x() + 1.0f)
            && qFuzzyCompare(a.y() + 1.0f, b.y() + 1.0f)
            && qFuzzyCompare(a.z() + 1.0f, b.z() + 1.0f);
    }

    void applyVector3D(const QVector3D &v)
    {
        if (fuzzyEqual(m_value, v)) {
            return;
        }
        m_value = v;
        Q_EMIT dataUpdated(0);
        const QVariant mapped = VecData(m_value).toVariantMap();
        Q_EMIT valueChanged(mapped);
    }

    VectorDragValueWidget *widget = new VectorDragValueWidget(VectorDragValueWidget::Vec3);
    QVector3D m_value {0.0f, 0.0f, 0.0f};
};
}
