// ============================================================================
//  MappingClipSection 实现 —— Mapping 片段消息表编辑区（无测试按钮）
//
//  交互约定（与 ControlClipSection 同源，写回目标为片段域 clip_data）：
//    * 写回 = 800ms 轮询 diff（可见时）——列表控件无内容变化信号，diff
//      快照是唯一真值判据，同时天然防广播回环；
//    * 无「测试发送」按钮：节点体系 Mapping 属性面板同样无（发送只在
//      回放发生——表达式依赖 $percent/$frame，离开播放上下文无意义）；
//    * onlyInternal=true（与旧 Mapping 的 OSCMessageListWidget(true) 一致）：
//      卡片隐藏 host:port，消息固定走本地状态容器（host/port 不入数据）。
// ============================================================================

#include "MappingClipSection.hpp"

#include <QEvent>
#include <QGroupBox>
#include <QJsonDocument>
#include <QLabel>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

#include "OscListWidget/OSCMessageListWidget.hpp" // OscListWidget 库

namespace ms {

// ---------------------------------------------------------------------------
// 【2026-11-15 悬垂修复】工作副本片段现查（ControlClipSection 同源）：
// 优先宿主注入的 clip_lookup（按 clip_id 实时定位，vector 搬移安全），
// 无 lookup 兜底工厂时刻指针（旧宿主场景）。
// ---------------------------------------------------------------------------
Clip* MappingClipSection::boundClip() {
    if (ctx_.clip_lookup) {
        if (Clip* c = ctx_.clip_lookup()) return c;
    }
    return ctx_.clip;
}

const Clip* MappingClipSection::boundClip() const {
    if (ctx_.clip_lookup) {
        if (Clip* c = ctx_.clip_lookup()) return c;
    }
    return ctx_.clip;
}

// ---------------------------------------------------------------------------
// 构造：控件树 + 预填 + 轮询启动（预填在轮询启动之前——首帧 diff 无变化）
// ---------------------------------------------------------------------------
MappingClipSection::MappingClipSection(const ClipSectionContext& ctx,
                                       QWidget* parent)
    : QWidget(parent), ctx_(ctx) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(4);

    auto* box = new QGroupBox(tr("映射消息"), this);
    auto* lay = new QVBoxLayout(box);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->setSpacing(4);

    // ---- 求值语义说明 --------------------------------------------------------
    hint_ = new QLabel(
        tr("播放期间逐帧求值表达式并重发：$percent = 区间内进度(0~1)，"
           "$frame = 区间内相对帧号；值可写列表 [a,b,c] 或算式"), box);
    hint_->setObjectName("mappingHint");
    hint_->setStyleSheet(QStringLiteral("color:#888;"));
    hint_->setWordWrap(true);
    lay->addWidget(hint_);

    // ---- 消息表（OSCMessageListWidget：增删/编辑/右键菜单全内置）-------------
    list_ = new OSCMessageListWidget(/*onlyInternal=*/true, box);
    list_->setObjectName("mappingMessages");
    list_->setMinimumHeight(96);
    lay->addWidget(list_);

    root->addWidget(box);

    // ---- 预填（片段域 clip_data → 列表）--------------------------------------
    if (const Clip* c = boundClip()) refreshFrom(c->clip_data);

    // ---- 轮询 diff 写回（仅可见时；800ms 对编辑落定足够即时）-----------------
    poll_ = new QTimer(this);
    poll_->setInterval(800);
    connect(poll_, &QTimer::timeout, this, &MappingClipSection::onEdited);
}

// ---------------------------------------------------------------------------
// 可见性驱动轮询：面板显示启动（切到本页/选中即编辑态），隐藏即停——
// 轮询只为「编辑落定」服务，不可见时无编辑可言。
// ---------------------------------------------------------------------------
void MappingClipSection::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    if (poll_) poll_->start();
}

void MappingClipSection::hideEvent(QHideEvent* e) {
    QWidget::hideEvent(e);
    if (poll_) poll_->stop();
    onEdited(); // 隐藏前把未落定的编辑冲掉（页签切走的瞬间兜底）
}

// ---------------------------------------------------------------------------
// 【ClipSectionInterface】宿主广播灌值（模型→面板向）
// ---------------------------------------------------------------------------
void MappingClipSection::refreshFromClip(const ms::Clip* clip) {
    if (!clip) return; // 签名已类型化（P3-B），无需 static_cast 还原
    refreshFrom(clip->clip_data);
}

// ---------------------------------------------------------------------------
// 列表当前值 → 紧凑 JSON 串（save() 结构 {"messages":[...]}；value =
// 表达式原文——save 覆写口径）
// ---------------------------------------------------------------------------
QString MappingClipSection::currentJson() const {
    if (!list_) return QString();
    return QString::fromUtf8(
        QJsonDocument(list_->save()).toJson(QJsonDocument::Compact));
}

// ---------------------------------------------------------------------------
// 片段 clip_data → 列表灌值（空串/解析失败 = 清空；diff 快照更新）
// ---------------------------------------------------------------------------
void MappingClipSection::refreshFrom(const std::string& json) {
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
void MappingClipSection::onEdited() {
    Clip* c = boundClip();
    if (!c) return;
    const QString now = currentJson();
    if (now == last_json_) return; // 无变化
    c->clip_data = now.toStdString();
    last_json_ = now;
    if (ctx_.on_edited) ctx_.on_edited(); // 宿主落定：sceneEdited + 广播
}

} // namespace ms
