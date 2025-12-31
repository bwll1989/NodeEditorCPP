#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QPlainTextEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include "AbstractDelegateModel.h"

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
     * @brief JSON转字符串数据模型
     */
    class FromJsonDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:

        /**
         * @brief 构造函数
         */
        FromJsonDataModel():_textEdit(new QPlainTextEdit("")){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="From JSON";
            WidgetEmbeddable=true;
            Resizable=true; // 允许调整大小

            connect(_textEdit, &QPlainTextEdit::textChanged, this, &FromJsonDataModel::onTextEdited);
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
            return std::make_shared<VariableData>(_textEdit->toPlainText());
        }

        /**
         * @brief 设置输入数据
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{
            if (data == nullptr){
                _textEdit->setPlainText("");
                Q_EMIT dataUpdated(portIndex);
                return;
            }
            auto varData = std::dynamic_pointer_cast<VariableData>(data);
            if (varData) {
                _textEdit->setPlainText(varData->toJsonString());
            } else {
                _textEdit->setPlainText("");
            }

            Q_EMIT dataUpdated(portIndex);
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
        {
            if (portType == QtNodes::PortType::In) {
                return "JSON";
            }
            if (portType == QtNodes::PortType::Out) {
                return "STRING";
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
