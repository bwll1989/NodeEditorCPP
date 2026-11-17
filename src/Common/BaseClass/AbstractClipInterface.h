#ifndef CLIPPLUGINTERFACE_H
#define CLIPPLUGINTERFACE_H

#include <QtPlugin>
#include <QStringList>
#include <QWidget>
#include "AbstractClipModel.hpp"

// 【P3-B 协议类型化 2026-11-17】MediaCore 类型前置声明——BaseClass 不依赖
// MediaCore（铁律），完整定义只在插件/宿主侧 include（签名用指针形参，
// 不完整类型合法）。class-key 必须与 MediaCore 侧定义一致（均为 struct）。
namespace ms {
struct ClipSectionContext; // MediaCore model/ClipSectionContext.hpp（工作副本片段+场景+编辑落定回调）
struct Clip;               // MediaCore model/SceneModel.hpp（工作副本片段）
}

// 【P4-A 动画页暴露面 schema 化 2026-11-17】插件声明的「动画」页
//（TweenSection）暴露面：本类型片段上哪些变换/合成属性有意义。字符串
// key 白名单而非 ms::TweenProp 枚举——plugin_audio 只链 BaseClass+Qt
//（不链 MediaCore，枚举不可达）；key 词汇对齐 P4 终态 FieldDef.key。
struct ClipTweenExposure {
    bool page_visible = true;  ///< 动画页整体显隐（false = 适配器整页不加，
                               ///< TweenSection 不构造）
    QStringList exposed_keys;  ///< 暴露属性 key 白名单；空 = 全暴露（既有
                               ///< 13 行行为，Video/Image/Text/Live/Mapping
                               ///< 零改动）。词汇：position/size/rotation/
                               ///< crop_top/crop_bottom/crop_left/crop_right/
                               ///< corner_lt/corner_rt/corner_rb/corner_lb/
                               ///< opacity/audio_gain（未知 key = schema
                               ///< 漂移，宿主 qWarning）
};

class ClipPlugInterface
{
public:
    /**
     * 析构函数
     */
    virtual ~ClipPlugInterface() {}
    /**
     * 获取类型
     * @return QString 类型
     */
    virtual QString clipType() const = 0;
    /**
     * 创建模型
     * @param qint64 startUs 片段起始时刻（μs）
     * @return AbstractClipModel* 模型
     */
    virtual AbstractClipModel* createModel(qint64 startUs) = 0;
    /**
     * 片段属性面板专属编辑区工厂（UI 扩展点，2026-11-13 片段插件化引入）。
     *
     * 宿主（QtTimelineModelAdapter::makeClipPropertyPanel）为片段组装属性
     * 面板时调用：插件返回类型专属编辑区（如文本区/实况区），追加到通用
     * 面板底部；返回 nullptr = 该类型无专属区（面板只含通用区）。
     *
     * @param ctx    ms::ClipSectionContext*（MediaCore 类型，前置声明
     *               类型化签名【P3-B 2026-11-17，原 void* 穿越 + 插件侧
     *               static_cast 还原退役】——BaseClass 仍不依赖 MediaCore；
     *               插件/宿主 include 完整定义后直用成员）
     * @param parent Qt 父控件（= 承载面板 ClipPropertyPanel，面板拥有
     *               返回控件的所有权）
     * @return QWidget* 专属编辑区，或 nullptr
     */
    virtual QWidget* createPropertySection(ms::ClipSectionContext* ctx,
                                           QWidget* parent)
    {
        return nullptr; // 默认无专属区（时间轴片段基类插件）
    }

    // ---- 类型显示元数据（P2-B 下放 2026-11-17）------------------------------
    // 页签名/媒体区语义/绑定行文案 = 插件声明，宿主 if 链退役——新类型零
    // 宿主改动（轨道去类型化同原则：类型知识跟片段走）。三者均为可选虚，
    // 默认实现保守兜底；vtable 追加在尾部（2026-11-17 前编译的旧插件不兼容
    // ——BaseClass 头变更铁律：src/Clips 全量重编）。

    /**
     * 类型显示名（片段语义中文名，不带「轨」字）——属性面板第 3 页页签
     * 等片段语义显示场合（与轨道名/轨道显示 hint 区分）。默认 = clipType()
     * 英文串（兜底可见，便于发现漏声明）。
     * 【宿主侧原 kindPageName 表已删除——该表曾漂移：Artnet/Mapping 缺失
     *  落兜底「属性」（11-17 实证），正是类型知识留宿主的维护病。】
     */
    virtual QString clipDisplayName() const { return clipType(); }

    /**
     * 本类型是否有媒体文件绑定语义（属性面板页 1 媒体区显隐依据）。
     * false = 面板媒体区整体隐藏（QGroupBox setVisible(false)，布局自动
     * 回收占位）。默认 true（Video/Image/Audio 等媒体类型零改动）。
     * 【宿主侧原 setMediaSectionVisible 的 AssetKind if 链下放至此。】
     */
    virtual bool hasMediaBinding() const { return true; }

    /**
     * 无媒体绑定语义时的绑定行说明文案（仅 hasMediaBinding()==false 时
     * 生效；空 = 保持宿主解析结果不变）。原宿主硬编码文案（Control/
     * Artnet/Mapping 的"无需媒体文件"类说明）下放插件声明；Live 的动态
     * 实况源名读素材域（Asset.live_source_name），仍归宿主。
     */
    virtual QString bindingHint() const { return QString(); }

    /**
     * 动画页暴露面声明（P4-A 2026-11-17，可选虚）：本类型片段的「动画」
     * 页（TweenSection）暴露哪些属性行。默认 = 全暴露 13 行（既有行为，
     * 未声明插件的保守兜底）。【vtable 尾部追加——BaseClass 头变更铁律：
     * src/Clips 全量重编。】
     */
    virtual ClipTweenExposure tweenExposure() const { return {}; }
};

/**
 * 片段专属编辑区刷新接口（宿主广播灌值用，2026-11-13 片段插件化引入）。
 *
 * 专属区随插件走后，宿主不再持有具体类型——外部变化（另一面板写回/
 * 画布拖拽/时间轴拖动）经 refreshClipPanels 广播时，对注册表内每个
 * 区块 qobject_cast 到本接口灌值；实现侧 QSignalBlocker 防回环 +
 * 焦点保护（正在编辑的控件不被灌值打断）。
 */
class ClipSectionInterface
{
public:
    virtual ~ClipSectionInterface() = default;
    /**
     * 灌值（模型→面板向）。
     * @param clip const ms::Clip*（【P3-B 2026-11-17 类型化签名，原 void*
     *             穿越 + static_cast 还原退役】；实现侧按 clip->asset_id
     *             现查绑定素材——换绑后取到新素材，不缓存）
     */
    virtual void refreshFromClip(const ms::Clip* clip) = 0;
};

#define ClipPlugInterface_iid "com.timeline.ClipPlugInterface"
Q_DECLARE_INTERFACE(ClipPlugInterface, ClipPlugInterface_iid)
#define ClipSectionInterface_iid "com.timeline.ClipSectionInterface"
Q_DECLARE_INTERFACE(ClipSectionInterface, ClipSectionInterface_iid)

#endif // CLIPPLUGINTERFACE_H