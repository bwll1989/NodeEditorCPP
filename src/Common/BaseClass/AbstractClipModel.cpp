//
// Created by WuBin on 2026/10/30.
// AbstractClipModel 的库内实现。
//
// 说明：initPropertyWidget / showPropertyWidget 原先是头文件内联实现。
// 由于它们会直接构造 Q_OBJECT 窗口类（历史上为 ClipTimePanel /
// ClipAppearancePanel，【P3 面板迁库 2026-11-17】已迁 ClipPanelBase），
// 宿主插件（.node/.plugin 动态库）在内联展开时会实例化这些窗口类的
// tr() / staticMetaObject 等符号——其中 staticMetaObject 是静态数据符号，
// 无法通过导入库解析，导致插件链接失败（LNK2019）。
// 因此实现留在库内（现仅构造 FramelessDialog 壳 + 嵌入工厂产物）。
//
// 【2026-11-10 剪裁→2026-11-12 恢复】上游 OSC 外部控制体系（控件地址注册/
// Ctrl 拖拽出 OSC mime/stateFeedBack 反向驱动）曾随原宿主工程退役剪裁；
// 2026-11-12 随 MediaServer 收编恢复（第五步新时间轴已接入 /timeline/master
// OSC 地址空间，剪裁前提失效）。现自旧 SDK AbstractClipModel/NodeClipDelegateModel
// 收编：registerExternalControl（StatusContainer 登记 + eventFilter Ctrl 拖拽）
// / makeFullOscAddress（m_oscAlias 默认 "/timeline/master"）/ stateFeedBack
//（GlobalEventBus::publishState）/ pushOscInitialState（setId 与
// ClipTimePanel 构造自治登记【P3 后】双触发）。mime 五元组按消费端读序写
//（host>>port>>address>>type>>value，OSCMessageListWidget::handleMimeDrop）。
//
// 【2026-11-12 收编 Common/BaseClass】随 BaseClass.dll 导出（SHARED），
// 导出宏换 BASECLASS_EXPORT；qrc 资源初始化归 timeline 库
//（BaseTimeLineWidget.cpp 的强制入链结构体），本文件不再涉及资源。
//

#include "AbstractClipModel.hpp"

// 【P3 面板迁库 2026-11-17】原 include 的 ClipTimePanel.hpp /
// ClipAppearancePanel.hpp 随面板迁出（→ ClipPanelBase）——本库回归零 UI，
// 面板装配收拢进 ClipPropertyPanel，本类只装配壳与工厂产物。

// startDrag / pushOscInitialState 中 qobject_cast<IntDragValueWidget*> 需要控件完整类型，
// 仅库内使用（原经 ClipTimePanel.hpp 间接 include，迁库后显式声明依赖）
#include "IntDragValueWidget/IntDragValueWidget.hpp"
#include "Common/Devices/StatusContainer/StatusContainer.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Common/AppConfig/OscAddressMimeUtil.hpp"
#include "FramelessDialog/FramelessDialog.hpp" // 属性弹窗宿主（统一无边框风格）

#include <QVBoxLayout>
#include <QWidget>
#include <QMetaProperty> // 属性绑定：notifySignalIndex/hasNotifySignal（qobjectdefs.h 仅前置声明）
#include <QDrag>
#include <QPixmap>
#include <QComboBox>
#include <QAbstractButton>
#include <QAbstractSlider>

// 【P3 面板迁库 2026-11-17】原 include "ClipPageHost.h"（P2 页容器装配
// 接口）随装配收拢退役——通用区装配内化进 ClipPropertyPanel，无需再协商。

#include <QDebug>
#include <QElapsedTimer> // panel-timing 排查埋点（临时）

