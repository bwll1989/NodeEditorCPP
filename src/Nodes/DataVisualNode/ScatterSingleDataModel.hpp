#pragma once

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QQmlContext>
#include <QQuickItem>
#include <vector>
#include <QtCore/qglobal.h>
#include "ConstantDefines.h"
#include "DataVisualInterface.hpp"
#include "OSCSender/OSCSender.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
    class ScatterSingleDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        ScatterSingleDataModel()
        {
            InPortCount =3;
            OutPortCount=0;
            CaptionVisible=true;
            Caption="ScatterSingle";
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
            // m_quickWidget->setSource(QUrl("qrc:qml/content/dynamicview.qml"));
            m_quickWidget->setSource(QUrl("qrc:/qml/3DScatter/main.qml"));
            m_quickWidget->rootContext()->setContextProperty("CppBridge", this);

            // NodeDelegateModel::registerOSCControl("/stop",widget->stopButton);

            connect(widget->editButton, &QPushButton::clicked, this, &ScatterSingleDataModel::toggleEditorMode);
        }

        ~ScatterSingleDataModel() override {
            if (m_quickWidget && m_quickWidget->isVisible()) {
                m_quickWidget->close();
            }
            // 如果窗口仍然存在，强制隐藏
        }

    public:

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                    switch (portIndex) {
                        case 0:  return "VALUE_X";
                        case 1:  return "VALUE_Y";
                        case 2:  return "VALUE_Z";
                        default: break;
                    }
            case PortType::Out:
                break;
            default:
                break;
            }
            return "";
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        // 函数级注释：根据输出端口返回对应的数据（0:时间[s]，1:百分比[%]，2:状态[bool]）
        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            return nullptr;
        }

        // 函数级注释：根据输入端口更新 x/y/z；当三者均就绪时，调用 QML 的 appendPoint(x,y,z) 追加散点
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (!data) { return; }
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) { return; }

            const double v = var->value().toDouble();
            switch (portIndex) {
                case 0: m_x = v; break; // X
                case 1: m_y = v; break; // Y
                case 2: m_z = v; break; // Z
                default:
                    return;

            }

            if (QObject* root = m_quickWidget->rootObject()) {
                QMetaObject::invokeMethod(root, "updatePoint",
                    Q_ARG(QVariant, QVariant(m_x)),
                    Q_ARG(QVariant, QVariant(m_y)),
                    Q_ARG(QVariant, QVariant(m_z)));
            }

        }

        // 函数级注释：保存节点状态到 JSON（当前示例不持久化图数据）
        QJsonObject save() const override
        {
            QJsonObject modelJson  = NodeDelegateModel::save();
            QJsonObject values;
            // 保留结构，暂不写入具体数据
            modelJson.insert("values", values);
            return modelJson;
        }

        // 函数级注释：加载节点状态（当前示例不持久化图数据）
        void load(const QJsonObject &p) override
        {
            Q_UNUSED(p);
        }

        QWidget *embeddedWidget() override{
            return widget;
        }

    public Q_SLOTS:
        void toggleEditorMode() {
            // 移除父子关系，使其成为独立窗口
            m_quickWidget->setParent(nullptr);
            m_quickWidget->setWindowTitle("DataVisual编辑器");
            m_quickWidget->setWindowIcon(QIcon(":/icons/icons/DataVisual.png"));
            m_quickWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
            m_quickWidget->setAttribute(Qt::WA_DeleteOnClose, false);
            m_quickWidget->setAttribute(Qt::WA_QuitOnClose, false);
            m_quickWidget->resize(800, 400);
            m_quickWidget->show();
            m_quickWidget->activateWindow();
            m_quickWidget->raise();
        }




        // 函数级注释：循环播放复选框槽函数，直接调用 QML 的 TimelineEditor.isLoop 切换循环播放状态



        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }
    public:
        DataVisualInterface *widget=new DataVisualInterface();
        shared_ptr<VariableData> inData;
        shared_ptr<VariableData> currentValue_Y=make_shared<VariableData>(0.0);
        shared_ptr<VariableData> currentValue_X=make_shared<VariableData>(0.0);
        shared_ptr<VariableData> currentTime=make_shared<VariableData>(0.0);
        shared_ptr<VariableData> currentStatus=make_shared<VariableData>(QVariant(false));
        void *patch;
        QQuickWidget *m_quickWidget=new QQuickWidget();

    private:
        // 函数级注释：最近一次 x/y/z 值缓存与就绪标记
        double m_x {0.0};
        double m_y {0.0};
        double m_z {0.0};

    };
}