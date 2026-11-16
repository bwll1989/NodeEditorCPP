// ============================================================================
//  ArtnetClipSection —— Artnet 片段专属编辑区（DMX 选择/目标主机/起始域）
//
//  【2026-11-15 片段移植】随 Artnet 片段插件走——宿主经
//  ClipPlugInterface::createPropertySection 工厂加载（ctx =
//  ms::ClipSectionContext*），广播灌值经 ClipSectionInterface::
//  refreshFromClip（qobject_cast 探测）。结构对齐 ControlClipSection
// （写回策略/防回环/悬垂修复同源）。
//
//  编辑域 = 片段域 ms::Clip.clip_data（JSON：{"file":媒体库相对路径,
//  "targetHost":"...","startUniverse":N}）——Artnet 不绑素材（asset_id 恒
//  空），配置就是片段自身业务数据，随场景 pb（field 13）保存与下发。
//
//  控件（对齐节点体系属性面板同款）：
//    * DMX 文件 = SelectorComboBox（MediaLibrary::Category::DMX，媒体库
//      候选列表；文本即库内相对文件名）；
//    * 目标主机 = QLineEdit（IPv4 校验，含 .255 广播）；
//    * 起始域   = QSpinBox（0~65535）；
//    * 信息标签 = 文件探测（FFV1/宽高/域数/时长）——独立轻探测，与模型
//      回放解码互不依赖。
//
//  写回策略（与 ControlClipSection 同源）：列表/控件无统一变更信号 →
//  800ms 轮询 diff（仅面板可见时运行）：serialize 与上次快照不同 → 直写
//  clip->clip_data + ctx.on_edited 落定；refreshFromClip 灌值与轮询共用
//  同一 diff 快照——广播回灌内容相同即跳过，天然防回环。
//  测试定位锚：objectName 前缀 artnet*（artnetFile/artnetHost/
//  artnetUniverse/artnetInfo）。
// ============================================================================

#pragma once

#include <string>

#include <QWidget>

#include "AbstractClipInterface.h"      // ClipSectionInterface（宿主广播接口）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（编辑上下文）
#include "model/SceneModel.hpp"

class QSpinBox;
class QTimer;
class QLabel;
class QLineEdit;
class SelectorComboBox; // 全局命名空间类（GUIElements）

namespace ms {

/// Artnet 片段专属编辑区：clip_data 配置域即时编辑（800ms 轮询 diff 写回
/// 内化 + on_edited 落定 + 文件探测信息）。生命周期：作为
/// ClipPropertyPanel::appendSection 的子控件由 Qt 父子关系管理（随面板
/// 销毁）；ctx 内指针由宿主保证覆盖本控件存活期。
class ArtnetClipSection : public QWidget, public ClipSectionInterface {
    Q_OBJECT
    Q_INTERFACES(ClipSectionInterface)
public:
    /// \param ctx    编辑上下文（工作副本片段指针 + 编辑落定回调；
    ///               Artnet 无素材，scene 仅随载荷携带、本区块不用）
    /// \param parent Qt 父控件（= 承载面板 ClipPropertyPanel）
    explicit ArtnetClipSection(const ClipSectionContext& ctx,
                               QWidget* parent = nullptr);

    /// 【ClipSectionInterface】宿主广播灌值（模型→面板向）：\p clip 为
    /// const ms::Clip*（经 void* 穿越基类边界）——按其 clip_data 现灌
    /// 控件。与轮询共用 diff 快照：内容相同即跳过（防回环）。
    void refreshFromClip(const void* clip) override;

protected:
    /// 可见性驱动轮询：显示启动 / 隐藏停止 + 隐藏前冲掉未落定编辑
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

private:
    /// 控件当前值 → 配置 JSON 串（紧凑文档；坏值容错回默认）
    QString currentJson() const;
    /// 灌值：clip_data JSON → 控件（QSignalBlocker 包裹；更新 diff 快照
    /// + 信息标签探测）——空串/解析失败 = 恢复默认值
    void refreshFrom(const std::string& json);
    /// 轮询落点：serialize ≠ 快照 → 直写 clip->clip_data + on_edited 落定
    void onEdited();
    /// 文件探测 → 信息标签（FFV1/宽×高/域数/时长；打不开 = 错误提示）
    void updateInfoLabel();
    /// 【2026-11-15 悬垂修复】工作副本片段现查：clip 存于 vector<Clip>，
    /// 片段增删会搬移元素——优先走宿主注入的 clip_lookup 实时定位，
    /// 无 lookup 时兜底工厂时刻指针（ControlClipSection 同源）。
    Clip*       boundClip();
    const Clip* boundClip() const;

    // ---- 控件（objectName = 测试定位锚）--------------------------------------
    SelectorComboBox* file_      = nullptr; ///< artnetFile：DMX 视频选择
    QLineEdit*        host_      = nullptr; ///< artnetHost：目标主机
    QSpinBox*         universe_  = nullptr; ///< artnetUniverse：起始域
    QLabel*           info_      = nullptr; ///< artnetInfo：文件探测信息

    // ---- 状态 -----------------------------------------------------------------
    ClipSectionContext ctx_;   ///< 编辑上下文（clip/clip_lookup/on_edited——
                               ///<  值拷贝；指针语义见 ClipSectionContext 头注释）
    QString   last_json_;      ///< 上次写回/灌值内容快照（diff 防回环）
    QTimer*   poll_ = nullptr; ///< 800ms 轮询定时器（仅可见时启动）
};

} // namespace ms
