#pragma once

#include "Common/DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPointer>
#include <QQmlContext>
#include <vector>
#include <QtCore/qglobal.h>
// #include "DataVisualInterface.hpp"
#include <QThread>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QWidget>
#include <QSizePolicy>

#include "Common/BaseClass/AbstractDelegateModel.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
    class ScatterSeriesDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        ScatterSeriesDataModel()
        {
            InPortCount =3;
            OutPortCount=0;
            CaptionVisible=true;
            Caption="ScatterSeries";
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            m_view = new QQuickView();
            m_view->setResizeMode(QQuickView::SizeRootObjectToView);
            m_view->rootContext()->setContextProperty("CppBridge", this);
            m_view->setSource(QUrl("qrc:/qml/3DScatter/main.qml"));

            m_container = QWidget::createWindowContainer(m_view);
            m_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            // NodeDelegateModel::registerOSCControl("/stop",widget->stopButton);

            // connect(widget->editButton, &QPushButton::clicked, this, &ScatterSeriesDataModel::toggleEditorMode);
        }

        ~ScatterSeriesDataModel() override {
            if (m_container) {
                m_container->deleteLater();
                m_container = nullptr;
                m_view = nullptr;
            }
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

            invokeRoot3("appendPoint", m_x, m_y, m_z);

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
            return m_container;
        }

    public:
        // DataVisualInterface *widget=new DataVisualInterface();
        std::shared_ptr<VariableData> inData;
        std::shared_ptr<VariableData> currentValue_Y=std::make_shared<VariableData>(0.0);
        std::shared_ptr<VariableData> currentValue_X=std::make_shared<VariableData>(0.0);
        std::shared_ptr<VariableData> currentTime=std::make_shared<VariableData>(0.0);
        std::shared_ptr<VariableData> currentStatus=std::make_shared<VariableData>(QVariant(false));
        void *patch;

    private:
        /**
         * 函数级注释：线程安全地调用 QML 根对象方法（带 3 个 double 参数）
         */
        void invokeRoot3(const char* method, double x, double y, double z)
        {
            if (!m_view) { return; }

            QPointer<QQuickView> safeView = m_view;
            const auto fn = [safeView, method, x, y, z]() {
                if (!safeView) { return; }
                QQuickItem* root = safeView->rootObject();
                if (!root) { return; }
                QMetaObject::invokeMethod(root, method,
                                         Q_ARG(QVariant, QVariant(x)),
                                         Q_ARG(QVariant, QVariant(y)),
                                         Q_ARG(QVariant, QVariant(z)));
            };

            if (QThread::currentThread() == m_view->thread()) {
                fn();
            } else {
                QMetaObject::invokeMethod(m_view.data(), fn, Qt::QueuedConnection);
            }
        }

        QPointer<QQuickView> m_view;
        QPointer<QWidget> m_container;

        // 函数级注释：最近一次 x/y/z 值缓存与就绪标记
        double m_x {0.0};
        double m_y {0.0};
        double m_z {0.0};

    };
}
