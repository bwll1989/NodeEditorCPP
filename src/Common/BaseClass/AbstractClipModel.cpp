//
// Created by WuBin on 2026/10/30.
// AbstractClipModel 的库内实现。
//
// 说明：initPropertyWidget / showPropertyWidget 原先是头文件内联实现。
// 由于它们会直接构造 ClipTimePanel / ClipAppearancePanel 等 Q_OBJECT 窗口类，
// 宿主插件（.node/.plugin 动态库）在内联展开时会实例化这些面板类的
// tr() / staticMetaObject 等符号——其中 staticMetaObject 是静态数据符号，
// 无法通过导入库解析，导致插件链接失败（LNK2019）。
// 因此将实现移入库内。
//
// 【2026-11-10 剪裁→2026-11-12 恢复】上游 OSC 外部控制体系（控件地址注册/
// Ctrl 拖拽出 OSC mime/stateFeedBack 反向驱动）曾随原宿主工程退役剪裁；
// 2026-11-12 随 MediaServer 收编恢复（第五步新时间轴已接入 /timeline/master
// OSC 地址空间，剪裁前提失效）。现自旧 SDK AbstractClipModel/NodeClipDelegateModel
// 收编：registerExternalControl（StatusContainer 登记 + eventFilter Ctrl 拖拽）
// / makeFullOscAddress（m_oscAlias 默认 "/timeline/master"）/ stateFeedBack
//（GlobalEventBus::publishState）/ pushOscInitialState（setId 与
// initPropertyWidget 双触发）。mime 五元组按消费端读序写
//（host>>port>>address>>type>>value，OSCMessageListWidget::handleMimeDrop）。
//
// 【2026-11-12 收编 Common/BaseClass】随 BaseClass.dll 导出（SHARED），
// 导出宏换 BASECLASS_EXPORT；qrc 资源初始化归 timeline 库
//（BaseTimeLineWidget.cpp 的强制入链结构体），本文件不再涉及资源。
//

#include "AbstractClipModel.hpp"

// 面板类完整定义只在库内使用（头文件中仅为前置声明，见 AbstractClipModel.hpp）
#include "ClipTimePanel.hpp"
#include "ClipAppearancePanel.hpp"

// startDrag / pushOscInitialState 中 qobject_cast<IntDragValueWidget*> 需要控件完整类型，
// 仅库内使用（ClipTimePanel.hpp 已间接 include，显式声明依赖）
#include "IntDragValueWidget/IntDragValueWidget.hpp"
#include "Common/Devices/StatusContainer/StatusContainer.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Common/AppConfig/OscAddressMimeUtil.hpp"

#include <QVBoxLayout>
#include <QWidget>
#include <QTabWidget> // 三页式属性面板识别（qobject_cast<QTabWidget*>）
#include <QDrag>
#include <QPixmap>
#include <QComboBox>
#include <QAbstractButton>
#include <QAbstractSlider>

