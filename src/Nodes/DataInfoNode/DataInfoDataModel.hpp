#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "Common/GUI/DataTreeModel/QmlDataBrowser.h"
#include <iostream>
#include <QVBoxLayout>
#include <QtCore/qglobal.h>
#include <QtQuickWidgets/QQuickWidget>
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
    /// The model dictates the number of inputs and outputs for the Node.
    /// In this example it has no logic.
    // 在现有代码中添加QML支持

    
    class DataInfoDataModel : public NodeDelegateModel
    {
        Q_OBJECT
    
    public:
        DataInfoDataModel()
        {
            InPortCount = 1;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Data Info";
            WidgetEmbeddable = true;
            Resizable = true;
            
            // 使用QML版本的数据浏览器
            qmlWidget = new QmlDataBrowser();
            qmlWidget->setLazyLoadingEnabled(true);
            qmlWidget->setMaxVisibleItems(1000);
        }
         ~DataInfoDataModel() override{}

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return inData;
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

        QWidget *embeddedWidget() override {
            return qmlWidget;
        }
    
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex);
            if (data == nullptr) {
                return;
            }
            
            if (inData = std::dynamic_pointer_cast<VariableData>(data)) {
                QVariantMap newModel = inData->getMap();
                
                // 使用高性能的增量更新
                if (model.isEmpty()) {
                    qmlWidget->buildPropertiesFromMap(newModel);
                } else {
                    qmlWidget->updatePropertiesIncremental(newModel);
                }
                
                model = newModel;
                Q_EMIT dataUpdated(0);
            }

        }
    
    private:
        QVariantMap model;
        QmlDataBrowser *qmlWidget;
        std::shared_ptr<VariableData> inData;
    };
}