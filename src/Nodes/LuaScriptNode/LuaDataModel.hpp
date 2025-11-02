#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"
#include "QDir"
#include <QtNodes/NodeDelegateModel>
#include "QThread"
#include <iostream>
#include "LuaScriptInterface.hpp"
#include "LuaThread.h"
#include <QtCore/qglobal.h>
#include "LuaModifier.hpp"
#include "QPushButton"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class LuaDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:

        LuaDataModel():widget(new LuaScriptInterface())
        {
            InPortCount =4;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Lua Script";
            WidgetEmbeddable=false;
            Resizable=true;
            PortEditable=true;
            inputPortIndex=0;
            connect(widget->codeWidget->run,SIGNAL(clicked(bool)),this,SLOT(onButtonClicked()));
        }
        ~LuaDataModel()
        {
            if(luaEngine!=nullptr){
                luaEngine->deleteLater();
            }
            widget->deleteLater();

        }
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            QString in = "➩";
            QString out = "➩";
            switch (portType) {
            case PortType::In:
                return in;
            case PortType::Out:
                return out;
            default:
                break;
            }
            return "";
        }

        unsigned int nPorts(PortType portType) const override
        {
            unsigned int result = 1;

            switch (portType) {
            case PortType::In:
                result = InPortCount;
                break;

            case PortType::Out:
                result = OutPortCount;

            default:
                break;
            }

            return result;
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portType)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            // Q_UNUSED(portIndex)
            //        return std::make_shared<VariableData>(out_dictionary[portIndex].getVariant());
            return std::make_shared<VariableData>(out[portIndex].variantMap);
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            if (auto d = std::dynamic_pointer_cast<VariableData>(data))
            {
                in[portIndex]=LuaQVariantMap(d->getMap());
                inputPortIndex=portIndex;
                //            in_dictionary[portIndex].setVariant( d->value());
                onButtonClicked();
                //            Q_EMIT dataUpdated(0);
            }
        }

        QWidget *embeddedWidget() override
        {
            // return showEditorButton;
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["scripts"] = widget->codeWidget->saveCode();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            modelJson["InPortCount"] =int(InPortCount);
            modelJson["OutPortCount"] =int(OutPortCount);
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                script = v["scripts"].toString();
                widget->codeWidget->loadCodeFromCode(script);
                InPortCount=p["InPortCount"].toInt();
                OutPortCount=p["OutPortCount"].toInt();
            };
        }

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

    private Q_SLOTS:
    /**
     * @brief 手动运行脚本
     */
        void onButtonClicked()
        {

            script=widget->codeWidget->saveCode();
            loadScripts(script);
            //        Q_EMIT dataUpdated(0);
        }
        /**
         * @brief 执行lua脚本
         * @param code lua代码
         */
        void loadScripts(QString code="print(\"Lua version: \" .. _VERSION)")
        {
            if (!luaEngine) {
                luaEngine = new LuaThread(code,this);

            }

            luaEngine->setCode(code);


            luaEngine->start();

        }
    public:
        /**
         * @brief
         * @return 输入端口数量
         */
        unsigned int getInputCount() {
            return InPortCount;
        }
        /**
         * @brief
         * @return 输出端口数量
         */
        unsigned int getOutputCount() {
            return OutPortCount;
        }
        /**
         * @brief
         * @return 端口索引
         */
        unsigned int inputIndex() {
            return inputPortIndex;
        }
        //    /**
        //     * @brief
        //     * @param portType 端口类型
        //     * @param index 端口索引
        //     * @return
        //     */
        //    LuaQVariant getDefaultValue(const std::string& portType, unsigned int index) {
        //        if(portType == "In") {
        //            return in_dictionary[index];
        //        }
        //        return out_dictionary[index];
        //    }
        LuaQVariantMap getTableValue(const std::string& portType, unsigned int index) {
            if(portType == "In") {
                return in[index];
            }
            return out[index];
        }
        /**
         * @brief
         * @param index 端口索引
         * @param value lua_stste
         */
        //    void setPortValue(unsigned int index,const LuaQVariant& value) {
        //
        //        QVariant variant = value.getVariant();
        //        if (!variant.isValid()) {
        //            qWarning() << "Invalid variant received for index:" << index;
        //            return;
        //        }
        //
        //        out_dictionary[index] = variant;
        //        QMetaObject::invokeMethod(this, "dataUpdated", Qt::QueuedConnection, Q_ARG(unsigned int, index));
        //    }
        void setTableValue(unsigned int index,const LuaQVariantMap& value) {

            //        QVariant variant = value.getVariant();
            //        if (!variant.isValid()) {
            //            qWarning() << "Invalid variant received for index:" << index;
            //            return;
            //        }

            out[index] = value;
            QMetaObject::invokeMethod(this, "dataUpdated", Qt::QueuedConnection, Q_ARG(unsigned int, index));
        }


    private:
        //    unordered_map<unsigned int, LuaQVariant> in_dictionary;
        //    // 输入值存储
        //    unordered_map<unsigned int, LuaQVariant> out_dictionary;
        unordered_map<unsigned int, LuaQVariantMap> in;
        unordered_map<unsigned int, LuaQVariantMap> out;
        // 输出值存储
        QString script;
        LuaScriptInterface *widget;
        unsigned inputPortIndex;
        LuaThread *luaEngine=nullptr;
        // QPushButton *showEditorButton=new QPushButton("Show editor");
    };

    // 注册类到lua
    static void registerLuaNode(lua_State* L) {
        using namespace luabridge;
        getGlobalNamespace(L)
            .beginClass<LuaDataModel>("LuaDataModel")  //多个类注册需要加一个namespace（test）
            .addConstructor<void(*)()>()                    //无参构造函数的注册
            .addFunction("inputCount", &LuaDataModel::getInputCount)//注册获取输入接口数量的方法到lua
            .addFunction("outputCount", &LuaDataModel::getOutputCount)//注册获取输出接口数量的方法到lua
    //        .addFunction("getDefaultValue", &LuaDataModel::getDefaultValue)//注册获取输入接口值的方法到lua
            .addFunction("getTableValue", &LuaDataModel::getTableValue)//注册获取输入接口值的方法到lua
    //        .addFunction("setValue", &LuaDataModel::setPortValue)//注册设置输出接口值的方法到lua
            .addFunction("setTableValue", &LuaDataModel::setTableValue)//注册设置输出接口值的方法到lua
            .addFunction("inputIndex", &LuaDataModel::inputIndex)//注册设置输出接口索引值的方法到lua
            .endClass();
    }
}