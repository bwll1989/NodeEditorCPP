#pragma once

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QSignalBlocker>
#include <QtMath>

#include <Eigen/Core>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "RangeMapInterface.hpp"
#include "JSEngineDefines/JSEngineDefines.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;

namespace Nodes
{
    class RangeMapDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        Q_PROPERTY(double inMin READ inMin WRITE setInMin NOTIFY inMinChanged)
        Q_PROPERTY(double inMax READ inMax WRITE setInMax NOTIFY inMaxChanged)
        Q_PROPERTY(double outMin READ outMin WRITE setOutMin NOTIFY outMinChanged)
        Q_PROPERTY(double outMax READ outMax WRITE setOutMax NOTIFY outMaxChanged)
        Q_PROPERTY(bool clamp READ clamp WRITE setClamp NOTIFY clampChanged)

    public:
        RangeMapDataModel()
        {
            InPortCount = 1;
            OutPortCount = 1;
            Caption = QStringLiteral("Range Map");
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;

            m_inMin = 0.0;
            m_inMax = 1.0;
            m_outMin = 0.0;
            m_outMax = 255.0;
            m_clamp = true;

            syncWidgetFromProperties();

            connect(widget->inMin, &FloatDragValueWidget::valueChanged, this, &RangeMapDataModel::setInMin);
            connect(widget->inMax, &FloatDragValueWidget::valueChanged, this, &RangeMapDataModel::setInMax);
            connect(widget->outMin, &FloatDragValueWidget::valueChanged, this, &RangeMapDataModel::setOutMin);
            connect(widget->outMax, &FloatDragValueWidget::valueChanged, this, &RangeMapDataModel::setOutMax);
            connect(widget->expression, &QLineEdit::editingFinished, this, &RangeMapDataModel::onParametersChanged);
            connect(widget->clampCheck, &QCheckBox::toggled, this, &RangeMapDataModel::setClamp);

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "inMin";
                b.control = widget->inMin;
                AbstractDelegateModel::registerExternalBinding("/inMin", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "inMax";
                b.control = widget->inMax;
                AbstractDelegateModel::registerExternalBinding("/inMax", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "outMin";
                b.control = widget->outMin;
                AbstractDelegateModel::registerExternalBinding("/outMin", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "outMax";
                b.control = widget->outMax;
                AbstractDelegateModel::registerExternalBinding("/outMax", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "clamp";
                b.control = widget->clampCheck;
                AbstractDelegateModel::registerExternalBinding("/clamp", this, b);
            }

            m_jsEngine = new QJSEngine(this);
        }

        ~RangeMapDataModel() override = default;

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return computeOutput();
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            if (data == nullptr) {
                return;
            }
            m_inData = std::dynamic_pointer_cast<VariableData>(data);
            Q_EMIT dataUpdated(0);
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values["inMin"] = m_inMin;
            values["inMax"] = m_inMax;
            values["outMin"] = m_outMin;
            values["outMax"] = m_outMax;
            values["clamp"] = m_clamp;
            values["expression"] = widget->expression->text();

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                setInMin(v["inMin"].toDouble(0.0));
                setInMax(v["inMax"].toDouble(1.0));
                setOutMin(v["outMin"].toDouble(0.0));
                setOutMax(v["outMax"].toDouble(255.0));
                setClamp(v["clamp"].toBool(true));
                widget->expression->setText(v["expression"].toString());
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        double inMin() const { return m_inMin; }
        double inMax() const { return m_inMax; }
        double outMin() const { return m_outMin; }
        double outMax() const { return m_outMax; }
        bool clamp() const { return m_clamp; }

    public slots:
        void setInMin(double v)
        {
            if (qFuzzyCompare(m_inMin, v)) {
                return;
            }
            m_inMin = v;
            {
                const QSignalBlocker blocker(widget->inMin);
                widget->inMin->setValue(v);
            }
            Q_EMIT inMinChanged(v);
            onParametersChanged();
        }

        void setInMax(double v)
        {
            if (qFuzzyCompare(m_inMax, v)) {
                return;
            }
            m_inMax = v;
            {
                const QSignalBlocker blocker(widget->inMax);
                widget->inMax->setValue(v);
            }
            Q_EMIT inMaxChanged(v);
            onParametersChanged();
        }

        void setOutMin(double v)
        {
            if (qFuzzyCompare(m_outMin, v)) {
                return;
            }
            m_outMin = v;
            {
                const QSignalBlocker blocker(widget->outMin);
                widget->outMin->setValue(v);
            }
            Q_EMIT outMinChanged(v);
            onParametersChanged();
        }

        void setOutMax(double v)
        {
            if (qFuzzyCompare(m_outMax, v)) {
                return;
            }
            m_outMax = v;
            {
                const QSignalBlocker blocker(widget->outMax);
                widget->outMax->setValue(v);
            }
            Q_EMIT outMaxChanged(v);
            onParametersChanged();
        }

        void setClamp(bool v)
        {
            if (m_clamp == v) {
                return;
            }
            m_clamp = v;
            {
                const QSignalBlocker blocker(widget->clampCheck);
                widget->clampCheck->setChecked(v);
            }
            Q_EMIT clampChanged(v);
            onParametersChanged();
        }

    signals:
        void inMinChanged(double v);
        void inMaxChanged(double v);
        void outMinChanged(double v);
        void outMaxChanged(double v);
        void clampChanged(bool v);

    protected:
        void afterModelReady() override
        {
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/inMin"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/inMax"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/outMin"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/outMax"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/clamp"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private slots:
        void onParametersChanged()
        {
            Q_EMIT dataUpdated(0);
        }

        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addr = ev.address;
            if (addr == makeFullOscAddress("/inMin")) {
                setInMin(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/inMax")) {
                setInMax(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/outMin")) {
                setOutMin(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/outMax")) {
                setOutMax(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/clamp")) {
                setClamp(ev.payload.toBool());
            }
        }

    private:
        void syncWidgetFromProperties()
        {
            widget->inMin->setValue(m_inMin);
            widget->inMax->setValue(m_inMax);
            widget->outMin->setValue(m_outMin);
            widget->outMax->setValue(m_outMax);
            widget->clampCheck->setChecked(m_clamp);
        }

        double extractInputValue() const
        {
            if (!m_inData) {
                return 0.0;
            }

            const QString expression = widget->expression->text().trimmed();
            if (expression.isEmpty()) {
                return m_inData->asNumber();
            }

            const QVariantMap dataMap = m_inData->asMap();
            QJSValue jsData = JSEngineDefines::variantMapToJSValue(m_jsEngine, dataMap);
            QJSValue global = m_jsEngine->globalObject();
            global.setProperty("$input", jsData);

            for (auto it = dataMap.begin(); it != dataMap.end(); ++it) {
                global.setProperty(it.key(), m_jsEngine->toScriptValue(it.value()));
            }

            QJSValue result = m_jsEngine->evaluate(expression);
            if (result.isError()) {
                qDebug() << "RangeMap JS表达式错误" << result.toString();
                return 0.0;
            }

            return result.toVariant().toDouble();
        }

        /** 表达式为空且 default 为列表时，逐元素映射；否则按标量映射 */
        std::shared_ptr<VariableData> computeOutput() const
        {
            if (!m_inData) {
                return std::make_shared<VariableData>(0.0);
            }

            const QString expression = widget->expression->text().trimmed();
            if (expression.isEmpty() && m_inData->isList()) {
                return std::make_shared<VariableData>(
                    floatVectorToList(mapVector(m_inData->asFloats())));
            }

            return std::make_shared<VariableData>(mapValue(extractInputValue()));
        }

        double mapValue(double value) const
        {
            if (qFuzzyCompare(m_inMin, m_inMax)) {
                return m_outMin;
            }

            double t = (value - m_inMin) / (m_inMax - m_inMin);
            if (m_clamp) {
                t = qBound(0.0, t, 1.0);
            }
            return m_outMin + t * (m_outMax - m_outMin);
        }

        /** Eigen 向量化：out = outMin + clamp01((x-inMin)/(inMax-inMin)) * (outMax-outMin) */
        QVector<float> mapVector(const QVector<float> &input) const
        {
            const int n = input.size();
            if (n <= 0) {
                return {};
            }
            if (qFuzzyCompare(m_inMin, m_inMax)) {
                return QVector<float>(n, float(m_outMin));
            }

            QVector<float> output(n);
            const Eigen::Map<const Eigen::ArrayXf> in(input.constData(), n);
            Eigen::Map<Eigen::ArrayXf> out(output.data(), n);

            const float inMin = float(m_inMin);
            const float inMax = float(m_inMax);
            const float outMin = float(m_outMin);
            const float outMax = float(m_outMax);
            const float inv = 1.0f / (inMax - inMin);

            out = (in - inMin) * inv;
            if (m_clamp) {
                out = out.max(0.0f).min(1.0f);
            }
            out = outMin + out * (outMax - outMin);
            return output;
        }

    private:
        RangeMapInterface *widget = new RangeMapInterface();
        std::shared_ptr<VariableData> m_inData;
        QJSEngine *m_jsEngine = nullptr;

        double m_inMin = 0.0;
        double m_inMax = 1.0;
        double m_outMin = 0.0;
        double m_outMax = 255.0;
        bool m_clamp = true;
    };
}
