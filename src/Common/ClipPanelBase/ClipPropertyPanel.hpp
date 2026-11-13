// ============================================================================
//  ClipPropertyPanel —— 片段属性面板（可附加控件，非模态）
//
//  定位（恢复 QtTimeLine 原生插件架构的宿主侧落地）：
//    * QtTimeLine 的属性面板体系 = 通用面板（ClipTimePanel 时间 +
//      ClipAppearancePanel 颜色/备注）+ 片段自定义区（虚函数
//      clipPropertyWidget() 返回的控件，库自动附加到通用面板下方）；
//    * 本类即"片段自定义区"的宿主实现：承载 ms::Clip 的媒体绑定编辑域
//      ——媒体文件（「指定媒体文件…」选文件导入 + 当前文件名展示）。
//      时间几何归库通用面板（帧级，实时回写链路已有）；
//    * 【画布变换区退役（2026-11-06 方案 A 用户拍板）】X/Y/Z/宽/高/旋转/
//      不透明度七属性编辑整体迁入 TweenSection 合并区（appendSection
//      追加区——值恒显示播放头处最终结果、双模直写：通道 auto-key /
//      无通道静态基准），消除"同属性两处显示/两套值"的混乱。本面板
//      不再持有 transform 控件（applyTo/refreshFrom 相应裁剪）；
//    * 【素材候选下拉已移除（2026-11-03）；"绑定"行标签已移除
//      （2026-11-04 用户定案"去掉绑定选项，改为文件名显示"）】
//      「指定媒体文件…」按钮保留（选文件导入+绑定一步完成），当前媒体
//      文件名以只读标签直接展示（无"绑定："前缀）——编辑入口唯一、
//      信息不丢；
//    * 即时生效：任一控件变更 → applyTo(快照) → 注入的 applier 回调
//      （适配器按 noteName 定位工作副本写回 + 模型镜像 + sceneEdited），
//      不再有 确定/取消 两段式——与库面板（spinbox 改即生效）交互一致。
//
//  弹出入口（2026-11-05 收敛：timeline 双击 = 唯一入口）：
//    * timeline 双击 = 库组合面板（通用区 + 本面板附加）；
//    * 画布/场景树双击弹面板已移除（用户定案"两处面板并存易歧义"——
//      画布改为拖拽经 clipTransformLiveChanged 信号实时同步面板显示；
//      适配器 showClipOwnPanel 保留供测试/程序化打开）。
//    * 素材导入经 EditorWindow 注入的钩子共用（importAssetForTrack）。
// ============================================================================

#pragma once

#include <cstdint>
#include <QWidget>
#include <QString>
#include <QTabWidget> // 基类（2026-11-13 三页式：面板即页容器）
#include <functional>

#include "model/SceneModel.hpp"
#include "Common/Devices/MediaLibrary/MediaLibrary.h"

#include "ClipPanelBaseExport.h" // CLIPPANELBASE_EXPORT（2026-11-13 迁入本 DLL）

class QGroupBox;
class QLabel;
class SelectorComboBox; // 全局命名空间类（GUIElements）——前向声明必须在 namespace 外

namespace ms {

/// 片段属性面板（2026-11-13 三页式：本面板即 QTabWidget 页容器）——
/// 页 1「通用」= 媒体文件区（库通用面板的时间属性/片段属性由 BaseClass
/// initPropertyWidget qobject_cast<QTabWidget*> 识别后插入本页）；
/// 页 2「动画」/页 3 专属区由适配器经 appendSection 追加（顺序即页序）。
/// 生命周期：由库组合面板（或独立宿主窗口）作为普通子控件管理；
/// 面板不持有模型指针——写回只依赖注入的回调（按 clip id 定位，稳定）。
///
/// 【字段级脏标记】组合面板场景下，同一片段的 start/end 由库通用面板
/// （ClipTimePanel）与时间轴拖拽实时维护——本面板的快照随时可能过期。
/// 因此 applyTo 只覆盖"用户在本面板真正动过的字段"（dirty_mask_ 位图），
/// 其余字段透传工作副本当前值，从机制上排除交叉覆盖回退。
/// （变换七属性 2026-11-06 方案 A 起归 TweenSection 合并区——本面板
///  dirty 位仅剩 kAssetId/kDuration。）
class CLIPPANELBASE_EXPORT ClipPropertyPanel : public QTabWidget {
    Q_OBJECT
public:
    /// 素材导入钩子：选文件 → 类型校验/入表（EditorWindow 侧实现）。
    /// 成功返回新 Asset；失败 id 为空并填 err。
    using ImportFn = std::function<Asset(const QString& file,
                                         std::string& err)>;
    /// 写回钩子：面板变更 → 工作副本（适配器按 clip id 定位 + 合并写回）。
    /// 【无参】实现侧从工作副本取当前值合并 dirty 字段（防过期快照交叉
    /// 覆盖），面板无需（也不应）构造完整 Clip 传入——原 const Clip&
    /// 签名迫使 onEdited 每次控件变更深拷贝快照，而所有实现都忽略该参数。
    using ApplyFn = std::function<void()>;

    /// \param clip           当前片段快照（只读初值来源；写回走 applier）
    /// \param media_category 本片段媒体文件在媒体库中的分类（候选列表数据源；
    ///                       适配器按 AssetKind 映射，Unknown = 不限分类候选）
    /// \param binding_name   当前绑定素材的展示名（文件名；调用方从工作副本
    ///                       素材表解析，空 = 显示未绑定引导/仅 id）
    /// \param parent         Qt 父控件（库组合面板/独立窗口）
    explicit ClipPropertyPanel(const Clip& clip,
                               MediaLibrary::Category media_category,
                               const QString& binding_name = QString(),
                               QWidget* parent = nullptr);

