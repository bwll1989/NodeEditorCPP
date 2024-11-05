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

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
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
        Resizable=false;
        PortEditable=true;
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
        return VariantData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        // Q_UNUSED(portIndex)
        return std::make_shared<VariantData>(out_dictionary[portIndex].getVariant());
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
        if (auto d = std::dynamic_pointer_cast<VariantData>(data))
        {
            in_dictionary[portIndex].setVariant( d->NodeValues);
            onButtonClicked();
            Q_EMIT dataUpdated(0);
        }
    }

    QWidget *embeddedWidget() override
    {
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
private Q_SLOTS:

    void onButtonClicked()
    {

        script=widget->codeWidget->saveCode();
        loadScripts(script);
        Q_EMIT dataUpdated(0);
    }

    void loadScripts(QString code="print(\"Lua version: \" .. _VERSION)")
    {
        if (!luaEngine) {
            luaEngine = new LuaThread(code,this);

        }

        luaEngine->setCode(code);


        luaEngine->start();

    }
public:
    void test() {
        qDebug() << "test";
    }
    unsigned int getInputCount() {
        return InPortCount;
    }
    unsigned int getOutputCount() {
        return OutPortCount;
    }

    LuaQVariant getPortValue(const std::string& portType, unsigned int index) {
        if(portType == "In") {
            return in_dictionary[index];
        }
        return out_dictionary[index];
    }

    void setPortValue(unsigned int index,const LuaQVariant& value) {

        try {
            out_dictionary[index]=value.getVariant();
            Q_EMIT dataUpdated(index);
        } catch (...) {
            qWarning() << "setPortValue error";
        }
    }


private:
    unordered_map<unsigned int, LuaQVariant> in_dictionary;
    // 输入值存储
    unordered_map<unsigned int, LuaQVariant> out_dictionary;
    // 输出值存储
    QString script;
    LuaScriptInterface *widget;
    LuaThread *luaEngine=nullptr;
};
// 注册类到lua
static void registerLuaNode(lua_State* L) {
    using namespace luabridge;
    getGlobalNamespace(L)
        .beginClass<LuaDataModel>("LuaDataModel")  //多个类注册需要加一个namespace（test）
        .addConstructor<void(*)()>()                    //无参构造函数的注册
        .addFunction("inputCount", &LuaDataModel::getInputCount)//注册获取输入接口数量的方法到lua
        .addFunction("outputCount", &LuaDataModel::getOutputCount)//注册获取输出接口数量的方法到lua
        .addFunction("getValue", &LuaDataModel::getPortValue)//注册获取输入接口值的方法到lua
        .addFunction("setValue", &LuaDataModel::setPortValue)//注册设置输出接口值的方法到lua
        // .addFunction("setValue", &LuaDataModel::luaSetValue)
        .endClass();
}