void AbstractClipModel::initPropertyWidget()
{
    m_standardPropertyWidget = new QWidget();
    m_layout = new QVBoxLayout(m_standardPropertyWidget);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);

    // 创建一个容器 widget 来放置主要内容
    auto contentWidget = new QWidget(m_standardPropertyWidget);
    auto contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(4);

    // 1. 时间属性组（重构后：由 ClipTimePanel 管理）+ 片段属性面板。
    // 【2026-11-13 三页式改版（用户定案）】适配器工厂若返回 QTabWidget
    // （ClipPropertyPanel 页容器：页 1 通用=媒体文件区），库通用面板
    // （时间属性/片段属性）插进第 1 页开头——页序：通用（时间/片段/文件）
    // → 动画 → 专属（用户定案「片段只要着重实现专属页，没有就不嵌入」）。
    // 工厂返回非 Tab 结构（旧式片段模型/退化路径）→ 保持原平铺不变。
    timeGroupBox = new ClipTimePanel(this, m_standardPropertyWidget);
    auto* appearancePanel = new ClipAppearancePanel(this, m_standardPropertyWidget);
    if (!m_clipPropertyWidget)
        m_clipPropertyWidget = clipPropertyWidget(); // 工厂闭包（适配器注入）
    if (auto* tabs = qobject_cast<QTabWidget*>(m_clipPropertyWidget);
        tabs && tabs->widget(0)) {
        if (auto* gl = qobject_cast<QBoxLayout*>(tabs->widget(0)->layout())) {
            gl->insertWidget(0, timeGroupBox);      // 时间属性（页首）
            gl->insertWidget(1, appearancePanel);   // 片段属性
        } else { // 页布局类型不符（协议断裂防御）——兜底平铺
            m_layout->addWidget(timeGroupBox);
            m_layout->addWidget(appearancePanel);
        }
    } else {
        m_layout->addWidget(timeGroupBox);
        m_layout->addWidget(appearancePanel);
    }

    // 【2026-11-12 恢复】时间属性 6 控件登记 OSC 外部控制（相对地址），
    // 随后统一推送初始状态（StatusContainer 登记 + 初始值反馈）。
    // 此处为双触发之一（另一处在 setId——控件通常先于/晚于 setId 创建均覆盖）。
    registerExternalControl(QStringLiteral("/start_frame"), timeGroupBox->m_startFrameSpinBox);
    registerExternalControl(QStringLiteral("/end_frame"), timeGroupBox->m_endFrameSpinBox);
    registerExternalControl(QStringLiteral("/position_frame"), timeGroupBox->m_positionFrameSpinBox);
    registerExternalControl(QStringLiteral("/start_time_code"), timeGroupBox->m_startTimeCodeLineEdit);
    registerExternalControl(QStringLiteral("/end_time_code"), timeGroupBox->m_endTimeCodeLineEdit);
    registerExternalControl(QStringLiteral("/position_time_code"), timeGroupBox->m_positionTimeLineEdit);
    pushOscInitialState();

    // 添加代理编辑器的占位符（三页式时 = QTabWidget 页容器）
    if (m_clipPropertyWidget)
        contentLayout->addWidget(m_clipPropertyWidget);

    // 将内容 widget 添加到主布局
    m_layout->addWidget(contentWidget);
    m_standardPropertyWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
}

void AbstractClipModel::showPropertyWidget()
{
    if (m_clipPropertyWidget) {
        m_standardPropertyWidget->show();
    } else {
        initPropertyWidget();
        m_standardPropertyWidget->show();
    }
}

// ============================================================================
// OSC 外部控制体系（2026-11-12 恢复，自旧 SDK TimelineEngine 收编）
// ============================================================================

// 函数说明：登记控件 OSC 相对地址 → 控件映射，并安装 Ctrl+左键拖拽事件过滤器。
// 实现说明：与旧 SDK AbstractClipModel::registerExternalControl 同款——地址必须
// 以 "/" 开头；同地址重复登记先删旧条目；installEventFilter + setMouseTracking
// 使 Ctrl+左键拖动可经 eventFilter 触发 startDrag。QPointer 存储防控件销毁悬空。
void AbstractClipModel::registerExternalControl(const QString& oscAddress, QWidget* control)
{
    if (!oscAddress.startsWith('/')) return;
    if (!control) return;
    auto it = _OscMapping.find(oscAddress);
    if (it != _OscMapping.end())
        _OscMapping.erase(it);
    control->installEventFilter(this);
    control->setMouseTracking(true);
    _OscMapping[oscAddress] = control;
}

// 函数说明：相对地址 → 完整 OSC 地址（/<oscAlias>/<clipId><rel>）。
// 实现说明：旧 SDK 从 timeline model 的 TimelineModelAliasRole 取别名，收编版
// TimeLineRoles 已无该 role——改自持 m_oscAlias（默认 "/timeline/master"，与
// 新时间轴宿主 MainWindow 的 msTimelineAddress 前缀一致，外部脚本零迁移）。
QString AbstractClipModel::makeFullOscAddress(const QString& oscAddress) const
{
    const QString norm = oscAddress.startsWith('/') ? oscAddress : ('/' + oscAddress);
    return m_oscAlias + '/' + QString::number(m_id) + norm;
}