    /// 注入写回回调（构造后立即设置；未设置时控件只读展示）。
    void setApplier(ApplyFn applier) { applier_ = std::move(applier); }
    /// 注入素材导入回调（「指定媒体文件…」按钮）。
    void setImporter(ImportFn importer) { importer_ = std::move(importer); }

    /// 附加一页到面板（2026-11-13 三页式改版）：本面板即 QTabWidget 页容器
    /// ——构造时已有第 1 页「通用」（媒体文件区；库通用面板的时间属性/
    /// 片段属性由 BaseClass initPropertyWidget 识别 Tab 结构后插入本页），
    /// 追加顺序即页序：适配器先追加「动画」页（TweenSection，全类型恒有）
    /// 再追加专属页（片段插件工厂供给；无则不加——用户定案「没有的话就
    /// 不嵌入」）。所有权随页转移（子控件，随面板销毁）。
    /// 【面板扩展铁律】既有控件冻结（2026-11-03 用户约定），专属区一律经
    /// 此追加（文本区 TextClipSection / 实况区随插件），不揉进、不重排
    /// 既有控件。
    void appendSection(QWidget* section, const QString& title);

    /// 媒体绑定区可见性（2026-11-09 用户定案：Live 片段隐藏——「指定媒体
    /// 文件…」与实况区的源选择功能重复，绑定/选择入口统一归实况区；
    /// 2026-11-13 用户定案：Text 片段同样隐藏——文本内容走文本专属编辑
    /// 区，无媒体文件绑定语义。其余类型默认显示，行为不变。追加式开关：
    /// 不重排既有控件）。
    void setMediaSectionVisible(bool visible);

    /// 采用一个已导入素材为本片段的绑定（onPickMediaFile 导入成功后的
    /// 公共落点；独立公开供测试/未来拖拽绑定复用）：
    /// 更新当前绑定 + 标脏 kAssetId + 视频/音频素材联动时长（kDuration）
    /// + 即时写回。无时长素材（图片/文本）不联动时长。
    /// （宽高回填 2026-11-06 方案 A 起移至适配器 applyClipPanelMerge——
    ///  面板不再持有 transform 控件。）
    void adoptImportedAsset(const Asset& a);

    /// 面板脏字段 → 片段（合并语义：只覆盖 dirty_mask_ 标记的字段，
    /// c 的其余字段保持调用方填入的当前值——通常是工作副本最新值）
    void applyTo(Clip& c) const;

    /// 【双向同步·模型→面板向 2026-11-05】外部变化灌显示：把工作副本
    /// 当前值刷进控件（媒体绑定变更后由适配器广播）。灌值规则：
    ///   * QSignalBlocker 包裹（灌值不产生编辑事件，不误标脏/写回）；
    ///   * 灌值后控件恒等于工作副本，applyTo 的 dirty 合并即使覆盖也
    ///     写回同值——过期回退问题随之消除。
    void refreshFrom(const Clip& c, const QString& binding_name = QString());

private slots:
    /// 媒体库选择器变更（2026-11-12 用户定案：文件选择经 SelectorComboBox
    /// 从媒体库获取，替换原 QFileDialog 任意路径选择）：候选文件名 →
    /// allFiles() 反查绝对路径 → 导入钩子 → adoptImportedAsset
    void onMediaSelected(const QString& file_name);
    /// 任一编辑控件变更 → 标脏 + applier 写回
    void onEdited(std::uint32_t field_bit);

private:
    /// 只读绑定标签刷新（asset_id + 展示名 → 文本，超长中间省略）
    void updateAssetLabel();
    /// 媒体库文件名 → 绝对路径（allFiles() 按文件名匹配；未命中返回空）
    QString resolveLibraryPath(const QString& file_name) const;
    /// 脏字段位图（applyTo 只覆盖置位字段；位定义见 .cpp kField* 常量）
    enum FieldBit : std::uint32_t {
        kAssetId  = 1u << 0,
        kDuration = 1u << 1,
    };
    // ---- 媒体文件绑定（媒体库选择器 + 只读详情；QFileDialog 入口退役）------
    QWidget*    general_page_ = nullptr; ///< 第 1 页「通用」（媒体区所在；
                                        ///<  BaseClass 通用面板插入本页）
    QGroupBox*  media_box_ = nullptr;   ///< 媒体区容器（Live 片段整体隐藏）
    SelectorComboBox* media_selector_ = nullptr; ///< 媒体库文件选择（候选=
                                                 ///< 对应分类文件名列表）
    QLabel*     asset_label_ = nullptr; ///< 当前绑定详情（文件名+id，只读）
    std::string current_asset_id_;      ///< 当前绑定（预填快照值；导入后更新）
    QString current_binding_name_;      ///< 当前绑定展示名（文件名；导入后更新）

    // ---- 快照与回调 ----------------------------------------------------------
    Clip     snapshot_;   // 打开时的片段快照（只读初值来源）
    std::int64_t effective_duration_us_ = 0; // 生效时长（素材切换自动联动媒体
                                             // 时长；面板无时长编辑控件——归库
                                             // 通用面板，联动值经此透传到写回）
    std::uint32_t dirty_mask_ = 0; // 用户动过的字段位图（applyTo 覆盖范围）
    ApplyFn  applier_;   // 表单值写回（适配器按 noteName 定位）
    ImportFn importer_; // 素材导入（EditorWindow 侧类型校验/入表）
};

} // namespace ms