void AbstractClipModel::initPropertyWidget()
{
    // 【panel-timing 2026-11-17】双击卡顿排查埋点（临时）：分阶段耗时落
    // qDebug（对接 spdlog）。定位后移除。
    QElapsedTimer _pt;
    _pt.start();
    // 【2026-11-17 统一无边框弹窗】宿主从裸 QWidget（Qt::WindowStaysOnTopHint
    // 置顶、无标题栏、无关闭钮）改为 FramelessDialog（GUIElements 统一弹窗
    // 基类：标题栏拖拽 / Esc / 标题栏关闭钮 / 统一 QSS）。非模态交互不变
    //（FramelessDialog 构造默认 modal，此处显式关闭——面板开着仍可操作
    // 画布/时间轴，与原版一致）。
    auto* prop_dialog = new FramelessDialog();
    m_standardPropertyWidget = prop_dialog;
    prop_dialog->setModal(false);
    prop_dialog->setCloseButtonVisible(false); // 标题栏 × = 关闭面板
    prop_dialog->setDialogTitle(displayText()); // 备注名优先，类型兜底
    QVBoxLayout* body_layout = prop_dialog->bodyLayout();
    body_layout->setSpacing(4);
    const qint64 _t_dialog = _pt.elapsed();

    // 【P3 面板迁库（2026-11-17）】本函数瘦身为「壳 + 工厂 + 嵌入」：
    //   * 壳 = FramelessDialog（统一无边框风格，非模态，置顶语义保留）；
    //   * 内容 = 工厂闭包产物（适配器注入的 ClipPropertyPanel 三页式页
    //     容器——通用区时间/片段/媒体 + 动画页 + 专属页全在页容器内自治；
    //     库通用面板 ClipTimePanel/ClipAppearancePanel 迁 ClipPanelBase，
    //     经面板构造参数 owner_model 自装配进页 1，OSC 外控登记随面板构造
    //     自治——原本函数的 6× registerExternalControl + pushOscInitialState
    //     块随之退役）；
    //   * 退化路径行为变化：工厂未返回产物（hooks 未注入/工作副本缺片段）
    //     时原平铺库通用面板兜底——面板迁出本库后无 UI 可兜底，对话框
    //     空壳留痕（qWarning）。
    if (!m_clipPropertyWidget)
        m_clipPropertyWidget = clipPropertyWidget(); // 工厂闭包（适配器注入）
    if (!m_clipPropertyWidget)
        qWarning() << "[clip-panel] 属性面板工厂未返回产物（宿主未注入或"
                      "片段缺位）——对话框空壳";
    if (m_clipPropertyWidget)
        body_layout->addWidget(m_clipPropertyWidget);

    // 置顶语义保留（原 Qt::WindowStaysOnTopHint）：叠加在 FramelessDialog
    // 既有 flags（Qt::Dialog|FramelessWindowHint）之上，勿整体覆盖。
    prop_dialog->setWindowFlags(prop_dialog->windowFlags() |
                                Qt::WindowStaysOnTopHint);
    qDebug() << "[panel-timing] initPropertyWidget 总" << _pt.elapsed()
             << "ms（壳" << _t_dialog << "+ 工厂/装配"
             << (_pt.elapsed() - _t_dialog) << "）";
}

void AbstractClipModel::showPropertyWidget()
{
    if (!m_clipPropertyWidget)
        initPropertyWidget();
    // 标题随显示刷新（id/备注名可能晚于面板创建才就绪）
    if (auto* dlg = qobject_cast<FramelessDialog*>(m_standardPropertyWidget))
        dlg->setDialogTitle(displayText());
    m_standardPropertyWidget->show();
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
    if (it != _OscMapping.end()) {
        // 换绑：解绑旧控件的拖拽过滤器（QPointer 悬空安全，此处仍存活才移除）
        if (it->second.data())
            it->second.data()->removeEventFilter(this);
        _OscMapping.erase(it);
    }
    control->installEventFilter(this);
    control->setMouseTracking(true);
    _OscMapping[oscAddress] = control;
    // 【对齐 QtNodes 底座语义（外部控制审查 2026-11-15）】控件销毁自动清条目。
    // QPointer 只保证悬空安全，条目本身会残留在 _OscMapping（onOscCommand/
    // pushOscInitialState 靠判空跳过）——显式清理保持映射精确；this 析构时
    // 本连接随 QObject 机制自动断开。
    connect(control, &QObject::destroyed, this, [this, oscAddress]() {
        _OscMapping.erase(oscAddress);
    });
}

