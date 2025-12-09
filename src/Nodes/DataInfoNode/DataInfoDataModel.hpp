#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "Common/GUI/DataTreeModel/QmlDataBrowser.h"
#include <iostream>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QtCore/qglobal.h>
#include <QtQuickWidgets/QQuickWidget>
// #include "DockManager.h"
// #include "DockHub/DockHub.hpp"
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
            viewButton=new QPushButton("View");
            // 使用QML版本的数据浏览器
            qmlWidget = new QmlDataBrowser();
            qmlWidget->setLazyLoadingEnabled(true);
            qmlWidget->setMaxVisibleItems(1000);
            connect(viewButton,&QPushButton::clicked,this,&DataInfoDataModel::toggleEditorMode);
        }
         ~DataInfoDataModel() override {
            if (qmlWidget) {
                qmlWidget->setParent(nullptr);
                qmlWidget->deleteLater();
            }
        }

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
            return viewButton;
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
    public slots:
        void toggleEditorMode() {
            // // 移除父子关系，使其成为独立窗口
            qmlWidget->setParent(nullptr);

            // 设置为独立窗口
            qmlWidget->setWindowTitle("Data Info");

            // 设置窗口图标
            qmlWidget->setWindowIcon(QIcon(":/icons/icons/info.png"));

            // 设置窗口标志：独立窗口 + 置顶显示 + 关闭按钮
            qmlWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

            // 设置窗口属性：当关闭时自动删除
            qmlWidget->setAttribute(Qt::WA_DeleteOnClose, false); // 不自动删除，我们手动管理
            qmlWidget->setAttribute(Qt::WA_QuitOnClose, false);   // 关闭窗口时不退出应用程序

            // 设置窗口大小和显示
            qmlWidget->resize(800, 400);
            qmlWidget->show();
            // 激活窗口并置于前台
            qmlWidget->activateWindow();
            qmlWidget->raise();
        }

    private:
        QPushButton *viewButton;
        QVariantMap model;
        QmlDataBrowser *qmlWidget;
        std::shared_ptr<VariableData> inData;
    };
}