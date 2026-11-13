// ============================================================================
//  ControlClipSection 实现 —— Control 片段指令编辑区（列表 + 测试发送）
//
//  交互约定（与 TextClipSection 同源，写回目标为片段域）：
//    * 写回 = 800ms 轮询 diff（可见时）+ 测试发送前强制落定——列表控件
//      无内容变化信号，diff 快照是唯一真值判据，同时天然防广播回环；
//    * 触发发送 = StatusContainer::instance()->parseOSC（与旧 Trigger
//      的 trigger() 同一入口）；测试发送按钮 = 手动执行同一动作；
//    * onlyInternal=true（与旧 Trigger 一致）：卡片隐藏 host:port，
//      消息固定走本地状态容器（host/port 不入数据）。
// ============================================================================

#include "ControlClipSection.hpp"

#include <QHBoxLayout>
#include <QEvent>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

#include "OscListWidget/OSCMessageListWidget.hpp" // OscListWidget 库
#include "ControlMessages.hpp"                   // ms::fireControlMessages（与触发引擎同源）

namespace ms {

// ---------------------------------------------------------------------------
// 构造：控件树 + 预填 + 轮询启动（预填在轮询启动之前——首帧 diff 无变化）
// ---------------------------------------------------------------------------
ControlClipSection::ControlClipSection(const ClipSectionContext& ctx,
                                       QWidget* parent)
    : QWidget(parent), ctx_(ctx) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(4);

    auto* box = new QGroupBox(QStringLiteral("指令"), this);
    auto* lay = new QVBoxLayout(box);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->setSpacing(4);

    // ---- 触发语义说明 --------------------------------------------------------
    hint_ = new QLabel(
        QStringLiteral("播放头进入片段时按顺序发送以下指令（上升沿触发一次）"), box);
    hint_->setObjectName("controlHint");
    hint_->setStyleSheet(QStringLiteral("color:#888;"));
    hint_->setWordWrap(true);
    lay->addWidget(hint_);

    // ---- 指令列表（OSCMessageListWidget：增删/编辑/右键菜单全内置）----------
    list_ = new OSCMessageListWidget(/*onlyInternal=*/true, box);
    list_->setObjectName("controlMessages");
    list_->setMinimumHeight(96);
    lay->addWidget(list_);

    // ---- 测试发送 ------------------------------------------------------------
    auto* btn_row = new QHBoxLayout();
    btn_row->addStretch(1);
    send_btn_ = new QPushButton(QStringLiteral("测试发送"), box);
    send_btn_->setObjectName("controlSend");
    send_btn_->setToolTip(QStringLiteral("立即发送当前列表中的全部指令"
                                         "（与播放触发同一入口）"));
    btn_row->addWidget(send_btn_);
    lay->addLayout(btn_row);

    root->addWidget(box);

    // ---- 预填（片段域 control_messages → 列表）------------------------------
    if (ctx_.clip) refreshFrom(ctx_.clip->control_messages);

    // ---- 信号连接 ------------------------------------------------------------
    connect(send_btn_, &QPushButton::clicked,
            this, &ControlClipSection::onSendTest);

    // ---- 轮询 diff 写回（仅可见时；800ms 对编辑落定足够即时）-----------------
    poll_ = new QTimer(this);
    poll_->setInterval(800);
    connect(poll_, &QTimer::timeout, this, &ControlClipSection::onEdited);
}

// ---------------------------------------------------------------------------
// 可见性驱动轮询：面板显示启动（切到本页/选中即编辑态），隐藏即停——
// 轮询只为「编辑落定」服务，不可见时无编辑可言。
// ---------------------------------------------------------------------------
void ControlClipSection::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    if (poll_) poll_->start();
}

void ControlClipSection::hideEvent(QHideEvent* e) {
    QWidget::hideEvent(e);
    if (poll_) poll_->stop();
    onEdited(); // 隐藏前把未落定的编辑冲掉（页签切走的瞬间兜底）
}

// ---------------------------------------------------------------------------
// 【ClipSectionInterface】宿主广播灌值（模型→面板向）
// ---------------------------------------------------------------------------
void ControlClipSection::refreshFromClip(const void* clip) {
    auto* c = static_cast<const ms::Clip*>(clip);
    if (!c) return;
    refreshFrom(c->control_messages);
}

// ---------------------------------------------------------------------------
// 「测试发送」：先 diff 落定（确保 clip 与场景副本拿到最新列表），再逐条
// parseOSC——与播放触发的发送入口完全一致（ms::fireControlMessages，
// StatusContainer → 广播链；解析逻辑与触发引擎同源）。
// ---------------------------------------------------------------------------
void ControlClipSection::onSendTest() {
    if (!ctx_.clip) return;
    const QString now = currentJson();
    if (now != last_json_) { // 编辑未落定：先写回（不经 on_edited 二次 diff）
        ctx_.clip->control_messages = now.toStdString();
        last_json_ = now;
        if (ctx_.on_edited) ctx_.on_edited();
    }
    if (last_json_.isEmpty()) return; // 空列表无可发送
    ms::fireControlMessages(last_json_.toStdString());
}

// ---------------------------------------------------------------------------
// 列表当前值 → 紧凑 JSON 串（save() 结构 {"messages":[...]}）
// ---------------------------------------------------------------------------
QString ControlClipSection::currentJson() const {
    if (!list_) return QString();
    return QString::fromUtf8(
        QJsonDocument(list_->save()).toJson(QJsonDocument::Compact));
}

// ---------------------------------------------------------------------------
// 片段 control_messages → 列表灌值（空串/解析失败 = 清空；diff 快照更新）
// ---------------------------------------------------------------------------
void ControlClipSection::refreshFrom(const std::string& json) {
    if (!list_) return;
    const QString q = QString::fromStdString(json);
    if (q == last_json_) return; // 内容未变：不重灌（防回环 + 保卡片焦点）
    last_json_ = q;
    const QJsonDocument doc = QJsonDocument::fromJson(q.toUtf8());
    const QSignalBlocker blocker(list_); // 列表无对外信号，防御性包裹
    list_->load(doc.object());
}

// ---------------------------------------------------------------------------
// 轮询落点：列表 JSON ≠ 快照 → 直写片段域 + on_edited 落定
// ---------------------------------------------------------------------------
void ControlClipSection::onEdited() {
    if (!ctx_.clip) return;
    const QString now = currentJson();
    if (now == last_json_) return; // 无变化
    ctx_.clip->control_messages = now.toStdString();
    last_json_ = now;
    if (ctx_.on_edited) ctx_.on_edited(); // 宿主落定：sceneEdited + 广播
}

} // namespace ms
