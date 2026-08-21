#pragma once

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QSignalBlocker>
#include <QtCore/QVariant>
#include <QtCore/QVector>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "VecSourceInterface.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;

namespace Nodes
{
/**
 * @brief 可变长度向量源：输出 VariableData，default 为 float 列表。
 * 维数由界面 Size 决定；输入端口可编辑，默认 5 路。
 * 前 n-1 口写对应下标分量；最后一口为 Vec，按当前 Size 截取或补 0。
 */
class VecSourceDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

public:
    VecSourceDataModel()
    {
        InPortCount = 5;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = QStringLiteral("Vector Source");
        WidgetEmbeddable = false;
        Resizable = true;
        PortEditable = true;

        m_values = widget->values();

        connect(widget, &VecSourceInterface::valuesChanged,
                this, [this](const QVector<float> &v) { applyValues(v, false); });
        connect(widget, &VecSourceInterface::sizeChanged,
                this, &VecSourceDataModel::onSizeChanged);

        connect(this, &VecSourceDataModel::valueChanged, this, [this](const QVariant &) {
            const QSignalBlocker b(widget);
            widget->setValues(m_values);
        });

        NodeDelegateModel::ExternalBinding binding;
        binding.member = QStringLiteral("value");
        binding.control = widget;
        AbstractDelegateModel::registerExternalBinding(QStringLiteral("/vec"), this, binding);
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            if (isVecInPort(portIndex)) {
                return QStringLiteral("Vec");
            }
            return QString::number(portIndex);
        }
        if (portType == PortType::Out) {
            return QStringLiteral("Vec");
        }
        return {};
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portType)
        Q_UNUSED(portIndex)
        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        return std::make_shared<VariableData>(value());
    }

    QVariant value() const
    {
        return floatVectorToList(m_values);
    }

    void setValue(const QVariant &v)
    {
        applyValues(VariableData(v).asFloats(), true);
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (!data) {
            return;
        }
        auto varData = std::dynamic_pointer_cast<VariableData>(data);
        if (!varData) {
            return;
        }

        if (isVecInPort(portIndex)) {
            // 整段向量：按当前 Size 截取/补 0
            applyValues(varData->asFloats(m_values.size()), true);
            return;
        }

        if (int(portIndex) >= m_values.size()) {
            return;
        }
        // 分量口：标量或列表首元
        QVector<float> next = m_values;
        next[int(portIndex)] = float(varData->asNumber());
        applyValues(next, true);
    }

    QJsonObject save() const override
    {
        QJsonArray arr;
        for (float v : m_values) {
            arr.append(v);
        }
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson[QStringLiteral("values")] = arr;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        const QJsonValue v = p[QStringLiteral("values")];
        QVector<float> loaded;
        if (v.isArray()) {
            loaded = VariableData(v.toArray().toVariantList()).asFloats();
        } else if (v.isObject()) {
            loaded = VariableData(v.toObject().toVariantMap()).asFloats();
        } else {
            return;
        }
        if (loaded.isEmpty()) {
            return;
        }
        // 存盘恢复维数；日常 applyValues 不改控件数量
        {
            const QSignalBlocker b(widget);
            widget->setValues(loaded);
        }
        m_values = widget->values();
        Q_EMIT dataUpdated(0);
        Q_EMIT valueChanged(value());
        Q_EMIT embeddedWidgetSizeUpdated();
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
            makeFullOscAddress(QStringLiteral("/vec")),
            this,
            SLOT(onGlobalEvent(GlobalEvent)));
    }

public Q_SLOTS:
    void onGlobalEvent(const GlobalEvent &ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address != makeFullOscAddress(QStringLiteral("/vec"))) {
            return;
        }
        applyValues(VariableData(ev.payload).asFloats(), true);
    }

private:
    bool isVecInPort(PortIndex portIndex) const
    {
        return InPortCount > 0 && portIndex == InPortCount - 1;
    }

    void onSizeChanged(int)
    {
        // Size 控件改维数：以界面为准
        m_values = widget->values();
        Q_EMIT dataUpdated(0);
        Q_EMIT valueChanged(value());
        Q_EMIT embeddedWidgetSizeUpdated();
    }

    /** 只按当前维数补 0 / 截取，不改控件数量 */
    void applyValues(QVector<float> next, bool syncWidget)
    {
        const int size = qMax(1, m_values.size());
        next.resize(size);

        if (next == m_values) {
            return;
        }

        m_values = std::move(next);
        if (syncWidget) {
            const QSignalBlocker b(widget);
            widget->setValues(m_values);
        }
        Q_EMIT dataUpdated(0);
        Q_EMIT valueChanged(value());
    }

    VecSourceInterface *widget = new VecSourceInterface();
    QVector<float> m_values;
};
}
