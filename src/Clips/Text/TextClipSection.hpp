// ============================================================================
//  TextClipSection —— 文本片段专属编辑区（片段属性面板的类型扩展区）
//
//  定位（2026-11-05 文本片段完整化；2026-11-13 随片段插件化迁入本目录）：
//    * 【2026-11-13 片段插件化】本区块随 Text 片段插件走——宿主经
//      ClipPlugInterface::createPropertySection 工厂加载（ctx=
//      ms::ClipSectionContext*：工作副本片段/场景/编辑落定回调），广播
//      灌值经 ClipSectionInterface::refreshFromClip（qobject_cast 探测）。
//      原"适配器 applyClipTextEdit 写回"职责内化为本类 onEdited 直写；
//    * 挂接方式 = ClipPropertyPanel::appendSection（面板"只追加不重构"
//      扩展约定——不揉进既有通用区控件，遵守片段属性面板冻结铁律）；
//    * 编辑域 = 绑定素材的 Asset.text_*（内容/字体/字号/颜色/粗斜/
//      水平对齐/背景色），与通用面板的 Clip 域（时间/变换）正交——
//      素材按 ctx.clip->asset_id 现查（换绑安全），非缓存引用；
//    * 即时生效：任一控件变更 → onEdited() 直写素材 text_* 全字段 +
//      TextRasterizer 测量回填自然尺寸（transform.w/h<=0 的 auto-size
//      语义显示尺寸即时跟随）+ ctx.on_edited 落定回调（宿主做
//      sceneEdited/自动下发 + 面板广播灌值）；
//    * 双向同步：refreshFromClip（宿主广播灌值；QSignalBlocker 防回环 +
//      内容框焦点保护——用户正在输入的文本不被外部灌值打断）；
//    * 字体下拉数据源 = TextRasterizer::fontCatalog()（与渲染端同一
//      映射表，杜绝 UI 列表与解析表漂移）；display 端缺字体时渲染端
//      自动回退雅黑——下拉不做本机过滤（两端目录可能不同）。
//  测试定位锚：objectName 前缀 text*（textContent/textFont/...）。
// ============================================================================

#pragma once

#include <cstdint>

#include <QWidget>

#include "AbstractClipInterface.h"      // ClipSectionInterface（宿主广播接口）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（编辑上下文）
#include "model/SceneModel.hpp"

class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class IntDragValueWidget; // 全局命名空间类（GUIElements）——前向声明必须在 namespace 外

namespace ms {

/// 文本片段专属编辑区：素材 text_* 域即时编辑（写回内化 + on_edited 落定）。
/// 生命周期：作为 ClipPropertyPanel::appendSection 的子控件由 Qt 父子
/// 关系管理（随面板销毁）；ctx 内指针由宿主保证覆盖本控件存活期。
class TextClipSection : public QWidget, public ClipSectionInterface {
    Q_OBJECT
    Q_INTERFACES(ClipSectionInterface)
public:
    /// \param ctx    编辑上下文（工作副本片段/场景/编辑落定回调；
    ///               初值素材按 ctx.clip->asset_id 现查——工厂已校验
    ///               kind==Text，查不到用默认空素材兜底）
    /// \param parent Qt 父控件（= 承载面板 ClipPropertyPanel）
    explicit TextClipSection(const ClipSectionContext& ctx,
                             QWidget* parent = nullptr);

    /// 【ClipSectionInterface】宿主广播灌值（模型→面板向）：\p clip 为
    /// const ms::Clip*（经 void* 穿越基类边界）——按其当前绑定现查素材
    ///（换绑安全）→ refreshFrom。QSignalBlocker 防回环 + 内容框焦点
    /// 保护（正在打字不灌——"正在编辑的字段用户说了算"）。
    void refreshFromClip(const void* clip) override;

private slots:
    /// 「文字颜色…」：QColorDialog 选色 → 成员 + 色块 → 写回
    void onPickTextColor();
    /// 「背景颜色…」（背景启用时可用）：同上
    void onPickBgColor();

private:
    /// 控件当前值 → 素材 text_* 全字段（无条件覆盖：文本域只有本区块
    /// 一个写者，无过期快照交叉覆盖问题）。
    /// 背景色语义：未勾选启用 → text_bg_color = 0（透明背景）。
    void applyTo(Asset& a) const;

    /// 【双向同步·模型→面板向】素材当前值 → 控件（refreshFromClip 落点）
    void refreshFrom(const Asset& a);

    /// 工作副本绑定素材定位（const/非 const 双形态；按 asset_id 现查，
    /// 未绑定/类型不符返回 nullptr——写回与灌值统一防御）
    const Asset* boundAsset() const;
    Asset* writableAsset();

    /// 任一控件变更 → 直写素材 text_* + 测量回填自然尺寸（与
    /// makeAssetFromFile/TextMediaSource::prepare 同一渲染入口测量——
    /// 三处同源无漂移）+ ctx.on_edited 落定回调
    void onEdited();
    /// 颜色成员 → 色块样式（写回链路无关的纯显示）
    void updateSwatches();

    // ---- 内容与样式（objectName = 测试定位锚）--------------------------------
    QPlainTextEdit* content_  = nullptr; ///< textContent：多行文本内容
    QComboBox*      font_     = nullptr; ///< textFont：映射表字体（data=键名）
    IntDragValueWidget* size_ = nullptr; ///< textFontSize：字号 px（8..512，
                                        ///<  拖拽/滚轮/双击精确输入——2026-11-12
                                        ///<  用户定案面板数值控件统一 DragValue）
    QPushButton*    color_btn_   = nullptr; ///< textColor：选色按钮
    QLabel*         color_swatch_= nullptr; ///< 文字颜色预览色块
    QCheckBox*      bold_     = nullptr;  ///< textBold
    QCheckBox*      italic_   = nullptr;  ///< textItalic
    QComboBox*      align_    = nullptr;  ///< textAlign：0=左 1=中 2=右
    QCheckBox*      bg_enable_   = nullptr; ///< textBgEnable：启用背景色块
    QPushButton*    bg_btn_      = nullptr; ///< textBgColor：背景选色按钮
    QLabel*         bg_swatch_   = nullptr; ///< 背景色预览色块

    // ---- 状态 -----------------------------------------------------------------
    std::uint32_t color_    = 0xFFFFFF; ///< 文字颜色 0xRRGGBB（选色后更新）
    std::uint32_t bg_color_ = 0;        ///< 背景色 0xRRGGBB（未启用时仍记忆，
                                        ///<  勾选恢复不打断用户上次选色）
    ClipSectionContext ctx_;            ///< 编辑上下文（clip/scene/on_edited——
                                        ///<  值拷贝；指针语义见头注释）
};

} // namespace ms
