#pragma once

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
//#include "z_libpd.h"
#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include "QTimer"
#include <iostream>
#include <vector>
#include <QtCore/qglobal.h>
#include "QSpinBox"
#include "QMLInterface.hpp"
#include "QtMath"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
    class QMLDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        QMLDataModel()
        {
            InPortCount =3;
            OutPortCount=3;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=true;
            PortEditable= false;
            m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
            // m_quickWidget->setSource(QUrl("qrc:qml/content/dynamicview.qml"));
            m_quickWidget->setSource(QUrl("qrc:qml/content/main.qml"));
            m_quickWidget->rootContext()->setContextProperty("CppBridge", this);
            connect(widget->editButton, &QPushButton::clicked, this, &QMLDataModel::toggleEditorMode);
        }

       ~QMLDataModel() override {
            if (m_quickWidget && m_quickWidget->isVisible()) {
                    m_quickWidget->close();
                }
                // 如果窗口仍然存在，强制隐藏

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

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            //        Q_UNUSED(port);
            return  inData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {

            if (data== nullptr){
                return;
            }

        }

        QWidget *embeddedWidget() override{

            return widget;
        }
    public Q_SLOTS:
        Q_INVOKABLE void togglePlay() {
            qDebug() << "togglePlay";
        }


        void toggleEditorMode() {
            // 移除父子关系，使其成为独立窗口
            m_quickWidget->setParent(nullptr);

            // 设置为独立窗口
            m_quickWidget->setWindowTitle("编辑器");

            // 设置窗口图标
            m_quickWidget->setWindowIcon(QIcon(":/icons/icons/js.png"));

            // 设置窗口标志：独立窗口 + 置顶显示 + 关闭按钮
            m_quickWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

            // 设置窗口属性：当关闭时自动删除
            m_quickWidget->setAttribute(Qt::WA_DeleteOnClose, false); // 不自动删除，我们手动管理
            m_quickWidget->setAttribute(Qt::WA_QuitOnClose, false);   // 关闭窗口时不退出应用程序

            // 设置窗口大小和显示
            m_quickWidget->resize(800, 400);
            m_quickWidget->show();
            // 激活窗口并置于前台
            m_quickWidget->activateWindow();
            m_quickWidget->raise();
        }
        Q_INVOKABLE void updatePlayheadTime(const QVariantList &parts) {
            // 提取第一个可解析的数字作为时间（秒），最后一个布尔值作为播放状态
            double timeSec = 0.0;
            bool hasTime = false;
            bool isPlaying = false;
            qDebug() << "updatePlayheadTime"<<parts;
            for (const QVariant &p : parts) {
                const QString s = p.toString().trimmed();
                bool ok = false;
                const double v = s.toDouble(&ok);
                if (ok && !hasTime) {
                    timeSec = v;
                    hasTime = true;
                } else if (s.compare("true", Qt::CaseInsensitive) == 0) {
                    isPlaying = true;
                } else if (s.compare("false", Qt::CaseInsensitive) == 0) {
                    isPlaying = false;
                }
            }

            // if (hasTime && statusLabel) {
            //     const int ms = static_cast<int>(timeSec * 1000.0 + 0.5);
            //     QTime base(0, 0, 0, 0);
            //     statusLabel->setTime(base.addMSecs(ms));
            // }
            //
            // if (startButton && stopButton) {
            //     startButton->setChecked(isPlaying);
            //     stopButton->setChecked(!isPlaying);
            // }
        }
    public:
        QMLInterface *widget=new QMLInterface();
        shared_ptr<VariableData> inData;
        void *patch;
        QQuickWidget *m_quickWidget=new QQuickWidget();
    };
}