#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <QtGui/QVector4D>

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
 * @brief 四维向量编辑节点：input + X/Y/Z/W + VectorDragValueWidget
 * - 输出：统一 VecData（长度 4）；端口类型 id 为 "vec"
 * - input：兼容 VecData / VariableData，按需截取前 4 分量（缺省 0）
 * - 反馈：{ "x","y","z","w","values" }
 */
class Vec4SourceDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

public:
    Vec4SourceDataModel()
    {
        InPortCount = 5;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Vec4 Source";
        WidgetEmbeddable = true;
        Resizable = false;

        widget->setComponents(VectorDragValueWidget::Vec4);
        widget->setMinimumWidth(120);
        widget->setSingleStep(0.01);
        widget->setDecimals(3);
        widget->setVector4D(m_value);

        connect(widget, &VectorDragValueWidget::valueChanged, this, [this](const QVariant &v) {
            applyVector4D(VecData::fromVariant(v, 4).toVector4D());
        });

        connect(this, &Vec4SourceDataModel::valueChanged, this, [this](const QVariant &) {
            widget->blockSignals(true);
            widget->setComponents(VectorDragValueWidget::Vec4);
            widget->setVector4D(m_value);
            widget->blockSignals(false);
        });

        NodeDelegateModel::ExternalBinding binding;
        binding.member = "value";
        binding.control = widget;
        AbstractDelegateModel::registerExternalBinding("/vec4", this, binding);
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            switch (portIndex) {
            case 0: return QStringLiteral("input");
            case 1: return QStringLiteral("X");
            case 2: return QStringLiteral("Y");
            case 3: return QStringLiteral("Z");
            case 4: return QStringLiteral("W");
            default: break;
            }
        }
        if (portType == PortType::Out) {
            return QStringLiteral("Vec4");
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
        applyVector4D(VecData::fromVariant(v, 4).toVector4D());
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (portIndex > 4) {
            return;
        }

        if (portIndex == 0) {
            if (!data) {
                applyVector4D(QVector4D());
                return;
            }
            if (auto vec = std::dynamic_pointer_cast<VecData>(data)) {
                applyVector4D(vec->toVector4D());
                return;
            }
            if (auto varData = std::dynamic_pointer_cast<VariableData>(data)) {
                applyVector4D(VecData::fromVariableData(varData, 4).toVector4D());
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
                case 4: component = vec->w(); break;
                default: break;
                }
            }
        }

        QVector4D next = m_value;
        switch (portIndex) {
        case 1: next.setX(component); break;
        case 2: next.setY(component); break;
        case 3: next.setZ(component); break;
        case 4: next.setW(component); break;
        default: return;
        }
        applyVector4D(next);
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
        applyVector4D(VecData::fromVariant(v.toObject().toVariantMap(), 4).toVector4D());
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
            makeFullOscAddress("/vec4"),
            this,
            SLOT(onGlobalEvent(GlobalEvent)));
    }

public Q_SLOTS:
    void onGlobalEvent(const GlobalEvent &ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address != makeFullOscAddress("/vec4")) {
            return;
        }
        applyVector4D(VecData::fromVariant(ev.payload, 4).toVector4D());
    }

private:
    static bool fuzzyEqual(const QVector4D &a, const QVector4D &b)
    {
        return qFuzzyCompare(a.x() + 1.0f, b.x() + 1.0f)
            && qFuzzyCompare(a.y() + 1.0f, b.y() + 1.0f)
            && qFuzzyCompare(a.z() + 1.0f, b.z() + 1.0f)
            && qFuzzyCompare(a.w() + 1.0f, b.w() + 1.0f);
    }

    void applyVector4D(const QVector4D &v)
    {
        if (fuzzyEqual(m_value, v)) {
            return;
        }
        m_value = v;
        Q_EMIT dataUpdated(0);
        const QVariant mapped = VecData(m_value).toVariantMap();
        Q_EMIT valueChanged(mapped);
    }

    VectorDragValueWidget *widget = new VectorDragValueWidget(VectorDragValueWidget::Vec4);
    QVector4D m_value {0.0f, 0.0f, 0.0f, 0.0f};
};
}
