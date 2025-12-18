#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QPlainTextEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>


#include <QtCore/qglobal.h>


using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QPlainTextEdit;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes {
    /**
     * @brief 字符串转JSON数据模型
     */
    class ToJsonDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:

        /**
         * @brief 构造函数
         */
        ToJsonDataModel():_textEdit(new QPlainTextEdit("")){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="To JSON";
            WidgetEmbeddable=true;
            Resizable=true; // 允许调整大小
            connect(_textEdit, &QPlainTextEdit::textChanged, this, &ToJsonDataModel::onTextEdited);
        }


        /**
         * @brief 获取端口数据类型
         */
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
        /**
         * @brief 获取输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            QString text = _textEdit->toPlainText();
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &error);
            
            if (error.error == QJsonParseError::NoError && doc.isObject()) {
                return std::make_shared<VariableData>(doc.object().toVariantMap());
            } else {
                QVariantMap map;
                map["default"] = text;
                return std::make_shared<VariableData>(map);
            }
        }

        /**
         * @brief 设置输入数据
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{
            if (data== nullptr){
                _textEdit->setPlainText("");
                return;
            }
            auto textData = std::dynamic_pointer_cast<VariableData>(data);
            if (textData->value().canConvert<QString>()) {
                _textEdit->setPlainText(textData->value().toString());
            } else {
                _textEdit->setPlainText("");
            }

            Q_EMIT dataUpdated(portIndex);
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
        {
            if (portType == QtNodes::PortType::In) {
                return "STRING";
            }
            if (portType == QtNodes::PortType::Out) {
                return "JSON";
            }
            return "";
        }

        /**
         * @brief 保存节点状态
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["text"] = _textEdit->toPlainText();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        
        /**
         * @brief 加载节点状态
         */
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                _textEdit->setPlainText(v["text"].toString());
            }
        }
        
        /**
         * @brief 获取嵌入式控件
         */
        QWidget *embeddedWidget() override{return _textEdit;}

        /**
         * @brief 端口连接策略
         */
        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override {
            auto result = ConnectionPolicy::One;
            switch (portType) {
                case PortType::In:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::Out:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::None:
                    break;
            }

            return result;
        }
        
        /**
         * @brief 状态反馈
         */
        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }

    private Q_SLOTS:

        /**
         * @brief 文本编辑槽函数
         */
        void onTextEdited()
        {
            Q_EMIT dataUpdated(0);
        }

    private:
        QPlainTextEdit *_textEdit;

    };
}
