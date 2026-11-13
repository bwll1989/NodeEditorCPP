// ============================================================================
//  ControlClipSection —— Control 片段专属编辑区（指令列表 + 测试发送）
//
//  定位（2026-11-13 Control 片段）：随 Control 片段插件走——宿主经
//  ClipPlugInterface::createPropertySection 工厂加载（ctx =
//  ms::ClipSectionContext*），广播灌值经 ClipSectionInterface::
//  refreshFromClip（qobject_cast 探测）。
//
//  编辑域 = 片段域 ms::Clip.control_messages（std::string，JSON；
//  OSCMessageListWidget::save/load 兼容格式 {"messages":[...]}）——
//  与 Text/Live 的素材域（Asset.text_* 等）不同：Control 不绑素材，
//  指令表就是片段自身的业务数据，随场景 pb（Clip.control_messages，
//  field 10）保存与下发。
//
//  写回策略（列表控件无内容变化信号——OSCMessageListWidget 只在卡片级
//  connect，不对外转发）：
//    * 800ms 轮询 diff（仅面板可见时运行）：列表 save() 序列化结果与
//      上次快照不同 → 直写 clip->control_messages + ctx.on_edited 落定；
//      覆盖增删/改值/拖拽/右键等一切编辑路径，开销可忽略（几张卡片）；
//    * 「测试发送」点击时立即 diff 写回（发送前确保数据已落定）；
//    * refreshFromClip 灌值与轮询共用同一 diff 快照——宿主广播回灌时
//      内容相同即跳过，天然防回环。
//
//  触发发送 = StatusContainer::instance()->parseOSC(msg)（与旧 Trigger
//  的 trigger() 同一入口）：入状态容器 → statusUpdated → WS 8992 广播
//  + GlobalEventBus 控件联动。测试发送按钮 = 手动执行同一动作。
//  测试定位锚：objectName 前缀 control*（controlMessages/controlSend）。
// ============================================================================

#pragma once

#include <string>

#include <QWidget>

#include "AbstractClipInterface.h"      // ClipSectionInterface（宿主广播接口）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（编辑上下文）
#include "model/SceneModel.hpp"

class QLabel;
class QPushButton;
class QTimer;
class OSCMessageListWidget; // 全局命名空间类（OscListWidget 库）

namespace ms {

/// Control 片段专属编辑区：片段 control_messages 域即时编辑
///（800ms 轮询 diff 写回内化 + on_edited 落定 + 测试发送）。
/// 生命周期：作为 ClipPropertyPanel::appendSection 的子控件由 Qt 父子
/// 关系管理（随面板销毁）；ctx 内指针由宿主保证覆盖本控件存活期。
class ControlClipSection : public QWidget, public ClipSectionInterface {
    Q_OBJECT
    Q_INTERFACES(ClipSectionInterface)
public:
    /// \param ctx    编辑上下文（工作副本片段指针 + 编辑落定回调；
    ///               Control 无素材，scene 仅随载荷携带、本区块不用）
    /// \param parent Qt 父控件（= 承载面板 ClipPropertyPanel）
    explicit ControlClipSection(const ClipSectionContext& ctx,
                                QWidget* parent = nullptr);

    /// 【ClipSectionInterface】宿主广播灌值（模型→面板向）：\p clip 为
    /// const ms::Clip*（经 void* 穿越基类边界）——按其 control_messages
    /// 现灌列表。与轮询共用 diff 快照：内容相同即跳过（防回环）。
    void refreshFromClip(const void* clip) override;

private slots:
    /// 「测试发送」：先 diff 写回落定，再按当前列表逐条 parseOSC
    void onSendTest();

protected:
    /// 可见性驱动轮询：显示启动 / 隐藏停止 + 隐藏前冲掉未落定编辑
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

private:
    /// 列表当前内容 → JSON 串（OSCMessageListWidget::save → 紧凑文档）
    QString currentJson() const;
    /// 列表灌值：control_messages JSON → load（QSignalBlocker 包裹；
    /// 更新 diff 快照）——空串/解析失败 = 清空列表
    void refreshFrom(const std::string& json);
    /// 轮询落点：列表 JSON ≠ 快照 → 直写 clip->control_messages +
    /// on_edited 落定（宿主 sceneEdited + 自动下发 + 广播灌值——广播
    /// 回到本区时 diff 相同跳过，无回环）
    void onEdited();

    // ---- 控件（objectName = 测试定位锚）--------------------------------------
    OSCMessageListWidget* list_     = nullptr; ///< controlMessages：指令列表
    QPushButton*          send_btn_ = nullptr; ///< controlSend：测试发送
    QLabel*               hint_     = nullptr; ///< 触发语义说明

    // ---- 状态 -----------------------------------------------------------------
    ClipSectionContext ctx_;   ///< 编辑上下文（clip/on_edited——值拷贝；
                               ///<  指针语义见 ClipSectionContext 头注释）
    QString   last_json_;      ///< 上次写回/灌值内容快照（diff 防回环）
    QTimer*   poll_ = nullptr; ///< 800ms 轮询定时器（仅可见时启动）
};

} // namespace ms
