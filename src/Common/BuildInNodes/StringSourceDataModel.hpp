#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>
#include "AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

#include <QtCore/qglobal.h>


using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes {
    class TextSourceDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)

    public:

        TextSourceDataModel():_lineEdit(new QLineEdit("")){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="String Source";
            WidgetEmbeddable=true;
            Resizable=false;
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "value";
            binding.control=_lineEdit;
            AbstractDelegateModel::registerExternalBinding("/string", this, binding);
            // AbstractDelegateModel::registerExternalControl("/string",_lineEdit);
            _lineEdit->setText(m_value);
            connect(_lineEdit, &QLineEdit::textChanged, this, &TextSourceDataModel::onTextEdited);
            connect(this, &TextSourceDataModel::valueChanged, this, [this](const QString &){
                if (_lineEdit->text()!=m_value)
                    _lineEdit->blockSignals(true);
                    _lineEdit->setText(m_value);
                    _lineEdit->blockSignals(false);
            });
        }


        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {

            Q_UNUSED(portIndex)
            switch (portType) {
                case PortType::In:
                    return VariableData().type();
                case PortType::Out:
                    return VariableData().type();
                case PortType::None:
                    break;
                default:
                    break;
            }
            // FIXME: control may reach end of non-void function [-Wreturn-type]

            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return std::make_shared<VariableData>(value());
        }

        /**
         * 函数级注释：获取当前字符串属性值
         */
        QString value() const
        {
            return m_value;
        }

        /**
         * 函数级注释：设置当前字符串属性值，仅在变化时发出通知
         */
        void setValue(const QString &v)
        {

            m_value = v;
            Q_EMIT dataUpdated(0);
            Q_EMIT valueChanged(v);
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{
            if (data== nullptr){
                setValue("");
                return;
            }
            auto textData = std::dynamic_pointer_cast<VariableData>(data);
            QString v;
            if (textData && textData->value().canConvert<QString>()) {
                v = textData->value().toString();
            }
            setValue(v);
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["text"] = value();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                setValue(v["text"].toString());
            }
        }
        QWidget *embeddedWidget() override{return _lineEdit;}

    signals:
        /**
         * 函数级注释：字符串属性发生变化时发出的通知信号
         */
        void valueChanged(const QString &value);

    protected:
        /**
         * 函数级注释：模型就绪后订阅字符串地址的命令事件
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/string"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:

        /**
         * 函数级注释：本地编辑字符串时通过属性接口驱动业务逻辑
         */
        void onTextEdited(QString const &string)
        {
            Q_UNUSED(string);
            setValue(_lineEdit->text());
        }

    public Q_SLOTS:
        /**
         * 函数级注释：处理来自事件总线的字符串命令事件，更新控件与状态
         */
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address != makeFullOscAddress("/string")) {
                return;
            }
            const QString v = ev.payload.toString();
            setValue(v);
        }

    private:
        QLineEdit *_lineEdit;
        QString m_value;

    };
}