// 函数说明：登记 OSC 属性绑定（rel → 模型 Q_PROPERTY 名；2026-11-15 对齐
// 节点 registerExternalBinding 语义）。实现说明：feedback=true 时连接属性
// NOTIFY 信号到 onOscPropertyChanged（变化自动出站，节点 onExternalCommandNotified
// 同款 senderSignalIndex 反查模式）；重登记断旧连新。rel 勿与控件绑定重叠
//（onOscCommand 控件优先）。属性不存在时仅登记不连接——推送期 property()
// 返回无效 QVariant 自然跳过（防御子类笔误，不崩溃）。
void AbstractClipModel::registerExternalProperty(const QString& oscAddress, const QString& member)
{
    if (!oscAddress.startsWith('/') || member.isEmpty()) return;
    ClipPropertyBinding& b = _OscPropertyMapping[oscAddress];
    b.member = member;
    if (b.notifyConnection) {
        disconnect(b.notifyConnection);
        b.notifyConnection = QMetaObject::Connection{};
    }
    const QMetaObject* mo = metaObject();
    const int pi = mo->indexOfProperty(member.toUtf8().constData());
    if (pi < 0) return; // 属性缺失：仅登记（推送期自然跳过），不连接
    const QMetaProperty prop = mo->property(pi);
    if (!prop.hasNotifySignal()) return; // 无 NOTIFY：反馈走子类手动 stateFeedBack
    const int sig = prop.notifySignalIndex();
    const int slot = metaObject()->indexOfSlot("onOscPropertyChanged()");
    if (sig >= 0 && slot >= 0)
        b.notifyConnection = QMetaObject::connect(this, sig, this, slot, Qt::AutoConnection);
}

// 函数说明：注销属性绑定（断 NOTIFY 连接 + 删条目）。
void AbstractClipModel::unregisterExternalProperty(const QString& oscAddress)
{
    auto it = _OscPropertyMapping.find(oscAddress);
    if (it == _OscPropertyMapping.end()) return;
    if (it->second.notifyConnection)
        disconnect(it->second.notifyConnection);
    _OscPropertyMapping.erase(it);
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
// 【2026-11-15 属性绑定】同时遍历 _OscPropertyMapping：订阅命令入站 +
// property(member) 读当前值推送——属性绑定不依赖控件存在，片段创建
//（setId）即推，与节点 onModelReady 批量推送语义对齐。
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

    // 属性绑定：订阅 + 推当前值（模型 Q_PROPERTY 经 property() 读取；与
    // 控件绑定同款幂等语义）。属性值经 stateFeedBack 进 StatusContainer
    //（updateState 建地址条目），无需 registerWidget（无控件可挂）。
    for (const auto& pair : _OscPropertyMapping) {
        const QString& rel = pair.first;
        const ClipPropertyBinding& b = pair.second;
        if (b.member.isEmpty()) continue;
        const QString full = makeFullOscAddress(rel);
        GlobalEventBus::instance()->subscribe(full, this, SLOT(onOscCommand(GlobalEvent)));
        const QVariant v = property(b.member.toUtf8().constData());
        if (v.isValid())
            stateFeedBack(rel, v);
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
// 【2026-11-15 属性绑定】控件映射未命中时反查 _OscPropertyMapping：
// setProperty 走 Q_PROPERTY WRITE（子类业务 setter，如写 Asset 域）——
// 面板未打开时外部控制同样可达（节点语义：控件只是 UI，命令落模型）。
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
        return;
    }
    // 属性绑定分支：rel 反查 → setProperty（QVariant 自动向属性类型转换；
    // 无 NOTIFY 的属性由子类 setter 内手动 stateFeedBack）
    for (const auto& pair : _OscPropertyMapping) {
        if (pair.second.member.isEmpty()) continue;
        if (makeFullOscAddress(pair.first) != ev.address) continue;
        setProperty(pair.second.member.toUtf8().constData(), ev.payload);
        return;
    }
}

// 函数说明：已登记 Q_PROPERTY 的 NOTIFY 触发（registerExternalProperty 连接）：
// 按 senderSignalIndex 反查绑定并 stateFeedBack——子类属性变化自动出站，
// 与节点 onExternalCommandNotified 同款模式，零反馈代码。
void AbstractClipModel::onOscPropertyChanged()
{
    const int sig = senderSignalIndex();
    if (sig < 0) return;
    const QMetaObject* mo = metaObject();
    for (const auto& pair : _OscPropertyMapping) {
        const ClipPropertyBinding& b = pair.second;
        if (!b.feedback || b.member.isEmpty()) continue;
        const int pi = mo->indexOfProperty(b.member.toUtf8().constData());
        if (pi < 0) continue;
        if (mo->property(pi).notifySignalIndex() != sig) continue;
        stateFeedBack(pair.first, property(b.member.toUtf8().constData()));
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

