#pragma once
#include <QtCore/QObject>
#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <iostream>
#include <QtCore/qglobal.h>
#include <cstdio>
#include <iostream>
#if WIN32
#include "windows.h"
#include <wtypes.h>
#endif
#include "public.sdk/samples/vst-hosting/editorhost/source/editorhost.h"
#include "public.sdk/samples/vst-hosting/editorhost/source/platform/appinit.h"
#include "public.sdk/samples/vst-hosting/editorhost/source/platform/iapplication.h"
#include "public.sdk/samples/vst-hosting/editorhost/source/platform/iwindow.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/utility/optional.h"
#include <QWindow>
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugview.h"
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivstunits.h>
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/vsttypes.h"
//#include "base/source/fcommandline.h"
#include "base/source/fdebug.h"
#include <QFileDialog>
#include "VST3PluginInterface.hpp"
#include "public.sdk/source/main/pluginfactory_constexpr.h"
#include "WindowContainer.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace Steinberg;
using namespace VST3;

class VST3PluginDataModel : public NodeDelegateModel
{
    Q_OBJECT


public:

    VST3PluginDataModel(){
        InPortCount =2;
        OutPortCount=2;
        CaptionVisible=true;
        Caption="VST3 Plugin";
        WidgetEmbeddable=true;
        Resizable=false;
        widget=new VST3PluginInterface();
        connect(widget->SelectVST,&QPushButton::clicked,this,&VST3PluginDataModel::onTextEdited);
//        connect(this->widget, &VST3PluginInterface::SelectVst3, this, &VST3PluginDataModel::onTextEdited);
        connect(widget->ShowController,&QPushButton::clicked,this,&VST3PluginDataModel::showController);
    }
    ~VST3PluginDataModel(){
//        delete button;

    }
public slots:
    void onTextEdited()
    {
//        QFileDialog *fileDialog=new QFileDialog();

        QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                        tr("Select vst3 File"), "./VST3", tr("Audio Files (*.vst3)"));
        if(fileName!="")
        {
            loadPlugin(fileName);

            Q_EMIT dataUpdated(0);
        }

    }
    void showController()
    {
        view = owned (editController->createView (Vst::ViewType::kEditor));
        window = std::make_shared<WindowContainer>(view);
        window->setWindowTitle( module->getFactory ().info().vendor().c_str());
//        window->setParent(widget->windowHandle());
        window->show();

    }
public:

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
        return std::make_shared<VariableData>();
    }
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{

        if (data== nullptr){
            return;
        }
        auto textData = std::dynamic_pointer_cast<VariableData>(data);

        Q_EMIT dataUpdated(portIndex);
    }


    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }
    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {

        }
    }
    QWidget *embeddedWidget() override{return widget;}



    void loadPlugin(const QString& pluginPath) {
        std::string error;
        auto PluginContext = owned(NEW Steinberg::Vst::HostApplication());
        Vst::PluginContextFactory::instance ().setPluginContext (PluginContext);
        VST3::Optional<VST3::UID> uid;
        module =VST3::Hosting::Module::create(pluginPath.toStdString().c_str(), error);
//        IPtr<Steinberg::Vst::PlugProvider> plugProvider {nullptr};
        if (!module) {
            qWarning() << "Failed to load module from path: " << pluginPath ;
            return ;
        }
        const auto& factory = module->getFactory();
        QVariantMap pluginInfo;
        pluginInfo["Plugin Path"]=pluginPath;
        // 假设 VST3::Hosting::Module 提供了相关 API 获取插件信息
        auto factoryInfo = module->getFactory ().info() ;
        pluginInfo["Vendor"]= QString::fromStdString(factoryInfo.get().vendor);
        pluginInfo["Version"]=QString::fromStdString(factoryInfo.get().url);
        pluginInfo["email"]=QString::fromStdString(factoryInfo.get().email);
        widget->browser->buildPropertiesFromMap(pluginInfo);

        for (auto& classInfo : factory.classInfos ())
        {
            if (classInfo.category () == kVstAudioEffectClass)
                plugProvider = owned (new Steinberg::Vst::PlugProvider (factory, classInfo, true));
            if (plugProvider->initialize () == false)
                plugProvider = nullptr;
            break;

        }
        if (!plugProvider)
        {
            qDebug()<< "No VST3 Audio Module Class found in file ";
            return;
        }
        editController = plugProvider->getController ();
        vstPlug=plugProvider->getComponent();
        if (!editController)
        {
            qDebug()<<"No EditController found (needed for allowing editor) in file ";
            widget->ShowController->setEnabled(false);
            return ;
        }
        widget->ShowController->setEnabled(true);
//        controller->release ();


    }


private:
    VST3::Hosting::Module::Ptr module = nullptr;
    Steinberg::IPtr<Steinberg::Vst::PlugProvider> plugProvider = nullptr;
    Steinberg::IPtr<Steinberg::Vst::IComponent> vstPlug = nullptr;
    Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> audioEffect = nullptr;
    Steinberg::IPtr<Steinberg::Vst::IEditController> editController = nullptr;

    Steinberg::IPtr<Steinberg::IPlugView> view = nullptr;
    VST3PluginInterface * widget;
//    Vst::HostApplication pluginContext;
    std::shared_ptr<WindowContainer> window;
};