// 函数说明：经 GlobalEventBus 发布状态反馈（→ StatusContainer.updateState →
// statusUpdated → ExternalControler OSC feedback / WebSocket / MQTT 出站）。
void AbstractClipModel::stateFeedBack(const QString& oscAddress, QVariant value)
{
    if (m_oscAlias.isEmpty()) return;
    GlobalEventBus::instance()->publishState(makeFullOscAddress(oscAddress), value);
}

// 函数说明：ID/控件就绪后的统一推送点——向 StatusContainer 登记完整地址
//（注册表即 OSC 地址树索引）+ 订阅命令入站（onOscCommand 反驱动），并按控件
// 类型推初始值（与旧 SDK NodeClipDelegateModel::onModelReady 同款 cast 链；
// QTimeEdit 无对应分支不推初始值，同旧行为）。
// 幂等：registerWidget 地址已存在仅更新指针，subscribe 同对象同地址同槽去重。
void AbstractClipModel::pushOscInitialState()
{
    for (auto it = _OscMapping.begin(); it != _OscMapping.end(); ++it) {
        const QString& rel = it->first;
        QWidget* control = it->second.data();
        if (!control) continue;
        const QString full = makeFullOscAddress(rel);
        StatusContainer::instance()->registerWidget(control, full);
        // 命令入站订阅：外部 OSC → onOscCommand 反驱动控件。
        GlobalEventBus::instance()->subscribe(full, this, SLOT(onOscCommand(GlobalEvent)));

        if (auto* button = qobject_cast<QAbstractButton*>(control)) {
            if (button->isCheckable())
                stateFeedBack(rel, QVariant(button->isChecked()));
        } else if (auto* slider = qobject_cast<QAbstractSlider*>(control)) {
            stateFeedBack(rel, QVariant(slider->value()));
        } else if (auto* spinBox = qobject_cast<QSpinBox*>(control)) {
            stateFeedBack(rel, QVariant(spinBox->value()));
        } else if (auto* dragInt = qobject_cast<IntDragValueWidget*>(control)) {
            stateFeedBack(rel, QVariant(dragInt->value()));
        } else if (auto* lineEdit = qobject_cast<QLineEdit*>(control)) {
            stateFeedBack(rel, QVariant(lineEdit->text()));
        } else if (auto* comboBox = qobject_cast<QComboBox*>(control)) {
            stateFeedBack(rel, QVariant(comboBox->currentIndex()));
        }
    }
}

// 函数说明：OSC 控件上的 Ctrl+左键拖拽检测（旧 SDK AbstractClipModel::eventFilter
// 同款状态机：按下记起点 → 移动超 startDragDistance 触发 startDrag 并吃掉事件 →
// 释放复位）。非 OSC 控件/非 Ctrl 事件一律 return false 零拦截。
bool AbstractClipModel::eventFilter(QObject* watched, QEvent* event)
{
    for (const auto& pair : _OscMapping) {
        if (pair.second.data() != watched)
            continue;
        QWidget* widget = pair.second.data();
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton && (mouseEvent->modifiers() & Qt::ControlModifier)) {
                    m_dragStartPos = mouseEvent->pos();
                    m_oscDragging = true;
                }
                break;
            }
            case QEvent::MouseMove: {
                if (!m_oscDragging) break;
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                if ((mouseEvent->pos() - m_dragStartPos).manhattanLength()
                    >= QApplication::startDragDistance()) {
                    startDrag(widget);
                    m_oscDragging = false;
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonRelease:
                m_oscDragging = false;
                break;
            default:
                break;
        }
        break;
    }
    return false;
}

