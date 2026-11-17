// ============================================================================
//  MappingClipSection —— Mapping 片段专属编辑区（OSC 消息表）
//
//  【2026-11-15 片段移植】随 Mapping 片段插件走——宿主经
//  ClipPlugInterface::createPropertySection 工厂加载（ctx =
//  ms::ClipSectionContext*），广播灌值经 ClipSectionInterface::
//  refreshFromClip（qobject_cast 探测）。结构对齐 ControlClipSection
// （写回策略/防回环/悬垂修复同源），差异 = 无「测试发送」按钮
// （节点体系 Mapping 属性面板同样无测试按钮——发送只在回放发生）。
//
//  编辑域 = 片段域 ms::Clip.clip_data（JSON {"messages":[...]}，
//  OSCMessageListWidget save/load 兼容——save 把求值结果覆写为表达式原文，
//  持久化即表达式文本）——Mapping 不绑素材（asset_id 恒空），消息表就是
//  片段自身业务数据，随场景 pb（field 13）保存与下发。
//
//  写回策略（与 ControlClipSection 同源）：列表控件无内容变化信号 →
//  800ms 轮询 diff（仅面板可见时运行）：save() 序列化结果与上次快照不同
//  → 直写 clip->clip_data + ctx.on_edited 落定；refreshFromClip 灌值与
//  轮询共用同一 diff 快照——广播回灌内容相同即跳过，天然防回环。
//  测试定位锚：objectName 前缀 mapping*（mappingMessages/mappingHint）。
// ============================================================================

#pragma once

#include <string>

#include <QWidget>

#include "AbstractClipInterface.h"      // ClipSectionInterface（宿主广播接口）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（编辑上下文）
#include "model/SceneModel.hpp"

class QLabel;
class QTimer;
class OSCMessageListWidget; // 全局命名空间类（OscListWidget 库）

namespace ms {

/// Mapping 片段专属编辑区：clip_data 消息表域即时编辑（800ms 轮询 diff
/// 写回内化 + on_edited 落定）。生命周期：作为 ClipPropertyPanel::
/// appendSection 的子控件由 Qt 父子关系管理（随面板销毁）；ctx 内指针
/// 由宿主保证覆盖本控件存活期。
class MappingClipSection : public QWidget, public ClipSectionInterface {
    Q_OBJECT
    Q_INTERFACES(ClipSectionInterface)
public:
    /// \param ctx    编辑上下文（工作副本片段指针 + 编辑落定回调；
    ///               Mapping 无素材，scene 仅随载荷携带、本区块不用）
    /// \param parent Qt 父控件（= 承载面板 ClipPropertyPanel）
    explicit MappingClipSection(const ClipSectionContext& ctx,
                                QWidget* parent = nullptr);

    /// 【ClipSectionInterface】宿主广播灌值（模型→面板向）：\p clip 为
    /// const ms::Clip*（经 void* 穿越基类边界）——按其 clip_data 现灌
    /// 列表。与轮询共用 diff 快照：内容相同即跳过（防回环）。
    void refreshFromClip(const ms::Clip* clip) override; // P3-B 类型化签名

protected:
    /// 可见性驱动轮询：显示启动 / 隐藏停止 + 隐藏前冲掉未落定编辑
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

private:
    /// 列表当前内容 → JSON 串（OSCMessageListWidget::save → 紧凑文档）
    QString currentJson() const;
    /// 列表灌值：clip_data JSON → load（QSignalBlocker 包裹；更新 diff
    /// 快照）——空串/解析失败 = 清空列表
    void refreshFrom(const std::string& json);
    /// 轮询落点：列表 JSON ≠ 快照 → 直写 clip->clip_data + on_edited 落定
    void onEdited();
    /// 【2026-11-15 悬垂修复】工作副本片段现查：clip 存于 vector<Clip>，
    /// 片段增删会搬移元素——优先走宿主注入的 clip_lookup 实时定位，
    /// 无 lookup 时兜底工厂时刻指针（ControlClipSection 同源）。
    Clip*       boundClip();
    const Clip* boundClip() const;

    // ---- 控件（objectName = 测试定位锚）--------------------------------------
    OSCMessageListWidget* list_ = nullptr; ///< mappingMessages：消息表
    QLabel*               hint_ = nullptr; ///< mappingHint：语义说明

    // ---- 状态 -----------------------------------------------------------------
    ClipSectionContext ctx_;   ///< 编辑上下文（clip/clip_lookup/on_edited——
                               ///<  值拷贝；指针语义见 ClipSectionContext 头注释）
    QString   last_json_;      ///< 上次写回/灌值内容快照（diff 防回环）
    QTimer*   poll_ = nullptr; ///< 800ms 轮询定时器（仅可见时启动）
};

} // namespace ms
