#pragma once


#include <QtCore/QObject>

#include "Nodes/NodeDataList.hpp"
#include "QDir"
#include <QtNodes/NodeDelegateModel>
#include "QThread"
#include <iostream>
#include <QtWidgets/QLineEdit>
#include "LuaScriptInterface.hpp"
#include "LuaThread.h"
#include <QtCore/qglobal.h>


using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;


/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class LuaDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:

    LuaDataModel()
    {
        InPortCount =4;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Lua Script";
        WidgetEmbeddable=false;
        Resizable=false;
        connect(widget->codeWidget->run,SIGNAL(clicked(bool)),this,SLOT(onButtonClicked(bool)));

//        luaEngine->start();
    }
    ~LuaDataModel()
    {
//       if(luaThread){
//            luaThread->stop();
//        }
        widget->deleteLater();
    }
public:

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

public:
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
//        Q_UNUSED(portIndex)
        Q_UNUSED(portType)
        switch (portIndex) {
            case 0:
                return StringData().type();
                break;
            case 1:
                return BoolData().type();
                break;
            case 2:
                return IntData().type();
                break;
            case 3:
                return FloatData().type();
                break;
        }
        return BoolData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        return std::make_shared<StringData>("_lineEdit->text()");
    }

    void setInData(std::shared_ptr<NodeData>, PortIndex const) override {}

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

    void onButtonClicked(bool const &string)
    {
        Q_UNUSED(string);
        script=widget->codeWidget->saveCode();
        loadScripts(script);
        Q_EMIT dataUpdated(0);
    }

    void loadScripts(QString code="print(\"Lua version: \" .. _VERSION)")
    {
        luaEngine = new LuaThread(code);
        luaEngine->start();
    }

private:
    QString script;
    LuaScriptInterface *widget=new LuaScriptInterface();
    LuaThread *luaEngine;
};
