#ifndef CLIPPLUGINTERFACE_H
#define CLIPPLUGINTERFACE_H

#include <QtPlugin>
#include <QWidget>
#include "AbstractClipModel.hpp"

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
     * @param ctx    ms::ClipSectionContext*（MediaCore 类型，经 void* 穿越
     *               基类边界——BaseClass 不依赖 MediaCore；插件侧
     *               static_cast 还原，含工作副本片段/场景/编辑落定回调）
     * @param parent Qt 父控件（= 承载面板 ClipPropertyPanel，面板拥有
     *               返回控件的所有权）
     * @return QWidget* 专属编辑区，或 nullptr
     */
    virtual QWidget* createPropertySection(void* ctx, QWidget* parent)
    {
        return nullptr; // 默认无专属区（时间轴片段基类插件）
    }
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
     * @param clip const ms::Clip*（MediaCore 类型经 void* 穿越；实现侧
     *             static_cast 还原，按 clip->asset_id 现查绑定素材——
     *             换绑后取到新素材，不缓存）
     */
    virtual void refreshFromClip(const void* clip) = 0;
};

#define ClipPlugInterface_iid "com.timeline.ClipPlugInterface"
Q_DECLARE_INTERFACE(ClipPlugInterface, ClipPlugInterface_iid)
#define ClipSectionInterface_iid "com.timeline.ClipSectionInterface"
Q_DECLARE_INTERFACE(ClipSectionInterface, ClipSectionInterface_iid)

#endif // CLIPPLUGINTERFACE_H