// 函数说明：OSC 命令入站反驱动（见 hpp 槽注释）。按 ev.address 反查控件，
// 按类型 setValue——控件信号照常传播（IntDragValueWidget::valueChanged →
// ClipTimePanel::updateStartFrame → 模型 setStart(framesToUs)），形成
// OSC → UI → 模型 → OSC 反馈 的完整闭环。
void AbstractClipModel::onOscCommand(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) return;
    for (const auto& pair : _OscMapping) {
        QWidget* control = pair.second.data();
        if (!control || makeFullOscAddress(pair.first) != ev.address)
            continue;
        if (auto* dragInt = qobject_cast<IntDragValueWidget*>(control)) {
            dragInt->setValue(ev.payload.toInt());
        } else if (auto* spinBox = qobject_cast<QSpinBox*>(control)) {
            spinBox->setValue(ev.payload.toInt());
        } else if (auto* slider = qobject_cast<QAbstractSlider*>(control)) {
            slider->setValue(ev.payload.toInt());
        } else if (auto* button = qobject_cast<QAbstractButton*>(control)) {
            if (button->isCheckable())
                button->setChecked(ev.payload.toBool());
        } else if (auto* comboBox = qobject_cast<QComboBox*>(control)) {
            comboBox->setCurrentIndex(ev.payload.toInt());
        } else if (auto* lineEdit = qobject_cast<QLineEdit*>(control)) {
            lineEdit->setText(ev.payload.toString());
        }
        break;
    }
}

// 函数说明：把 OSC 控件拖拽数据打包为 mime 并启动拖拽（Ctrl+左键拖动控件时触发）。
// 实现说明：qobject_cast<IntDragValueWidget*> 需要控件完整类型（GUIElements 库），
// 故留在库内 .cpp。地址 = makeFullOscAddress(rel)，同时复制到剪贴板。
// mime 编解码走 OscAddressMimeUtil（全工程统一：消费端字段序 host>>port>>
// address>>type>>value）。host/port 为本机控制端口的提示值——BaseClass 不链
// AppConfig，硬编码与 ConfigManager 默认值（127.0.0.1:8991）保持一致。
void AbstractClipModel::startDrag(QWidget* widget)
{
    QString rel;
    for (const auto& pair : _OscMapping) {
        if (pair.second.data() == widget) {
            rel = pair.first;
            break;
        }
    }
    if (rel.isEmpty()) return;

    const QString address = makeFullOscAddress(rel);
    QApplication::clipboard()->setText(address);

    // 获取控件的值（与旧 SDK startDrag 同款 cast 链；QTimeEdit 落默认空 String）
    QVariant value;
    QString type = QStringLiteral("String");
    if (auto* button = qobject_cast<QAbstractButton*>(widget)) {
        value = button->isChecked();
        type = QStringLiteral("Int");
    } else if (auto* slider = qobject_cast<QAbstractSlider*>(widget)) {
        value = slider->value();
        type = QStringLiteral("Int");
    } else if (auto* spinBox = qobject_cast<QSpinBox*>(widget)) {
        value = spinBox->value();
        type = QStringLiteral("Int");
    } else if (auto* dragInt = qobject_cast<IntDragValueWidget*>(widget)) {
        value = dragInt->value();
        type = QStringLiteral("Int");
    } else if (auto* lineEdit = qobject_cast<QLineEdit*>(widget)) {
        value = lineEdit->text();
        type = QStringLiteral("String");
    } else if (auto* label = qobject_cast<QLabel*>(widget)) {
        value = label->text();
        type = QStringLiteral("String");
    }

    auto* mimeData = new QMimeData;
    OscAddressMime::encode(mimeData, QStringLiteral("127.0.0.1"), 8991,
                           address, type, value);

    QDrag* drag = new QDrag(widget);
    drag->setMimeData(mimeData);
    drag->setPixmap(OscAddressMime::addressPixmap(address));
    drag->setHotSpot(QPoint(100, 15));
    drag->exec(Qt::CopyAction);
}

