#pragma once

#include "Common/DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QQmlContext>
#include <QQuickItem>
#include <QtQuick/QQuickView>
#include <vector>
#include <QtCore/qglobal.h>
#include "PluginDefinition.hpp"
#include "CurveInterface.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include <QCheckBox>
#include <QFont>
#include <QGridLayout>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTime>
#include <QTimeEdit>
#include <QWidget>
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
    class CurveDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(bool start READ start WRITE setStart NOTIFY startChanged)
        Q_PROPERTY(bool loop READ loop WRITE setLoop NOTIFY loopChanged)

    Q_SIGNALS:
        /**
         * 函数级注释：播放状态属性变化通知（用于属性系统、外部控制与状态反馈）
         */
        void startChanged(bool start);

        /**
         * 函数级注释：循环状态属性变化通知（用于属性系统、外部控制与状态反馈）
         */
        void loopChanged(bool loop);

    public:
        CurveDataModel()
        {
            InPortCount =1;
            OutPortCount=3;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= true;

            m_interface = new CurveInterface();
            m_interface->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            m_view = new QQuickView();
            m_view->setResizeMode(QQuickView::SizeRootObjectToView);
            m_view->rootContext()->setContextProperty("CppBridge", this);
            m_view->setSource(QUrl("qrc:/qml/content/main.qml"));

            editorContainer = QWidget::createWindowContainer(m_view, m_interface.data());
            editorContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            m_interface->setEditorWidget(editorContainer);
            statusLabel = m_interface->statusLabel;

            connect(m_interface->startButton, &QPushButton::toggled, this, [this](bool checked) {
                setStart(checked);
            });
            connect(m_interface->loopCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
                setLoop(checked);
            });

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "start";
                AbstractDelegateModel::registerExternalBinding("/start", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "loop";
                AbstractDelegateModel::registerExternalBinding("/loop", this, b);
            }
        }

       // 函数级注释：析构函数
       // 说明：销毁节点时，安全释放 QQuickView 资源，防止悬挂指针。
       ~CurveDataModel() override {
            if (m_view && m_view->rootContext()) {
                m_view->rootContext()->setContextProperty("CppBridge", nullptr);
            }
            editorContainer = nullptr;
            m_view = nullptr;
            m_interface = nullptr;
            statusLabel = nullptr;
        }

    public:

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return "TRIGGER";
            case PortType::Out:
                switch (portIndex) {
                case 0:  return "PROGRESS";
                default: return QString("Curve%1").arg(portIndex);
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
            default: {
                // 安全读取：未赋值或越界时返回 0.0，避免崩溃
                const int idx = static_cast<int>(port) - 1;
                const double safeValue =
                    (idx >= 0 && idx < static_cast<int>(currentValuesY.size()))
                    ? currentValuesY[idx]
                    : 0.0;
                return std::make_shared<VariableData>(safeValue);
            }
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            if (!data) { return; }
            inData = std::dynamic_pointer_cast<VariableData>(data);
            setStart(inData ? inData->value().toBool() : false);
        }
        
        // 函数级注释：保存节点状态到 JSON。
        // 读取 QML 根项 Rectangle 的 savedData（JSON 字符串），若能解析为对象则写入 values.timeline；
        // 若解析失败则写入 values.timelineString，以便后续兼容恢复。
        QJsonObject save() const override
        {
            QJsonObject modelJson  = NodeDelegateModel::save();
            QJsonObject values;
    
            QString savedJson;
            if (m_view && m_view->rootObject()) {
                const QVariant v = m_view->rootObject()->property("savedData");
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
    
            // 函数级注释：保存循环播放开关到 JSON，以便恢复播放逻辑。
            values.insert("loop", loop());
    
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
    
            if (m_view && m_view->rootObject()) {
                QObject *root = m_view->rootObject();
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
            setLoopInternal(loopEnabled, true);
        }


        QWidget *embeddedWidget() override {
            return m_interface.data();
        }

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，使用完整地址接收播放与循环命令
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/start"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/loop"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

        /**
         * 函数级注释：获取播放状态属性
         */
        bool start() const { return m_start; }

        /**
         * 函数级注释：设置播放状态属性，驱动 QML 时间轴播放/暂停
         */
        void setStart(bool isChecked)
        {
            if (m_start == isChecked) {
                return;
            }
            m_start = isChecked;

            QObject *timeline = getTimelineEditor();
            if (timeline) {
                if (isChecked) {
                    QMetaObject::invokeMethod(timeline, "play");
                } else {
                    QMetaObject::invokeMethod(timeline, "pause");
                    QMetaObject::invokeMethod(timeline, "seek", Q_ARG(QVariant, QVariant(0.0)));
                }
            }

            if (m_interface) {
                m_interface->setPlayingState(m_start);
            }

            Q_EMIT startChanged(m_start);
        }

        /**
         * 函数级注释：获取循环状态属性
         */
        bool loop() const { return m_loop; }

        /**
         * 函数级注释：设置循环状态属性，并同步到 QML 时间轴
         */
        void setLoop(bool status)
        {
            setLoopInternal(status, true);
        }

        /**
         * 函数级注释：内部设置循环状态，可选择是否同步到 QML（用于从 QML 回调更新属性，避免循环）
         */
        void setLoopInternal(bool status, bool applyToTimeline)
        {
            if (m_loop == status) {
                return;
            }
            m_loop = status;

            if (applyToTimeline) {
                if (QObject *timeline = getTimelineEditor()) {
                    QMetaObject::invokeMethod(timeline, "setIsLoop", Q_ARG(QVariant, QVariant(status)));
                }
            }

            if (m_interface) {
                m_interface->setLoopState(m_loop);
            }

            Q_EMIT loopChanged(m_loop);
        }

        Q_INVOKABLE void updatePlayheadTime(const QVariantList &parts) {
            // 期望 parts 形如：["event.playheadTimeChange:", <timeSec>, <percent>, <isPlaying>]
            if (parts.size() < 5)
                return;

            bool okTime = false;
            const double timeSec = parts[1].toString().trimmed().toDouble(&okTime);
            const double valueX = parts[2].toString().trimmed().toDouble();
            if (parts[3].typeId() == QMetaType::QVariantList) {
                const QVariantList vl = parts[3].toList();
                currentValuesY.resize(vl.size());
                for (int i = 0; i < vl.size(); ++i) {
                    currentValuesY[i] = vl[i].toDouble();
                    Q_EMIT dataUpdated(i + 1);
                }
            }
            const QString statusStr = parts[4].toString().trimmed();
            const bool isPlaying = (statusStr.compare("true", Qt::CaseInsensitive) == 0);

            // 更新顶部 QTimeEdit 显示（将秒转为 mm:ss.zzz）
            if (okTime) {
                const int ms = static_cast<int>(timeSec * 1000.0 + 0.5);
                const QTime t = QTime(0, 0, 0, 0).addMSecs(ms);
                if (m_interface) {
                    m_interface->setPlayheadTime(t);
                }
                currentTime = std::make_shared<VariableData>(timeSec);
            } else {
                currentTime.reset();
            }
            currentTime = std::make_shared<VariableData>(valueX);
            Q_EMIT dataUpdated(0);

            if (currentStatus->value().toBool() != isPlaying) {
                currentStatus = std::make_shared<VariableData>(isPlaying);
                if (m_start != isPlaying) {
                    m_start = isPlaying;
                    if (m_interface) {
                        m_interface->setPlayingState(m_start);
                    }
                    Q_EMIT startChanged(m_start);
                }
            }
        }

        Q_INVOKABLE void updateLoop(const bool &status) {
            setLoopInternal(status, false);
        }

    public:
        QPointer<CurveInterface> m_interface;
        QPointer<QWidget> editorContainer;
        QPointer<QTimeEdit> statusLabel;
        std::shared_ptr<VariableData> inData;
        std::vector<double> currentValuesY=std::vector<double>();
        std::shared_ptr<VariableData> currentValue_Time=std::make_shared<VariableData>(0.0);
        std::shared_ptr<VariableData> currentTime=std::make_shared<VariableData>(0.0);
        std::shared_ptr<VariableData> currentStatus=std::make_shared<VariableData>(QVariant(false));
        QPointer<QQuickView> m_view;
    private:
        bool m_start = false;
        bool m_loop = false;

        // 函数级注释：通过 objectName 查找 QML 中的 TimelineEditor（main.qml 内 id: timelineEditor）
        QObject* getTimelineEditor() {
            if (!m_view) return nullptr;
            QObject *root = m_view->rootObject();
            if (!root) return nullptr;
            return root->findChild<QObject*>("timelineEditor");
        }

    private Q_SLOTS:
        /**
         * 函数级注释：处理来自全局事件总线的命令，控制播放与循环状态
         */
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString addrStart = makeFullOscAddress("/start");
            const QString addrLoop  = makeFullOscAddress("/loop");

            if (ev.address == addrStart) {
                const bool play = ev.payload.toBool();
                setStart(play);
            } else if (ev.address == addrLoop) {
                const bool loop = ev.payload.toBool();
                setLoop(loop);
            }
        }
    };
}
