#pragma once

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "Common/GUI/DataTreeModel/DataBrowserWidget.h"
#include <iostream>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QtCore/qglobal.h>
// #include "DockManager.h"
// #include "DockHub/DockHub.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
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
    class DataInfoDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        DataInfoDataModel()
        {
            InPortCount = 1;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Data Info";
            WidgetEmbeddable = false;
            Resizable = false;
            WidgetEmbeddable = true;
            Resizable = false;
            viewButton=new QPushButton("View");
            // 【2026-11-26】QWidget 版数据浏览器（替代 QML 版，去 Quick 依赖）
            browserWidget = new DataBrowserWidget();
            browserWidget->setLazyLoadingEnabled(true);
            browserWidget->setMaxVisibleItems(1000);
            connect(viewButton,&QPushButton::clicked,this,&DataInfoDataModel::toggleEditorMode);
        }
         ~DataInfoDataModel() override {
            if (browserWidget) {
                browserWidget->setParent(nullptr);
                browserWidget->deleteLater();
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
                QVariantMap newModel = inData->asMap();

                // 使用高性能的增量更新
                if (model.isEmpty()) {
                    browserWidget->buildPropertiesFromMap(newModel);
                } else {
                    browserWidget->updatePropertiesIncremental(newModel);
                }

                model = newModel;
                Q_EMIT dataUpdated(0);
            }

        }
    public slots:
        void toggleEditorMode() {
            // // 移除父子关系，使其成为独立窗口
            browserWidget->setParent(nullptr);

            // 设置为独立窗口
            browserWidget->setWindowTitle("Data Info");

            // 设置窗口图标
            browserWidget->setWindowIcon(QIcon(":/icons/icons/info.png"));

            // 设置窗口标志：独立窗口 + 置顶显示 + 关闭按钮
            browserWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

            // 设置窗口属性：当关闭时自动删除
            browserWidget->setAttribute(Qt::WA_DeleteOnClose, false); // 不自动删除，我们手动管理
            browserWidget->setAttribute(Qt::WA_QuitOnClose, false);   // 关闭窗口时不退出应用程序

            // 设置窗口大小和显示
            browserWidget->resize(800, 400);
            browserWidget->show();
            // 激活窗口并置于前台
            browserWidget->activateWindow();
            browserWidget->raise();
        }

    private:
        QPushButton *viewButton;
        QVariantMap model;
        DataBrowserWidget *browserWidget;
        std::shared_ptr<VariableData> inData;
    };
}