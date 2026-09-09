#pragma once

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/qglobal.h>
#include <QtCore/QDebug>
#include <QtWidgets/QLineEdit>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "JSEngineDefines/JSEngineDefines.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace QtNodes;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief Log Out：用完整 JS 表达式生成消息，打印到终端并从 OUTPUT 输出。
     * 输入以 $input 注入引擎（与 Extract 一致）；输出附带明文时间戳。
     */
    class LogOutDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        LogOutDataModel()
        {
            InPortCount = 1;
            OutPortCount = 1;
            Caption = QStringLiteral("Log Out");
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = false;

            widget->setPlaceholderText(
                QStringLiteral("JS Expression (e.g., \"设备允许错误\" + $input.default)"));
            widget->setText(QStringLiteral("$input.default"));

            m_outData = std::make_shared<VariableData>();
            m_jsEngine = new QJSEngine(this);

            connect(widget, &QLineEdit::editingFinished, this, [this]() {
                if (m_lastValue) {
                    processValue(m_lastValue);
                }
            });
        }

        ~LogOutDataModel() override
        {
            if (m_jsEngine) {
                delete m_jsEngine;
                m_jsEngine = nullptr;
            }
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            switch (portType) {
            case PortType::In:
                return QStringLiteral("VALUE");
            case PortType::Out:
                return QStringLiteral("OUTPUT");
            default:
                return QString();
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return m_outData ? m_outData : std::make_shared<VariableData>();
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            if (data == nullptr) {
                return;
            }

            auto incoming = std::dynamic_pointer_cast<VariableData>(data);
            if (!incoming) {
                return;
            }

            m_lastValue = std::make_shared<VariableData>(incoming->asMap());
            processValue(m_lastValue);
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values[QStringLiteral("message")] = widget->text();

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson[QStringLiteral("values")] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p[QStringLiteral("values")];
            if (!v.isUndefined() && v.isObject()) {
                widget->setText(v.toObject().value(QStringLiteral("message")).toString());
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

    private:
        void processValue(const std::shared_ptr<VariableData> &incoming)
        {
            if (!incoming) {
                return;
            }

            const QString message = evaluateMessage(incoming);
            const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));

            QJsonObject payload;
            payload.insert(QStringLiteral("timestamp"), ts);
            payload.insert(QStringLiteral("log"), message);
            const QString jsonText = QString::fromUtf8(
                QJsonDocument(payload).toJson(QJsonDocument::Compact));

            m_outData = std::make_shared<VariableData>(jsonText);

            qInfo().noquote() << message;

            Q_EMIT dataUpdated(0);
        }

        /** 整段文本作为 JS 表达式求值；$input 为完整输入对象 */
        QString evaluateMessage(const std::shared_ptr<VariableData> &incoming) const
        {
            const QString expression = widget->text().trimmed();
            if (expression.isEmpty()) {
                return incoming->toJsonString();
            }

            const QVariantMap dataMap = incoming->asMap();
            QJSValue global = m_jsEngine->globalObject();
            global.setProperty(QStringLiteral("$input"),
                               JSEngineDefines::variantMapToJSValue(m_jsEngine, dataMap));

            for (auto it = dataMap.begin(); it != dataMap.end(); ++it) {
                global.setProperty(it.key(), m_jsEngine->toScriptValue(it.value()));
            }

            const QJSValue result = m_jsEngine->evaluate(expression);
            if (result.isError()) {
                qDebug() << "Log Out JS表达式错误:" << result.toString();
                return QString();
            }

            return jsValueToDisplay(result);
        }

        static QString jsValueToDisplay(const QJSValue &value)
        {
            if (value.isUndefined() || value.isNull()) {
                return QString();
            }
            if (value.isString() || value.isNumber() || value.isBool()) {
                return value.toString();
            }

            const QVariant variant = value.toVariant();
            if (variant.typeId() == QMetaType::QVariantList || variant.typeId() == QMetaType::QStringList) {
                return QString::fromUtf8(
                    QJsonDocument(QJsonArray::fromVariantList(variant.toList()))
                        .toJson(QJsonDocument::Compact));
            }
            if (variant.typeId() == QMetaType::QVariantMap) {
                return QString::fromUtf8(
                    QJsonDocument(QJsonObject::fromVariantMap(variant.toMap()))
                        .toJson(QJsonDocument::Compact));
            }
            return variant.toString();
        }

    private:
        QLineEdit *widget = new QLineEdit();
        QJSEngine *m_jsEngine = nullptr;
        std::shared_ptr<VariableData> m_outData;
        std::shared_ptr<VariableData> m_lastValue;
    };
}
