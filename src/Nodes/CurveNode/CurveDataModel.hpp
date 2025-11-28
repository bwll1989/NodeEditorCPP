#pragma once

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QQmlContext>
#include <QQuickItem>
#include <vector>
#include <QtCore/qglobal.h>
#include "PluginDefinition.hpp"
#include "ConstantDefines.h"
#include "CurveInterface.hpp"
#include "OSCSender/OSCSender.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
    class CurveDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        CurveDataModel()
        {
            InPortCount =1;
            OutPortCount=4;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
            // m_quickWidget->setSource(QUrl("qrc:qml/content/dynamicview.qml"));
            m_quickWidget->setSource(QUrl("qrc:/qml/content/main.qml"));
            m_quickWidget->rootContext()->setContextProperty("CppBridge", this);
            NodeDelegateModel::registerOSCControl("/start",widget->startButton);
            // NodeDelegateModel::registerOSCControl("/stop",widget->stopButton);
            NodeDelegateModel::registerOSCControl("/loop",widget->loopCheckBox);
            connect(widget->editButton, &QPushButton::clicked, this, &CurveDataModel::toggleEditorMode);
            connect(widget->startButton, &QPushButton::clicked, this, &CurveDataModel::onStartButtonClicked);
            // connect(widget->stopButton,  &QPushButton::clicked, this, &QMLDataModel::onStopButtonClicked);
            connect(widget->loopCheckBox, &QCheckBox::clicked, this, &CurveDataModel::onLoopCheckBoxClicked); // 绑定循环播放复选框点击事件
        }

       ~CurveDataModel() override {
            QObject *timeline = getTimelineEditor();
            if (timeline) {
                QMetaObject::invokeMethod(timeline, "pause");
            }
            if (m_quickWidget) {
                m_quickWidget->setParent(nullptr);
                m_quickWidget->deleteLater();
            }
        }

    public:

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return "TRIGGER";
            case PortType::Out:
                switch (portIndex) {
                case 0:  return "TIME";
                case 1:  return "VALUE_X";
                case 2:  return "VALUE_Y";
                case 3:  return "STATUS";
                default: break;
                }
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
            switch (port) {
            case 0: return currentTime;
            case 1: return currentValue_X;
            case 2:return currentValue_Y;
            case 3: return currentStatus;
            default: return nullptr;
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            if (!data) { return; }
            inData = std::dynamic_pointer_cast<VariableData>(data);

            widget->startButton->clicked(inData->value().toBool());

        }
        
        // 函数级注释：保存节点状态到 JSON。
        // 读取 QML 根项 Rectangle 的 savedData（JSON 字符串），若能解析为对象则写入 values.timeline；
        // 若解析失败则写入 values.timelineString，以便后续兼容恢复。
        QJsonObject save() const override
        {
            QJsonObject modelJson  = NodeDelegateModel::save();
            QJsonObject values;
    
            QString savedJson;
            if (m_quickWidget && m_quickWidget->rootObject()) {
                const QVariant v = m_quickWidget->rootObject()->property("savedData");
                if (v.isValid())
                    savedJson = v.toString();
            }
    
            if (!savedJson.isEmpty()) {
                QJsonParseError err;
                const QJsonDocument doc = QJsonDocument::fromJson(savedJson.toUtf8(), &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    values.insert("timeline", doc.object());
                } else {
                    values.insert("timelineString", savedJson);
                }
            }
    
            // 函数级注释：保存循环播放开关到 JSON，以便恢复 UI 与播放逻辑。
            values.insert("loop", widget->loopCheckBox->isChecked());
    
            modelJson.insert("values", values);
            return modelJson;
        }

        // 函数级注释：从持久化 JSON 加载时间轴。
        // 优先读取 values.timeline（对象），否则回退到 values.timelineString（字符串）；
        // 通过 QML 的 WebEngineView.runJavaScript 注入到页面，并同步根项 savedData。
        // 函数级注释：从持久化 JSON 加载时间轴
        // 写入根项 savedData，并尝试在 timeline 已定义时立即导入；
        // 页面未就绪时由 QML 的 onLoadingChanged / onSavedDataChanged 负责后续导入。
        void load(const QJsonObject &p) override
        {
            const QJsonValue vv = p.value("values");
            if (vv.isUndefined() || !vv.isObject())
                return;
    
            const QJsonObject values = vv.toObject();
            QString js;       // 要执行的导入脚本（不带防护）
            QString savedStr; // 写回根项的 JSON 字符串
    
            if (values.contains("timeline") && values.value("timeline").isObject()) {
                const QJsonDocument doc(values.value("timeline").toObject());
                const QString jsonCompact = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
                js = QStringLiteral("timeline.importData(%1);").arg(jsonCompact);
                savedStr = jsonCompact;
            } else if (values.contains("timelineString") && values.value("timelineString").isString()) {
                const QString jsonStr = values.value("timelineString").toString();
                // 直接作为字符串保存，QML 端用模板字符串注入
                savedStr = jsonStr;
                // 若需要立即注入，使用 JSON.parse 方式更稳妥
                QString escaped = jsonStr;
                escaped.replace("\\", "\\\\").replace("'", "\\'");
                js = QStringLiteral("timeline.importData(JSON.parse('%1'));").arg(escaped);
            }
    
            if (m_quickWidget && m_quickWidget->rootObject()) {
                QObject *root = m_quickWidget->rootObject();
                // 写入 savedData，供 QML 侧在页面就绪或属性变更时自动导入
                root->setProperty("savedData", savedStr);
    
             
            }
            bool loopEnabled = false;
            if (values.contains("loop")) {
                const QJsonValue lv = values.value("loop");
                if (lv.isBool()) {
                    loopEnabled = lv.toBool();
                } else if (lv.isString()) {
                    loopEnabled = lv.toString().compare("true", Qt::CaseInsensitive) == 0;
                } else if (lv.isDouble()) {
                    loopEnabled = (lv.toInt() != 0);
                }
            }
            widget->loopCheckBox->setChecked(loopEnabled);
            if (QObject *timeline = getTimelineEditor()) {
                QMetaObject::invokeMethod(timeline, "setIsLoop", Q_ARG(QVariant, QVariant(loopEnabled)));
            }
        }


        QWidget *embeddedWidget() override{

            return widget;
        }
    public Q_SLOTS:
        void toggleEditorMode() {
            // 移除父子关系，使其成为独立窗口
            m_quickWidget->setParent(nullptr);

            // 设置为独立窗口
            m_quickWidget->setWindowTitle("Curve编辑器");

            // 设置窗口图标
            m_quickWidget->setWindowIcon(QIcon(":/icons/icons/curve.png"));

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

        // 函数级注释：开始按钮槽函数，直接调用 QML 的 TimelineEditor.play() 启动播放
        void onStartButtonClicked(bool isChecked) {
            QObject *timeline = getTimelineEditor();
            if (!timeline) return;
            if (isChecked)
                QMetaObject::invokeMethod(timeline, "play");
            else {
                QMetaObject::invokeMethod(timeline, "pause");
                QMetaObject::invokeMethod(timeline, "seek", Q_ARG(QVariant, QVariant(0.0)));
            }
        }


        // 函数级注释：循环播放复选框槽函数，直接调用 QML 的 TimelineEditor.isLoop 切换循环播放状态
        void onLoopCheckBoxClicked() {
            QObject *timeline = getTimelineEditor();
            if (!timeline) return;
            QMetaObject::invokeMethod(timeline, "setIsLoop", Q_ARG(QVariant, QVariant(widget->loopCheckBox->isChecked())));
        }

        Q_INVOKABLE void updatePlayheadTime(const QVariantList &parts) {
            // 期望 parts 形如：["event.playheadTimeChange:", <timeSec>, <percent>, <isPlaying>]
            if (parts.size() < 5)
                return;

            bool okTime = false;
            const double timeSec = parts[1].toString().trimmed().toDouble(&okTime);
            const double valueX = parts[2].toString().trimmed().toDouble();
            const double valueY = parts[3].toString().trimmed().toDouble();
            const QString statusStr = parts[4].toString().trimmed();
            const bool isPlaying = (statusStr.compare("true", Qt::CaseInsensitive) == 0);

            // 更新顶部 QTimeEdit 显示（将秒转为 mm:ss.zzz）
            if (okTime) {
                const int ms = static_cast<int>(timeSec * 1000.0 + 0.5);
                QTime base(0, 0, 0, 0);
                widget->statusLabel->setTime(base.addMSecs(ms));
                currentTime = make_shared<VariableData>(timeSec);
            } else {
                currentTime.reset();
            }
            Q_EMIT dataUpdated(0);
            // 更新当前百分比
            currentValue_Y = make_shared<VariableData>(valueY);
            currentValue_X = make_shared<VariableData>(valueX);
            Q_EMIT dataUpdated(1);
            Q_EMIT dataUpdated(2);
            if (currentStatus->value().toBool() != isPlaying) {
                currentStatus = make_shared<VariableData>(isPlaying);
                updateStatus(isPlaying);
                Q_EMIT dataUpdated(3);
            }
        }

        Q_INVOKABLE void updateLoop(const bool &status) {

            widget->loopCheckBox->setChecked(status);
        }
        void updateStatus(const bool &status) {

            if ( widget->startButton->isChecked() != status) {
                widget->startButton->setChecked(status);
                // widget->stopButton->setChecked(!status);
            }
            widget->startButton->setText(status ? "Stop" : "Play");
        }
        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }
    public:
        CurveInterface *widget=new CurveInterface();
        shared_ptr<VariableData> inData;
        shared_ptr<VariableData> currentValue_Y=make_shared<VariableData>(0.0);
        shared_ptr<VariableData> currentValue_X=make_shared<VariableData>(0.0);
        shared_ptr<VariableData> currentTime=make_shared<VariableData>(0.0);
        shared_ptr<VariableData> currentStatus=make_shared<VariableData>(QVariant(false));
        void *patch;
        QQuickWidget *m_quickWidget=new QQuickWidget();
    private:
        // 函数级注释：通过 objectName 查找 QML 中的 TimelineEditor（main.qml 内 id: timelineEditor）
        QObject* getTimelineEditor() {
            if (!m_quickWidget) return nullptr;
            QObject *root = m_quickWidget->rootObject();
            if (!root) return nullptr;
            return root->findChild<QObject*>("timelineEditor");
        }
    };
}