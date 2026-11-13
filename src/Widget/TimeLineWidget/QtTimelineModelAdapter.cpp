// ============================================================================
//  QtTimelineModelAdapter.cpp —— Scene(微秒) ↔ QtTimeLine(帧) 适配层实现
//
//  装载策略（loadSceneToModel）：
//    * 用 beginResetModel/endResetModel 包全量重建——reset 语义下视图整体
//      刷新，无需逐行 beginInsertRows；内部索引（m_clipLocation/m_trackSet/
//      clipsByStart）统一经 rebuildIndex() 重建（基类 protected，适配器可调）；
//    * 每个片段 new AbstractClipModel 基类实例（控件允许基类实例化，绘制
//      data 均有默认实现），noteName 记 ms clip id（回写定位锚点）、
//      setColor 按轨道类型区分；不使用插件 clip（无插件目录时
//      BasePluginLoader::createModelForType 返回空，属安全 no-op）。
//
//  回写策略（onClipGeometryChanged / syncModelToWorkScene）：
//    * 按 noteName 定位工作副本 clip（对增删稳定）；写 start_us 与
//      duration_us（μs 同基直传，2026-11-10 微秒原生）；出点超过时间线
//      总时长时兜底拉长（与属性面板 applyPropertyToClip 同规则）；
//    * dirty_ 置位 + sceneEdited() 信号——状态栏提示"未下发"。
//
//  seek 节流：onSetPlayheadPos 只记 pending + 启动 80ms 单发定时器；
//  flushPendingSeek 发最后一次值。连续拖动时定时器持续再触发形成
//  "80ms 一发"的命令流，松手后 ≤80ms 内最终位置必达。
// ============================================================================

#include "QtTimelineModelAdapter.hpp"

// 片段属性面板（素材绑定 + 画布变换；附加进库组合面板）
// 【2026-11-13 片段插件化】通用面板/Tween 合并区迁 Common/ClipPanelBase.dll；
// 片段专属编辑区（Text/Live）随各自片段插件走（src/Clips/，经
// ClipPlugInterface::createPropertySection 工厂加载）——本文件不再持有
// 任何具体片段 UI 类型，片段 UI 改动只重编对应 .plugin。
#include "Common/ClipPanelBase/ClipPropertyPanel.hpp"
#include "Common/ClipPanelBase/TweenSection.hpp" // Tween 关键帧动画区（2026-11-05，全类型片段追加）

// 媒体类型插件（src/Clips/<Type>/）：纯磁盘驱动（2026-11-13 用户定案
// 「片段与内核彻底解耦」）——插件头不再编进主程序，内置兜底表已删；
// 类型表完全来自 plugin/*.plugin 磁盘扫描，没有 .plugin 文件 = 类型不存在
// （Add Track 菜单不露项、Add Item 落空）。基类 SceneClipModel 在 BaseClass.dll。
#include "SceneClipModel.hpp"

#include "model/ClipSectionContext.hpp" // 片段专属区工厂载荷（Clip/Scene/回调）
#include "ControlMessages.hpp"          // Control 触发引擎（ms::fireControlMessages）

#include <QDialog>
#include <QFileInfo>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "media/TextRasterizer.hpp" // 文本测量回填（onAddClip 自动素材）

namespace ms {

using us = std::int64_t;

namespace {

// ---------------------------------------------------------------------------
// Tween 关键帧镜像构建（2026-11-05，时间轴曲线编辑带）：ms::Clip.tweens
//（us 域）→ SceneClipModel 帧域镜像（TweenChannelDots）。通道/关键帧顺序
// 保持原序（镜像下标 = 工作副本下标，applyTweenKeyMove 据此回写）。
//
// 值域（vmin/vmax，曲线带 0..1 归一化 ↔ 业务值的双向映射基准）：
//   * 基础 = 通道键值范围，上下各外扩 25%（曲线/标记不顶带边，且给拖拽
//     留出可见调节余量——拖不出带 = 拖不出值域）；
//   * 退化（单键或全同值，range≈0）：按属性给"合理呼吸步长"——
//     X/Y/W/H = max(100, 画布对应轴×25%)；旋转 = 45°；不透明度 = 0.25；
//   * 硬钳：不透明度最终收进 [0,1]（与 TweenEngine.resolveTransform 的
//     输出钳制同口径）；宽/高下限 0（负尺寸未定义）。
// ---------------------------------------------------------------------------
std::vector<TweenChannelDots> buildTweenDots(const Clip& c,
                                             double canvas_w,
                                             double canvas_h) {
    std::vector<TweenChannelDots> dots;
    dots.reserve(c.tweens.size());
    for (const auto& ch : c.tweens) {
        TweenChannelDots d;
        d.prop = static_cast<int>(ch.prop);
        d.keys.reserve(ch.keys.size());
        double vmin = 0.0, vmax = 0.0;
        bool has = false;
        for (const auto& k : ch.keys) {
            TweenKeyDot dot;
            // μs 直传（2026-11-10 微秒原生——镜像与工作副本同基，无换算层）
            dot.t_us   = static_cast<double>(k.t_us);
            dot.value  = k.v0; // v0 分量（标量值/向量第一分量）
            dot.interp = static_cast<int>(k.interp);
            if (!has) { vmin = vmax = k.v0; has = true; }
            else {
                vmin = std::min(vmin, k.v0);
                vmax = std::max(vmax, k.v0);
            }
            d.keys.push_back(dot);
        }
        // 值域：键值范围外扩 25%；退化时按属性步长（呼吸空间）
        const double range = vmax - vmin;
        if (range < 1e-9) {
            double step = 1.0;
            switch (d.prop) {
                case 1: // 位置（v0=X 横移）
                    step = std::max(100.0, canvas_w * 0.25);
                    break;
                case 2: // 尺寸（v0=W）
                    step = std::max(100.0, canvas_w * 0.25);
                    break;
                case 3: step = 45.0;  break;  // 旋转（度）
                case 4: step = 0.25;  break;  // 不透明度
                case 5: step = 10.0;  break;  // Z 层级
                case 6: case 7: case 8: case 9: // 角点位移（画布 px）
                    step = 100.0;
                    break;
                default: break;
            }
            vmin -= step;
            vmax += step;
        } else {
            const double pad = range * 0.25;
            vmin -= pad;
            vmax += pad;
        }
        // 属性硬钳（与解算端语义对齐）
        if (d.prop == 4) { // 不透明度 [0,1]
            vmin = std::max(0.0, vmin);
            vmax = std::min(1.0, vmax);
            if (vmax - vmin < 1e-9) { vmin = 0.0; vmax = 1.0; } // 退化兜底
        }
        if (d.prop == 2) // 尺寸下限 0（负尺寸未定义）
            vmin = std::max(0.0, vmin);
        d.vmin = vmin;
        d.vmax = vmax;
        dots.push_back(std::move(d));
    }
    return dots;
}

} // namespace

// ---------------------------------------------------------------------------
// 时间基准：μs 原生（2026-11-10 路线 C）——库模型与 Scene 权威域同基，
// 原 framesToUs/usToFrames（PAL 25fps 双向换算镜像）已整体退役。
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 构造：seek 节流定时器（单发模式）+ 进程内轨道类型表
// （构造实现在文件尾"结构编辑"区块——与 WorkScenePluginLoader 定义同区）
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 工作副本挂接
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::attachWorkScene(Scene* work) {
    work_ = work;
}

// ---------------------------------------------------------------------------
// 颜色/类型映射：轨道资产类型 → 控件类型串与着色
// ---------------------------------------------------------------------------
QString QtTimelineModelAdapter::typeForKind(AssetKind kind) {
    switch (kind) {
        case AssetKind::Video: return "Video";
        case AssetKind::Image: return "Image";
        case AssetKind::Text:  return "Text";
        case AssetKind::Audio: return "Audio";
        case AssetKind::Live:  return "Live";
        case AssetKind::Control: return "Control";
        default:               return "Unknown";
    }
}

// （类型着色已归媒体插件：VideoClipModel 等构造时 setColor 自己的类型色）

// ---------------------------------------------------------------------------
// 全量装载：Scene → 模型（reset 语义，视图整体刷新）
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::loadSceneToModel(const Scene& scene) {
    loading_ = true; // 装载期间屏蔽 clip 信号回写（避免把自己刚写的值又写回）

    beginResetModel();
    // ---- 清空旧结构：reset 语义下直接释放并清容器（视图随后整体重建，
    //      不需要逐行 remove 通知；内部索引一并清）----
    for (TrackData* track : m_tracks) {
        if (!track) continue;
        for (AbstractClipModel* clip : track->clips) delete clip;
        track->clips.clear();
        delete track;
    }
    m_tracks.clear();
    m_clipLocation.clear();
    m_trackSet.clear();
    m_pendingGeometryClips.clear();

    // ---- 轨道与片段 ----
    // 工作副本与装载场景对齐（重载/首载同路径）；同指针时跳过（EditorWindow
    // 常以 scene_work_ 自身调用，避免无谓自拷贝）
    if (work_ && work_ != &scene) *work_ = scene;

    for (const auto& tr : scene.timeline.tracks) {
        auto* td = new TrackData;
        td->type = typeForKind(tr.kind);
        td->name = tr.name.empty() ? QString::fromStdString(tr.id)
                                   : QString::fromStdString(tr.name);
        m_tracks.push_back(td);
        m_trackSet.insert(td);

        for (const auto& c : tr.clips) {
            // 片段模型经插件工厂创建（类型专属模型：VideoClipModel…，
            // 类型色由插件内定）；start 直传（μs 同基，2026-11-10 微秒原生）
            const qint64 start_us = std::max<std::int64_t>(0, c.start_us);
            AbstractClipModel* clip = nullptr;
            if (auto* loader = getPluginLoader())
                clip = loader->createModelForType(td->type, start_us);
            if (!clip) // 防御：类型未注册时退回基类骨架
                clip = new AbstractClipModel(start_us, td->type);
            clip->setParent(this); // 生命周期统一挂适配器
            clip->setId(++m_clipNextId);          // 控件侧 id 只需唯一
            clip->setTimelineModel(this);
            clip->setTimeCodeType(getTimeCodeType()); // 与模型一致（PAL）
            clip->setNoteName(QString::fromStdString(c.id)); // ★ 回写定位锚点
            // ---- Live 轨片段素材缺失自愈（2026-11-09）------------------------
            // Add Item 漏绑 live_asset_id 期间（11-06~11-08）保存的场景里，
            // 实况片段 asset_id 为空 → 实况区不追加/实况写回断裂/display
            // 端无画面。装载时补建默认实况素材并回绑（与 Add Item 同默认
            // 值；素材 id 对既有素材表防重）。work_ 与 scene 同指针时二者
            // 同一实体（198 行仅异指针拷贝），索引对齐回绑两侧一致。
            std::string bound_asset = c.asset_id;
            if (tr.kind == AssetKind::Live && bound_asset.empty() && work_) {
                Asset la;
                la.kind             = AssetKind::Live;
                la.live_source_name = "";  // 空 = 未选源（实况区选择）
                la.live_ingest      = 0;   // ndi
                la.live_latency     = 0;
                la.width            = 1280;
                la.height           = 720;
                int  next  = 1;
                bool clash = true;
                while (clash) {
                    clash = false;
                    la.id = "live-" + std::to_string(next++);
                    for (const auto& x : work_->assets)
                        if (x.id == la.id) { clash = true; break; }
                }
                work_->assets.push_back(la);
                bound_asset = la.id;
                // 回绑工作副本侧（work_==&scene 时 c 即同一元素，无碍）
                for (auto& wt : work_->timeline.tracks)
                    for (auto& wc : wt.clips)
                        if (wc.id == c.id) { wc.asset_id = la.id; break; }
                std::fprintf(stderr,
                             "[adapter] 实况片段 %s 素材缺失（历史漏绑），"
                             "装载自愈补建 %s\n", c.id.c_str(), la.id.c_str());
                std::fflush(stderr);
            }
            // 素材引用镜像进模型（画布渲染/属性对话框联动用）+
            // 片段自定义属性面板工厂（统一走 configureClipPanel：注入工厂 +
            // 非 SceneClipModel 的诊断留痕）
            if (auto* sc = dynamic_cast<SceneClipModel*>(clip)) {
                sc->setAssetId(QString::fromStdString(bound_asset));
                // Tween 关键帧镜像（时间轴曲线编辑带；装载即灌，μs 直传
                // + 值域计算需要画布尺寸）
                sc->setTweenChannels(buildTweenDots(c, scene.canvas_w,
                                                    scene.canvas_h));
                configureClipPanel(sc, c.id, td->type);
            }
            // 时长直传；不足 1μs 的片段钳到 1μs（零长片段无编辑意义）
            const qint64 end_us = c.start_us + c.duration_us;
            clip->setEnd(std::max(end_us, start_us + 1));
            watchClip(clip); // 建立模型内部几何监听（基类：dataChanged 队列）
            // 业务回写监听：片段被拖拽/面板改帧 → 工作副本同步 + dirty 置位
            //（与基类 watch 并行，二者订阅同一信号各司其职）
            connect(clip, &AbstractClipModel::timelinePositionChanged, this,
                    [this, clip](int) { onClipGeometryChanged(clip); });
            connect(clip, &AbstractClipModel::lengthChanged, this,
                    [this, clip](int) { onClipGeometryChanged(clip); });
            td->clips.push_back(clip);
        }
    }

    rebuildIndex();           // clip 指针 → (trackRow, clipRow) 索引（基类）
    onUpdateTimeLineLength(); // 聚合总长（max end）并广播 S_LengthChanged
    endResetModel();

    loading_ = false;
    dirty_clips_.clear();     // 刚从权威场景对齐，脏片段清单一并作废
}

// ---------------------------------------------------------------------------
// 属性面板 → 模型：单 clip 几何（增量，不 reset、不重置视图缩放）
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::setClipGeometryFromScene(
        const std::string& clip_id, std::int64_t start_us, std::int64_t dur_us) {
    const QString note = QString::fromStdString(clip_id);
    for (TrackData* td : m_tracks) {
        for (AbstractClipModel* clip : td->clips) {
            if (clip->noteName() != note) continue;
            loading_ = true; // 本路径的 setStart/setEnd 信号不回写（值已同源）
            clip->setStart(start_us);
            clip->setEnd(std::max(start_us + dur_us, start_us + 1)); // 不足 1μs 钳 1μs
            loading_ = false;
            // 面板值对本 clip 恢复权威：移出脏清单（若之前在时间轴拖过它，
            // 本次面板编辑意味着面板值更新，时间轴旧拖拽不再需要回写）
            dirty_clips_.erase(note.toStdString());
            return;
        }
    }
}

void QtTimelineModelAdapter::setClipAssetFromScene(
        const std::string& clip_id, const std::string& asset_id) {
    const QString note = QString::fromStdString(clip_id);
    for (TrackData* td : m_tracks) {
        for (AbstractClipModel* clip : td->clips) {
            if (clip->noteName() != note) continue;
            // 纯镜像更新（无信号、无回写）；动态派生跨 DLL 安全（MSVC /MD
            // 按 type_info 字符串比较——见 SceneClipModel 跨模块 RTTI 注释）
            if (auto* sc = dynamic_cast<SceneClipModel*>(clip))
                sc->setAssetId(QString::fromStdString(asset_id));
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// 唯一片段 id 生成（onDuplicateClip 创建路径共用）：
// clip-N 线性探测，与工作副本全表查重（收敛原两份同构探测循环）。
// ---------------------------------------------------------------------------
std::string QtTimelineModelAdapter::nextClipId() const {
    int next = 1; // 起点任意，靠查重推进
    for (;;) {
        const std::string cand = "clip-" + std::to_string(next++);
        bool clash = false;
        for (const auto& t : work_->timeline.tracks)
            for (const auto& c : t.clips)
                if (c.id == cand) { clash = true; break; }
        if (!clash) return cand;
    }
}

// ---------------------------------------------------------------------------
// 片段类型 → 媒体库分类（2026-11-12 用户定案：片段媒体文件选择统一走
// 媒体库 SelectorComboBox，本映射决定候选列表的分类；Live 无文件随 pb
// 分发，面板媒体区整体隐藏，映射值不参与运行）。
// ---------------------------------------------------------------------------
static MediaLibrary::Category categoryForKind(AssetKind kind);

// AssetKind → 专属页页签中文名（2026-11-13 三页式：第 3 页=片段专属属性页；
// 与 kindDisplayName「轨道名」区分——页签是片段语义，不带「轨」字）
static QString kindPageName(AssetKind kind) {
    switch (kind) {
        case AssetKind::Video: return QStringLiteral("视频");
        case AssetKind::Image: return QStringLiteral("图片");
        case AssetKind::Text:  return QStringLiteral("文本");
        case AssetKind::Audio: return QStringLiteral("音频");
        case AssetKind::Live:  return QStringLiteral("实况");
        case AssetKind::Control: return QStringLiteral("指令");
        default:               return QStringLiteral("属性");
    }
}


// ---------------------------------------------------------------------------
// 片段自定义面板工厂注入（片段创建路径统一入口）：
//   loadSceneToModel / onAddClip → configureClipPanel
// 组合面板打开时经 clipPropertyWidget() 调到注入的闭包，实时组装素材/变换
// 编辑区（见 makeClipPropertyPanel）。dynamic_cast 失败（插件 ABI 断裂/
// 基类骨架兜底）时打印警告——组合面板将没有自定义区，必须留痕排障。
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::configureClipPanel(SceneClipModel* sc,
                                                const std::string& clip_id,
                                                const QString& type) {
    if (!sc) return;
    // 工厂闭包捕获适配器 this、片段 id（回写定位锚）与轨道类型（候选过滤）
    sc->setClipPanelFactory(
        [this, clip_id, type]() -> QWidget* {
            return makeClipPropertyPanel(clip_id, type);
        });
    // 关键帧写回闭包组：时间轴片段带编辑（合并菱形拖拽/右键删除）→ 模型
    // 镜像先行 → 各自闭包 → applyTweenKey*（us 落盘 + sceneEdited + 面板
    // 广播）。返回 false = 拒绝（片段已删/无键可动），模型据此在下一拍
    // sync 重灌对齐；拖拽路径高频调用可忽略返回值。
    sc->setTweenKeyFrameMoveHandler(
        [this, clip_id](double old_frame, double new_frame) -> bool {
            return applyTweenKeyFrameMove(clip_id, old_frame, new_frame);
        });
    sc->setTweenKeyRemoveHandler(
        [this, clip_id](int channel, int key) -> bool {
            return applyTweenKeyRemove(clip_id, channel, key);
        });
    std::fprintf(stderr, "[clip-panel] 工厂已注入 clip=%s type=%s\n",
                 clip_id.c_str(), type.toUtf8().constData());
    std::fflush(stderr);
}

// ---------------------------------------------------------------------------
// 片段属性面板工厂（SceneClipModel::clipPropertyWidget → 此闭包）：
// 打开时刻实时组装——工作副本快照 + 当前绑定展示名（从工作副本素材表解析，
// 候选下拉已移除）+ 写回/导入钩子。宿主未注入 hooks_（如单测环境）时
// 返回 nullptr，组合面板只含库通用区（时间/颜色/备注），行为退化为原生默认。
// ---------------------------------------------------------------------------
QWidget* QtTimelineModelAdapter::makeClipPropertyPanel(
        const std::string& clip_id, const QString& type) {
    // 出口 1：宿主未注入 hooks（单测环境/时序异常）——退化为纯通用区
    if (!hooks_.import_media) {
        std::fprintf(stderr,
                     "[clip-panel] %s：hooks 未注入，无自定义区\n",
                     clip_id.c_str());
        std::fflush(stderr);
        return nullptr;
    }
    // 出口 2：工作副本里找不到片段（已被删，防御）
    Clip* c = locateWorkClip(QString::fromStdString(clip_id));
    if (!c) {
        std::fprintf(stderr,
                     "[clip-panel] %s：工作副本中未定位到片段，无自定义区\n",
                     clip_id.c_str());
        std::fflush(stderr);
        return nullptr;
    }
    const AssetKind kind = kindForType(type);
    // 当前绑定展示名：从工作副本素材表解析（找不到 = 空 → 面板显示未绑定
    // 引导/仅 id）。Live 素材无文件（path 恒空）——显示实况源状态而非
    // 误导性的"未绑定"：未选源提示去实况区选择，已选源显示源名。
    // Control（2026-11-13）：无媒体绑定语义——恒显示说明文案
    QString binding_name;
    if (kind == AssetKind::Control) {
        binding_name = QStringLiteral("（指令片段无需媒体文件）");
    } else {
        for (const auto& a : work_->assets)
            if (a.id == c->asset_id) {
                if (a.kind == AssetKind::Live)
                    binding_name = a.live_source_name.empty()
                        ? QStringLiteral("（实况源未选——在下方实况区选择）")
                        : QString::fromStdString(a.live_source_name);
                else
                    binding_name =
                        QFileInfo(QString::fromStdString(a.path)).fileName();
                break;
            }
    }
    auto* panel = new ClipPropertyPanel(*c, categoryForKind(kind), binding_name);
    // Live 片段：隐藏媒体文件区（2026-11-09 用户定案）——「指定媒体文件…」
    // 与实况区的源选择功能重复，绑定/选择入口统一归实况区
    // Text 片段：隐藏媒体文件区（2026-11-13 用户定案）——文本内容走文本
    // 专属编辑区，无媒体文件绑定语义
    // Control 片段：隐藏媒体文件区（2026-11-13）——无媒体文件绑定语义，
    // 指令表在「指令」专属页编辑
    if (kind == AssetKind::Live || kind == AssetKind::Text ||
        kind == AssetKind::Control)
        panel->setMediaSectionVisible(false);
    // 登记到广播注册表（双向同步）：此后画布拖拽/另一面板写回/时间轴拖动
    // 等一切工作副本变化经 refreshClipPanels 灌进本面板（非焦点控件）。
    // 面板销毁由 QPointer 自动感知，广播时惰性清理，无需关闭回调。
    clip_panels_[clip_id].push_back(panel);
    // 写回闭包：QPointer 守护（组合面板/面板可能先于适配器销毁）
    QPointer<ClipPropertyPanel> guard(panel);
    panel->setApplier([this, clip_id, guard]() {
        if (!guard) return; // 面板已销毁（信号队列残留），忽略
        applyClipPanelMerge(clip_id, guard.data());
    });
    panel->setImporter([this, kind](const QString& file, std::string& err) {
        return hooks_.import_media(file, kind, err);
    });

    // ---- 变换与关键帧合并区（2026-11-06 方案 A，全类型片段追加）------------
    // 【2026-11-13 三页式改版】先追加 = 第 2 页「动画」（页序：通用 → 动画
    // → 专属）。X/Y/Z/宽/高/旋转/不透明度七属性唯一编辑区（2026-11-06 用户
    // 拍板：原画布变换区退役——值恒显示播放头处最终结果，通道启用=改值即
    // 打键 auto-key / 未启用=静态基准）。登记进 tween_panels_ 广播注册表；
    // 写回经 applyClipTweenEdit（按 clip id 定位 → clip.tweens 无条件覆盖 +
    // transform 静态脏字段合并 + sceneEdited）；播放头注入使值框恒显示播放
    // 头处结果（WATCHOUT 语义）。
    {
        auto* tsec = new TweenSection(*c, panel); // panel 拥有
        panel->appendSection(tsec, QStringLiteral("动画"));
        tween_panels_[clip_id].push_back(tsec);
        QPointer<TweenSection> wguard(tsec);
        tsec->setPlayheadFn([this]() {
            return getPlayheadPos(); // 播放头（μs；库与 Scene 同基直传）
        });
        // 尺寸行 ⟲ 默认值（2026-11-09 补设）：绑定素材的原始像素宽高
        //（无绑定素材/宽高 0 如 Live 不注入——⟲ 点击不动作）
        if (const Asset* sa = work_->findAsset(c->asset_id);
            sa && sa->width > 0 && sa->height > 0)
            tsec->setSizeDefaultHint(sa->width, sa->height);
        tsec->setApplier([this, clip_id, wguard]() {
            if (!wguard) return; // 区块已销毁（信号队列残留），忽略
            applyClipTweenEdit(clip_id, wguard.data());
        });
        // 播放头钩子/写回钩子就位后重灌一次：打开即显示当前播放头处的
        // 插值值（构造时钩子未注入，数值是静态基准兜底）
        tsec->refreshFrom(*c);
    }

    // ---- 片段专属编辑区（2026-11-13 片段插件化）= 第 3 页 ------------------
    // 宿主不持有任何具体 Section 类型——按片段类型从插件表取工厂，经
    // ClipPlugInterface::createPropertySection 扩展点加载（文本区随
    // text.plugin、实况区随 live.plugin）；未绑定对应素材/插件未供给时
    // 返回 nullptr——页不加（用户定案「没有的话就不嵌入」）。区块实现
    // ClipSectionInterface 的登记 clip_sections_ 广播注册表；字段写回已
    // 内化进区块（直接写绑定素材），编辑落定经 ctx.on_edited 回调
    //（sceneEdited + 广播）。页签 = 类型中文名（kindDisplayName）。
    if (BasePluginLoader* pl = getPluginLoader()) {
        if (ClipPlugInterface* factory = pl->getPluginByType(type)) {
            ms::ClipSectionContext ctx;
            ctx.clip  = c;
            ctx.scene = work_;
            // 编辑落定闭包：QPointer 守护适配器（面板/区块可能先于适配器销毁）
            QPointer<QtTimelineModelAdapter> aguard(this);
            ctx.on_edited = [this, aguard, clip_id]() {
                if (!aguard) return;
                emit sceneEdited();          // "未下发"提示 + 自动下发链路
                refreshClipPanels(clip_id);  // 广播灌值（区块内焦点保护防打断）
            };
            if (QWidget* sec = factory->createPropertySection(&ctx, panel)) {
                panel->appendSection(sec, kindPageName(kind));
                clip_sections_[clip_id].push_back(sec);
            }
        }
    }
    return panel;
}

// ---------------------------------------------------------------------------
// 面板写回（合并语义）：工作副本当前值（含通用面板/拖拽/画布的最新修改）
// + 面板 dirty 字段（applyTo 只覆盖置位字段）→ 写回 + 模型镜像 + sceneEdited
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::applyClipPanelMerge(
        const std::string& clip_id, ClipPropertyPanel* panel) {
    if (!work_ || !panel) return;
    Clip* c = locateWorkClip(QString::fromStdString(clip_id));
    if (!c) return; // 片段已被删：面板后续编辑静默忽略（库面板同样语义）

    const std::string old_asset = c->asset_id;
    Clip merged = *c;        // 工作副本当前值打底
    panel->applyTo(merged);  // 只覆盖面板脏字段（防过期快照交叉覆盖）
    *c = merged;

    // 总时长兜底：片段出点超过总时长时拉长（TimelineEngine 求值边界）
    const us end = c->start_us + c->duration_us;
    if (end > work_->timeline.duration_us)
        work_->timeline.duration_us = end;

    // 模型镜像：素材变化 → assetId（画布渲染/依赖联动用）
    if (c->asset_id != old_asset) {
        setClipAssetFromScene(clip_id, c->asset_id);
        // 【宽高联动 2026-11-04 用户反馈"添加媒体后宽高一直为 0"；2026-11-06
        // 方案 A 随画布变换区自 ClipPropertyPanel 迁入】视频/图片素材（有
        // 像素尺寸）在当前宽/高未设（<=0，新片段默认态）时自动回填素材
        // 原始宽高——合并区所见即生效值。仅在未设尺寸时回填：用户已手工
        // 设定的宽高不被换绑覆盖；音频/文本素材无像素尺寸不联动。
        //（渲染/依赖两侧本就有 w<=0 auto-size 同源语义，此处回填只为
        //  UI 可辨识 + 用户可下手改；数据语义不变。）
        const Asset* na = work_->findAsset(c->asset_id);
        if (na && na->width > 0 && na->height > 0 &&
            (c->transform.w <= 0.0 || c->transform.h <= 0.0)) {
            c->transform.w = static_cast<double>(na->width);
            c->transform.h = static_cast<double>(na->height);
        }
    }
    // 时长若被面板改动（素材联动），时间轴片段长度跟随（loading_ 屏蔽回写）
    setClipGeometryFromScene(clip_id, c->start_us, c->duration_us);

    emit sceneEdited(); // 状态栏"未下发"提示（EditorWindow 挂接）
}

// ---------------------------------------------------------------------------
// 【2026-11-13 片段插件化】原 applyClipTextEdit/applyClipLiveEdit 已随专属
// 区块内化（Text/Live 区块直写绑定素材 + ctx.on_edited 落定回调）——字段
// 级写回归片段插件，宿主只保留 sceneEdited 编排与面板广播。
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Tween 区写回（2026-11-05 Tween 体系）：TweenSection 控件态 → 工作副本
// clip.tweens（tween 域唯一写者，无条件覆盖——通道增删/关键帧编辑全部在
// 区块内存态发生，这里只做拷贝落盘）→ sceneEdited（"未下发"提示 + 自动
// 下发链路）。尾部 refreshClipPanels 广播灌值：Tween 区 refreshFrom 带
// QSignalBlocker 防回环（广播 → 灌显示 → 不写回）；通用区/文本区灌回的
// 是刚写回的同值，无视觉扰动。
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::applyClipTweenEdit(const std::string& clip_id,
                                                TweenSection* section) {
    if (!work_ || !section) return;
    Clip* c = locateWorkClip(QString::fromStdString(clip_id));
    if (!c) return; // 片段已被删：后续编辑静默忽略（其他区块同语义）

    section->applyTo(*c); // 控件态 → clip.tweens（tween 域唯一写者）

    // 模型镜像同步：通道增删/关键帧增删改后，时间轴菱形随之重绘
    //（appearanceChanged → dataChanged → updateClipGeometry 局部刷新）
    syncClipTweenDots(clip_id);

    emit sceneEdited();  // "未下发"提示 + 自动下发（EditorWindow 挂接）
    refreshClipPanels(clip_id); // 广播灌值（区块内 QSignalBlocker 防回环）
}

// ---------------------------------------------------------------------------
// 【Tween 时间轴镜像】工作副本 clip.tweens → 该片段模型关键帧镜像重灌
//（面板"记录/删除/插值编辑"后菱形跟随；拖拽路径不经此——镜像先动、
// 工作副本随后落值，重灌反而会打断拖拽中的小数帧）。
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::syncClipTweenDots(const std::string& clip_id) {
    if (!work_) return;
    Clip* c = locateWorkClip(QString::fromStdString(clip_id));
    if (!c) return; // 片段已删：无镜像可灌
    const QString note = QString::fromStdString(clip_id);
    for (TrackData* td : m_tracks) {
        if (!td) continue;
        for (AbstractClipModel* clip : td->clips) {
            if (clip->noteName() != note) continue;
            if (auto* sc = dynamic_cast<SceneClipModel*>(clip))
                sc->setTweenChannels(buildTweenDots(*c, work_->canvas_w,
                                                    work_->canvas_h));
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// 【画布 auto-key 镜像（#89）】全量重灌：画布拖拽/方向键 auto-key 直写
// 工作副本（tweenAutoKeyWrite），不经 applyClipTweenEdit——此处补镜像，
// 片段带菱形随之出现（appearanceChanged → dataChanged → 局部重绘）。
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::syncAllClipTweenDots() {
    if (!work_) return;
    for (TrackData* td : m_tracks) {
        if (!td) continue;
        for (AbstractClipModel* clip : td->clips) {
            auto* sc = dynamic_cast<SceneClipModel*>(clip);
            if (!sc) continue;
            const Clip* c = locateWorkClip(sc->noteName());
            if (!c) continue; // 片段已删：模型对象应随重建消失，防御跳过
            sc->setTweenChannels(buildTweenDots(*c, work_->canvas_w,
                                                work_->canvas_h));
        }
    }
}

// ---------------------------------------------------------------------------
// 时间轴关键帧编辑写回（模型镜像帧域 → 工作副本 clip.tweens us 域）。
// 2026-11-09 合并关键帧模式：拖拽走 applyTweenKeyFrameMove（全通道桶移动，
// 通道内升序由模型桶级邻钳保证）；本区块另含单键底层入口 applyTweenKeyMove
//（联测探针/单测用）与单通道删键 applyTweenKeyRemove（右键"按参数删除"）。
// ---------------------------------------------------------------------------

/// 移动/改值（底层单键入口，探针/单测直调）。返回是否接受（片段/下标
/// 非法 = false）。
bool QtTimelineModelAdapter::applyTweenKeyMove(const std::string& clip_id,
                                               int channel, int key,
                                               double local_us,
                                               double value) {
    if (!work_) return false;
    Clip* c = locateWorkClip(QString::fromStdString(clip_id));
    if (!c) return false; // 片段已删（其他编辑路径同语义）
    if (channel < 0 || channel >= (int)c->tweens.size()) return false;
    auto& keys = c->tweens[channel].keys;
    if (key < 0 || key >= (int)keys.size()) return false;

    // μs 直通（四舍五入整数化，与镜像桶化同口径）；
    std::int64_t t = (std::int64_t)std::llround(local_us);
    // 片段区间钳制（关键帧本地时间 ∈ [0, duration_us]，R8 校验不变量）
    t = std::clamp<std::int64_t>(t, 0, std::max<std::int64_t>(0, c->duration_us));
    // 邻键升序钳制（双保险；镜像已钳一道）
    if (key > 0) t = std::max(t, keys[key - 1].t_us);
    if (key + 1 < (int)keys.size())
        t = std::min(t, keys[key + 1].t_us);
    // 时间与值都无变化：不落盘不广播（拖拽静止时的 mousemove 防抖）
    if (keys[key].t_us == t && keys[key].v0 == value) return true;

    keys[key].t_us   = t;
    keys[key].v0     = value; // v0 分量（底层单键入口只动第一分量；v1 不变）
    emit sceneEdited();          // "未下发"提示 + 自动下发链路
    refreshClipPanels(clip_id);  // Tween 区数值显示跟随（灌值防回环）
    return true;
}

/// 右键菜单删键（模型镜像在确认后才删）。空通道连通道一起删（R8 非空
/// 纪律：空通道不可序列化；镜像侧同步删通道，通道序保持一致）。
bool QtTimelineModelAdapter::applyTweenKeyRemove(const std::string& clip_id,
                                                 int channel, int key) {
    if (!work_) return false;
    Clip* c = locateWorkClip(QString::fromStdString(clip_id));
    if (!c) return false;
    if (channel < 0 || channel >= (int)c->tweens.size()) return false;
    auto& keys = c->tweens[channel].keys;
    if (key < 0 || key >= (int)keys.size()) return false;

    keys.erase(keys.begin() + key);
    if (keys.empty())
        c->tweens.erase(c->tweens.begin() + channel);
    emit sceneEdited();
    refreshClipPanels(clip_id);
    return true;
}

/// 合并菱形拖拽移动（2026-11-09 Resolve 模式）：把时刻 old_us 处全部
/// 通道的键统一移到 new_us。通道内升序不变量由模型桶级邻钳保证（桶序
/// 保持 = 各通道键序保持），此处只做 μs 取整 + 片段区间钳制。值不动（值
/// 编辑唯一入口 = 属性面板 auto-key）。返回是否有键被移动。
bool QtTimelineModelAdapter::applyTweenKeyFrameMove(const std::string& clip_id,
                                                    double old_us,
                                                    double new_us) {
    if (!work_) return false;
    Clip* c = locateWorkClip(QString::fromStdString(clip_id));
    if (!c) return false; // 片段已删（其他编辑路径同语义）

    // μs 取整（镜像桶化同口径）
    const std::int64_t old_t = (std::int64_t)std::llround(old_us);
    std::int64_t new_t = (std::int64_t)std::llround(new_us);
    // 片段区间钳制（关键帧本地时间 ∈ [0, duration_us]，R8 校验不变量）
    new_t = std::clamp<std::int64_t>(new_t, 0,
                                     std::max<std::int64_t>(0, c->duration_us));
    if (new_t == old_t) return false; // 量化后无位移（拖拽静止防抖）

    bool any = false;
    for (auto& tw : c->tweens)
        for (auto& k : tw.keys)
            if (k.t_us == old_t) { k.t_us = new_t; any = true; }
    if (!any) return false; // 该时刻无键（镜像/副本短暂漂移防御）
    emit sceneEdited();          // "未下发"提示 + 自动下发链路
    refreshClipPanels(clip_id);  // Tween 区数值显示跟随（灌值防回环）
    return true;
}

// ---------------------------------------------------------------------------
// 【Tween 专用·播放头驱动刷新】播放头移动 → 已打开 Tween 区的"播放头处
// 数值显示"重灌。只遍历 tween_panels_（注册表空 = 零开销，播放期每帧
// echo seekTo 驱动下高频调用安全）；每区块独立定位片段（删除/重载后
// locateWorkClip 落空即跳过，QPointer 失效项跳过——惰性清理交给
// refreshClipPanels 的常规路径）。只动显示：refreshFrom 内 QSignalBlocker
// 防回环，不产生写回/sceneEdited。
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::refreshTweenPanelsPlayhead() {
    if (tween_panels_.empty() || !work_) return; // 常态零开销
    for (auto& kv : tween_panels_) {
        // 惰性清理已销毁区块（随宿主面板关闭销毁）
        auto& vec = kv.second;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [](const QPointer<TweenSection>& s) {
                                     return s.isNull();
                                 }),
                  vec.end());
        if (vec.empty()) continue;
        Clip* c = locateWorkClip(QString::fromStdString(kv.first));
        if (!c) continue; // 片段已删：显示冻结在最后状态（同既有语义）
        for (auto& s : vec) s->refreshFrom(*c);
    }
}

// ---------------------------------------------------------------------------
// 【双向同步·模型→面板向】广播刷新：工作副本当前值 → 该片段所有打开中
// 属性面板的非焦点控件（灌值不产生编辑事件，refreshFrom 内 QSignalBlocker
// 防回环——广播链路：本函数 → refreshFrom，不会重新触发 sceneEdited）。
// 触发源（EditorWindow 挂接）：refreshSceneViews 汇聚点（时间轴拖动/面板
// 写回/装载等一切内容变化）+ 画布 sceneEdited（画布拖拽/方向键）。
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::refreshClipPanels(const std::string& clip_id) {
    auto it  = clip_panels_.find(clip_id);
    auto wit = tween_panels_.find(clip_id);
    auto sit = clip_sections_.find(clip_id);
    const bool has_generic  = it != clip_panels_.end();
    const bool has_tween    = wit != tween_panels_.end();
    const bool has_sections = sit != clip_sections_.end();
    if (!has_generic && !has_tween && !has_sections)
        return; // 该片段无打开面板（常态：零开销）
    if (!work_) return;

    // 惰性清理已销毁面板（组合面板关闭 / 对话框 WA_DeleteOnClose）
    if (has_generic) {
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [](const QPointer<ClipPropertyPanel>& p) {
                                     return p.isNull();
                                 }),
                  vec.end());
    }
    if (has_tween) {
        auto& wv = wit->second;
        wv.erase(std::remove_if(wv.begin(), wv.end(),
                                [](const QPointer<TweenSection>& s) {
                                    return s.isNull();
                                }),
                 wv.end());
    }
    if (has_sections) {
        auto& sv = sit->second;
        sv.erase(std::remove_if(sv.begin(), sv.end(),
                                [](const QPointer<QWidget>& s) {
                                    return s.isNull();
                                }),
                 sv.end());
    }

    // 工作副本已无此片段（时间轴删除/场景重载）：旧面板停止刷新——显示
    // 冻结在最后状态，由用户关闭（与"面板编辑已删片段被静默忽略"同语义）
    Clip* c = locateWorkClip(QString::fromStdString(clip_id));
    if (!c) return;

    // ---- 通用区广播（既有逻辑：Clip 域灌值）----
    if (has_generic) {
        auto& vec = it->second;
        if (!vec.empty()) {
            // 绑定展示名：从工作副本素材表解析（与工厂同逻辑；空 = 未绑定）
            QString binding_name;
            for (const auto& a : work_->assets)
                if (a.id == c->asset_id) {
                    binding_name =
                        QFileInfo(QString::fromStdString(a.path)).fileName();
                    break;
                }
            for (auto& p : vec) p->refreshFrom(*c, binding_name);
        }
    }
    // ---- Tween 区广播（2026-11-05；Clip 域 tweens 灌值，全类型片段）--------
    // 区块随宿主面板销毁（QPointer 失效项已在上方清理）；灌值含播放头处
    // 数值显示（refreshFrom 内 TweenEngine 解算）——内容变化与播放头移动
    // 两条触发源共用同一灌值入口。
    if (has_tween) {
        auto& wv = wit->second;
        for (auto& s : wv) s->refreshFrom(*c);
    }
    // ---- 片段专属区广播（2026-11-13 片段插件化；经 ClipSectionInterface）----
    // 专属区块随片段插件走（宿主无具体类型）——qobject_cast 到接口灌值；
    // 实现侧按 clip 当前绑定现查素材（换绑安全）+ 焦点保护/防回环自治。
    if (has_sections) {
        auto& sv = sit->second;
        for (auto& s : sv)
            if (auto* ifc = qobject_cast<ClipSectionInterface*>(s.data()))
                ifc->refreshFromClip(c);
    }
}

void QtTimelineModelAdapter::refreshAllClipPanels() {
    // 注册表空 = 无面板打开：一次哈希遍历零实际开销（高频调用安全）
    for (const auto& kv : clip_panels_)
        refreshClipPanels(kv.first);
}

// ---------------------------------------------------------------------------
// 片段自有属性面板独立打开（程序化入口；UI 入口 2026-11-05 收敛）：
//   timeline 双击 = 库组合面板（原生通用区 + clipPropertyWidget 自定义区）
//   ——【唯一 UI 入口】（用户定案"画布/场景树双击弹面板与组合面板并存
//   易歧义"，两处入口已移除，画布改为拖拽实时同步面板显示）；
//   本函数保留：回归测试程序化开面板 + 未来拖拽绑定等交互复用。
// 实现走 SceneClipModel::clipPropertyWidget() 虚工厂（与库组合面板完全同源）：
//   * 工厂每次调用新建面板实例（SceneClipModel 覆写不缓存），与已打开的
//     timeline 组合面板互不 reparent——两面板并存安全；
//   * 面板写回 setApplier → applyClipPanelMerge（合并语义，只覆盖脏字段）
//     → sceneEdited 信号 → EditorWindow 侧 refreshSceneViews +
//     scheduleAutoPublish（既有闭环），本函数弹出后无需任何善后。
// ---------------------------------------------------------------------------
bool QtTimelineModelAdapter::showClipOwnPanel(int track_idx, int clip_idx,
                                              QWidget* parent) {
    // 出口 1：未挂工作副本 / 下标越界（画布双击传入的工作副本下标）
    if (!work_ || track_idx < 0 || clip_idx < 0 ||
        track_idx >= (int)work_->timeline.tracks.size() ||
        clip_idx >= (int)work_->timeline.tracks[track_idx].clips.size())
        return false;

    // 工作副本片段 id（= 模型侧 noteName，回写定位锚点；按下标取 id 后
    // 改按 id 定位模型片段——对增删稳定，不信任下标在两边一致）
    const std::string clip_id =
        work_->timeline.tracks[track_idx].clips[clip_idx].id;
    const QString note = QString::fromStdString(clip_id);

    // 在模型中按 noteName 定位片段（找不到 = 已被删/未装载，防御）
    AbstractClipModel* model_clip = nullptr;
    for (TrackData* td : m_tracks) {
        if (!td) continue;
        for (AbstractClipModel* clip : td->clips) {
            if (clip->noteName() != note) continue;
            model_clip = clip;
            break;
        }
        if (model_clip) break;
    }
    if (!model_clip) return false;

    // 虚工厂取片段自有面板：媒体绑定 + 画布变换区。
    // 返回 null 的情形：hooks 未注入（makeClipPropertyPanel 出口 1）/
    // 工作副本缺片段（出口 2）/ 基类骨架兜底（未走插件工厂）——留痕后失败。
    QWidget* panel = model_clip->clipPropertyWidget();
    if (!panel) {
        std::fprintf(stderr, "[clip-panel] %s：自有面板不可用（工厂未注入或"
                             "非 SceneClipModel 骨架）\n", clip_id.c_str());
        std::fflush(stderr);
        return false;
    }

    // 包非模态对话框：关窗即销毁（WA_DeleteOnClose，面板随布局为
    // dialog 子对象一并释放）；无 OK/Cancel——控件变更即时写回，
    // 与 timeline 组合浮窗同为"开着就能继续操作画布/时间轴"的交互。
    auto* dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(QString("片段属性：%1（轨道 %2）")
                            .arg(note, QString::fromStdString(
                                     work_->timeline.tracks[track_idx].id)));
    auto* lay = new QVBoxLayout(dlg);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->addWidget(panel);
    dlg->show();
    return true;
}

// ---------------------------------------------------------------------------
// 回写定位：noteName（ms clip id）→ 工作副本 Clip*
// ---------------------------------------------------------------------------
Clip* QtTimelineModelAdapter::locateWorkClip(const QString& note) {
    if (!work_) return nullptr;
    const std::string id = note.toStdString();
    for (auto& tr : work_->timeline.tracks)
        for (auto& c : tr.clips)
            if (c.id == id) return &c;
    return nullptr;
}

// ---------------------------------------------------------------------------
// clip 几何变化 → 工作副本回写（实时路径）
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::onClipGeometryChanged(AbstractClipModel* clip) {
    if (loading_ || !work_ || !clip) return;

    Clip* c = locateWorkClip(clip->noteName());
    if (!c) return; // 模型里的片段不在工作副本（理论不可达；防御）

    c->start_us    = clip->start();
    c->duration_us = clip->length(); // length = end - start（μs 同基直传）
    // 总时长兜底：片段出点超过总时长时拉长（TimelineEngine 求值边界）
    const us end = c->start_us + c->duration_us;
    if (end > work_->timeline.duration_us)
        work_->timeline.duration_us = end;

    dirty_clips_.insert(clip->noteName().toStdString()); // 按片段记脏
    emit sceneEdited();
}

// ---------------------------------------------------------------------------
// 兜底同步：模型 → 工作副本（下发补丁前调用）
// ---------------------------------------------------------------------------
// 注意：只回写 dirty_clips_ 中的片段——面板写入的精确 us 值（帧界之间）
// 不能被模型的帧量化值覆盖，全量同步会静默篡改用户编辑。
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::syncModelToWorkScene() {
    if (!work_ || dirty_clips_.empty()) return;
    for (TrackData* td : m_tracks) {
        if (!td) continue;
        for (AbstractClipModel* clip : td->clips) {
            const std::string id = clip->noteName().toStdString();
            if (!dirty_clips_.count(id)) continue; // 未在时间轴改过，跳过
            Clip* c = locateWorkClip(clip->noteName());
            if (!c) { dirty_clips_.erase(id); continue; } // 已被删（防御）
            c->start_us    = clip->start();
            c->duration_us = clip->length(); // μs 同基直传
            const us end = c->start_us + c->duration_us;
            if (end > work_->timeline.duration_us)
                work_->timeline.duration_us = end;
        }
    }
    dirty_clips_.clear(); // 已同步
}

// ---------------------------------------------------------------------------
// 播放头
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::onSetPlayheadPos(qint64 newPlayheadPos) {
    // 1) 模型状态 + S_playheadMoved → 视图刷新（基类职责；μs 同基直传）
    BaseTimeLineModel::onSetPlayheadPos(newPlayheadPos);
    // 2) 排队 seek 转发（节流：定时器活着就只刷新 pending，最多 80ms 一发）
    pending_seek_us_ = newPlayheadPos;
    if (!seek_flush_->isActive()) seek_flush_->start();
    // 3) Tween 区"播放头处数值显示"跟随（注册表空时零开销——2026-11-05）
    refreshTweenPanelsPlayhead();
    // 4) Control 触发引擎（上升沿检测；见 updateControlTriggers 注释）
    updateControlTriggers(newPlayheadPos);
}

void QtTimelineModelAdapter::flushPendingSeek() {
    if (pending_seek_us_ < 0) return;
    const us v = pending_seek_us_;
    pending_seek_us_ = -1;
    emit seekRequestedUs(v);
}

void QtTimelineModelAdapter::setPlayheadFromRuntimeUs(std::int64_t us_total) {
    if (us_total < 0) us_total = 0;
    // 静态绑定基类：只更新模型状态+视图，绝不触发 seekRequestedUs 回环
    BaseTimeLineModel::onSetPlayheadPos(us_total);
    // Tween 区"播放头处数值显示"跟随播放回显（注册表空时零开销；
    // 播放期每帧 echo 驱动——refreshFrom 灌值走 QSignalBlocker，无写回）
    refreshTweenPanelsPlayhead();
    // Control 触发引擎（播放回显与手动 seek 同一检测——语义一致）
    updateControlTriggers(us_total);
}

// ---------------------------------------------------------------------------
// Control 触发引擎（2026-11-13）：播放头上升沿 → 逐条发送片段指令。
// 位置判定取模型（clip->start() 实时——时间轴拖动 dirty 期间也正确），
// 消息体取工作副本（业务真相源；locateWorkClip 按 noteName 定位）。
// 装载期（loading_）只建立基线不触发（模型重建中位置无意义）。
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::updateControlTriggers(std::int64_t now_us) {
    const std::int64_t prev = prev_playhead_us_;
    prev_playhead_us_ = now_us;
    if (prev < 0 || now_us == prev) return;   // 首拍基线 / 同值 echo
    if (loading_ || !work_) return;           // 装载期只建基线
    for (TrackData* td : m_tracks) {
        if (!td) continue;
        if (kindForType(td->type) != AssetKind::Control) continue;
        for (AbstractClipModel* clip : td->clips) {
            if (!clip) continue;
            const qint64 st = clip->start();
            if (!(prev < st && now_us >= st)) continue; // 非上升沿
            Clip* c = locateWorkClip(clip->noteName());
            if (!c) continue; // 已删（防御）
            ms::fireControlMessages(c->control_messages);
            std::fprintf(stderr,
                         "[control] 触发 clip=%s msgs=%s\n",
                         c->id.c_str(),
                         c->control_messages.empty() ? "(空)"
                             : c->control_messages.c_str());
            std::fflush(stderr);
        }
    }
}

// ===========================================================================
// 结构编辑：轨道/片段增删（模型 + 工作副本 双写）
// ===========================================================================
//
// 背景：QtTimeLine 的 onAddTrack/onAddClip 依赖 BasePluginLoader（宿主的
// DLL 插件体系）。两个途径打通原生交互：
//   1) 注入 WorkScenePluginLoader —— 磁盘 .plugin 扫描装载类型表（纯磁盘
//      驱动，没有 .plugin = 类型不存在），轨道列表右键 "Add Track" 子菜单
//      与 clip 创建工厂即恢复可用（修复原 getPluginLoader() 空指针崩溃
//      ——用户反馈"没有可用的轨道类型可以添加"的根因）；
//   2) 适配器覆写 onAddTrack/onDeleteTrack —— 基类只改模型行，工作副本
//      （业务真相源）由这里同步，保证 diff/补丁链路看到一致的场景。
// ---------------------------------------------------------------------------

namespace {

/// 宿主媒体类型白名单（与 QtTimelineModelAdapter::kindForType/typeForKind
/// 同步维护）。磁盘 plugin/ 目录可能混入 QtTimeLine 示例插件（Trigger 等）
/// ——它们能被 QPluginLoader 正常加载并注册进类型表（轨道右键菜单随之
/// 出现 "Add Trigger Track"），但宿主业务模型（AssetKind 六类、ms 回写、
/// 画布渲染）不支持：kindForType 映射为 Unknown 后轨道类型串被改写为
/// "Unknown"，Add Item 时 createModelForType("Unknown") 必然落空，报
/// "Failed to create clip model"。故在注册前按白名单拦截。
/// Control（2026-11-13）：指令片段入列宿主类型——无媒体文件/无画布呈现，
/// 仅时间轴存在；消息列表存 Clip.control_messages（片段域，随场景 pb 分发）。
inline bool isHostMediaType(const QString& type) {
    return type == "Video" || type == "Image" ||
           type == "Text"  || type == "Audio" || type == "Live" ||
           type == "Control";
}

/// 媒体插件加载器：纯磁盘驱动（无任何内置兜底——彻底解耦定案）。
///
/// 类型表完全由磁盘 plugins/TimeLine/*.plugin 驱动：QPluginLoader 按
/// ClipPlugInterface_iid 识别，插件由构建系统直接产出到
/// bin/plugins/TimeLine/（对齐节点插件 bin/plugins/DataFlow 先例）。
/// 没有对应 .plugin 文件 = 该类型不存在（Add Track 菜单不露项、
/// Add Item 落空、加载含该类型片段的场景时报类型缺失）。
/// 扫描目录（一律相对 exe 定位，对齐 display「全自含」约定）：
///   * exe 同级 plugins/TimeLine/ —— 唯一默认（开发/发布布局一致，
///     与构建输出目录同源；对齐节点插件 plugins/DataFlow 与 display 自含）
/// 诊断走 stderr（GUI 进程排障唯一可靠通道，无缓冲）。
class WorkScenePluginLoader : public BasePluginLoader {
public:
    explicit WorkScenePluginLoader(QObject* parent,
                                   const QString& plugin_dir_override = {})
        : BasePluginLoader(parent), plugin_dir_override_(plugin_dir_override) {}

    void loadPlugins() override {
        // ---- 磁盘插件扫描（*.plugin）------
        // 配置了 plugin_dir（config.ini [editor]）时只扫该目录；
        // 未配置走唯一默认 = exe 同级 plugins/TimeLine/。
        QStringList dirs;
        if (!plugin_dir_override_.isEmpty()) {
            dirs << plugin_dir_override_;
        } else {
            dirs << QCoreApplication::applicationDirPath() + "/plugins/TimeLine";
        }
        QSet<QString> seen_dirs; // 候选目录去重（不同候选可解析到同一路径）
        for (const QString& dir_path : dirs) {
            const QString clean = QDir::cleanPath(dir_path);
            if (seen_dirs.contains(clean)) continue;
            seen_dirs.insert(clean);
            const QDir plugins_dir(clean);
            if (!plugins_dir.exists()) continue;
            for (const QString& fname :
                 plugins_dir.entryList(QStringList() << "*.plugin",
                                       QDir::Files)) {
                const QString full = plugins_dir.absoluteFilePath(fname);
                // 堆分配并 parent 到 loader：QPluginLoader 栈对象析构会使
                // 插件库引用计数归零而被卸载（instance 悬垂）——本加载器
                // 与适配器同生命周期，插件库全程驻留，行为可预期。
                auto* loader = new QPluginLoader(full, this);
                QObject* instance = loader->instance();
                auto* clip_plugin = qobject_cast<ClipPlugInterface*>(instance);
                // 跳过路径统一收尾：instance() 已把插件库拉进内存，若只
                // continue 不 unload，库（及其全部依赖 DLL）会驻留到进程
                // 退出——跳过的插件越多内存浪费越大。unload 卸库 +
                // deleteLater 释放 loader（parent=this 的兜底仍在）。
                auto skip = [loader](const char* reason_fmt, auto&&... args) {
                    std::fprintf(stderr, reason_fmt, args...);
                    std::fflush(stderr);
                    loader->unload();
                    loader->deleteLater();
                };
                if (!clip_plugin) {
                    skip("[plugins] 跳过 %s：%s\n",
                         qPrintable(fname),
                         qPrintable(loader->errorString()));
                    continue;
                }
                const QString type = clip_plugin->clipType();
                // 宿主媒体类型白名单：非宿主类型（QtTimeLine 示例插件
                // Trigger/Control 等）不注册——注册了轨道菜单会露出
                // "Add Trigger Track"，而宿主业务链路（ms 回写/渲染）
                // 不支持，Add Item 必然失败（"Failed to create clip
                // model"）。跳过并留痕，插件文件保留在磁盘无副作用。
                if (!isHostMediaType(type)) {
                    skip("[plugins] 跳过 %s：类型 %s 非宿主媒体类型"
                         "（示例/第三方插件，宿主业务不适用）\n",
                         qPrintable(fname), qPrintable(type));
                    continue;
                }
                if (getAvailableTypes().contains(type)) {
                    skip("[plugins] 跳过 %s：类型 %s 已加载\n",
                         qPrintable(fname), qPrintable(type));
                    continue;
                }
                registerPlugin(clip_plugin); // 归基类类型表（同官方磁盘插件）
                std::fprintf(stderr, "[plugins] 已加载 %s（type=%s，%s）\n",
                             qPrintable(fname), qPrintable(type),
                             qPrintable(full));
                std::fflush(stderr);
            }
        }

        // 【2026-11-13 片段与内核彻底解耦（用户定案）】内置兜底注册已删除
        // ——类型表完全由磁盘 plugin/*.plugin 驱动：没有对应 .plugin 文件
        // = 该类型不存在（Add Track 菜单不露项、Add Item 落空、加载含该
        // 类型片段的场景时报类型缺失）。插件头不再编进主程序。
    }

private:
    QString plugin_dir_override_; ///< [editor] plugin_dir（空 = 默认候选）
};

} // namespace

// ---------------------------------------------------------------------------
// 构造：注入插件类型表（纯磁盘驱动，plugin_dir 可由 config.ini [editor]
// 覆盖）——修复右键菜单空指针崩溃 + 打通原生加轨入口
// ---------------------------------------------------------------------------
QtTimelineModelAdapter::QtTimelineModelAdapter(QObject* parent,
                                               const QString& plugin_dir)
    : BaseTimeLineModel(parent) {
    seek_flush_ = new QTimer(this);
    seek_flush_->setSingleShot(true);
    seek_flush_->setInterval(80);
    connect(seek_flush_, &QTimer::timeout,
            this, &QtTimelineModelAdapter::flushPendingSeek);

    // 媒体插件表（纯磁盘驱动）：loader 与片段模型都挂到适配器下
    setPluginLoader(new WorkScenePluginLoader(this, plugin_dir));
}

// ---------------------------------------------------------------------------
// 类型串 ↔ AssetKind 与显示名（三处保持一致：模型 type / id 前缀 / 中文名）
// ---------------------------------------------------------------------------
AssetKind QtTimelineModelAdapter::kindForType(const QString& type) {
    if (type == "Video") return AssetKind::Video;
    if (type == "Image") return AssetKind::Image;
    if (type == "Text")  return AssetKind::Text;
    if (type == "Audio") return AssetKind::Audio;
    if (type == "Live")  return AssetKind::Live; // 实时源（NDI/窗口/Spout/网络流）
    if (type == "Control") return AssetKind::Control; // 指令片段（无媒体）
    return AssetKind::Unknown;
}

// ---------------------------------------------------------------------------
// 片段类型 → 媒体库分类（见 makeClipPropertyPanel 前向声明处注释）
// ---------------------------------------------------------------------------
static MediaLibrary::Category categoryForKind(AssetKind kind) {
    switch (kind) {
        case AssetKind::Video: return MediaLibrary::Category::Video;
        case AssetKind::Image: return MediaLibrary::Category::Image;
        case AssetKind::Text:  return MediaLibrary::Category::Document;
        case AssetKind::Audio: return MediaLibrary::Category::Audio;
        default:               return MediaLibrary::Category::Unknown;
    }
}

const char* QtTimelineModelAdapter::kindShortName(AssetKind kind) {
    switch (kind) {
        case AssetKind::Video: return "video";
        case AssetKind::Image: return "image";
        case AssetKind::Text:  return "text";
        case AssetKind::Audio: return "audio";
        case AssetKind::Live:  return "live";
        case AssetKind::Control: return "control";
        default:               return "track";
    }
}

static QString kindDisplayName(AssetKind kind) {
    switch (kind) {
        case AssetKind::Video: return "视频轨";
        case AssetKind::Image: return "图片轨";
        case AssetKind::Text:  return "字幕轨";
        case AssetKind::Audio: return "音频轨";
        case AssetKind::Live:  return "实况轨";
        case AssetKind::Control: return "指令轨";
        default:               return "轨道";
    }
}

// ---------------------------------------------------------------------------
// 新增轨道（场景树按钮 / 时间轴右键共用）：工作副本 + 模型 双写
// ---------------------------------------------------------------------------
int QtTimelineModelAdapter::addTrackWithKind(AssetKind kind) {
    if (!work_) return -1;
    // 防御：未知类型拒绝建轨（否则轨道类型串 typeForKind→"Unknown"，
    // 片段工厂 createModelForType("Unknown") 落空——"Failed to create
    // clip model" 的间接来源；正常入口已被插件白名单拦截，此处兜底）
    if (kind == AssetKind::Unknown) {
        std::fprintf(stderr,
                     "[adapter] 拒绝创建未知类型轨道（宿主仅支持 "
                     "Video/Image/Text/Audio）\n");
        std::fflush(stderr);
        return -1;
    }

    // ---- 工作副本：追加 ms::Track（id 唯一化，防删轨后重名）----
    const std::string base = std::string("t-") + kindShortName(kind);
    int n = (int)work_->timeline.tracks.size() + 1;
    std::string id;
    do {
        id = base + "-" + std::to_string(n++);
    } while (std::any_of(work_->timeline.tracks.begin(),
                         work_->timeline.tracks.end(),
                         [&](const Track& t) { return t.id == id; }));

    ms::Track tr;
    tr.id   = id;
    tr.name = QString("%1 %2").arg(kindDisplayName(kind)).arg(n - 1)
                  .toStdString(); // "视频轨 2"（n-1：循环里先自增）
    tr.kind = kind;
    work_->timeline.tracks.push_back(tr);

    // ---- 模型：加行（与基类 onAddTrack 相同的 Model/View 通知仪式）----
    beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size());
    auto* td     = new TrackData;
    td->type     = typeForKind(kind);
    td->name     = QString::fromStdString(tr.name);
    m_tracks.push_back(td);
    m_trackSet.insert(td);
    rebuildIndex();
    endInsertRows();

    emit sceneEdited();
    return (int)work_->timeline.tracks.size() - 1;
}

// ---------------------------------------------------------------------------
// 时间轴原生入口：右键 "Add Track" / 删除轨道 → 同步工作副本
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::onAddTrack(const QString& type) {
    addTrackWithKind(kindForType(type)); // 类型串 → AssetKind → 双写
}

void QtTimelineModelAdapter::onDeleteTrack(int trackIndex) {
    // 先同步工作副本（基类删除会 delete TrackData，行号随后失效）
    if (work_ && trackIndex >= 0 && trackIndex < (int)work_->timeline.tracks.size())
        work_->timeline.tracks.erase(
            work_->timeline.tracks.begin() + trackIndex);
    // 基类：beginRemoveRows + 释放轨道拥有的 clip + endRemoveRows
    BaseTimeLineModel::onDeleteTrack(trackIndex);
    onUpdateTimeLineLength();

    emit sceneEdited();
}

// ---------------------------------------------------------------------------
// 原生"添加片段"（时间轴右键 "Add Item"——工具条极简约定后的正式入口）：
// 基类用插件工厂造的是【无素材绑定】的孤立片段（asset_id 未知），此处
// 接管为"占位片段"语义，双写保持数据一致：
//   * 调基类完成 Model/View 仪式（插件造片段 + beginInsertRows 加行 +
//     ClipIdRole 编号）；
//   * 取回新片段 → 线性探测生成唯一 ms id（clip-N）→ setNoteName 回写
//     （noteName = ms clip id 是本适配器的标识约定，回写/删除按它定位）；
//   * 工作副本同步 push（asset_id 置空 = 未绑定素材占位——下发补丁前由
//     EditorWindow 做 R1 前置提示，引导用户双击片段绑定媒体）；
//   * 补接几何信号（拖拽/改长 → 工作副本回写；基类只做了 watchClip，
//     不含 onClipGeometryChanged 的双写联动）。
// 用户随后【双击片段】在属性面板中点「指定媒体文件…」完成素材绑定。
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::onAddClip(int trackIndex, qint64 startUs) {
    // 防御：未挂工作副本时基类造的片段会成"孤儿"（回写无门）→ 拒绝并引导
    if (!work_) {
        std::fprintf(stderr, "[adapter] 原生添加片段失败：工作副本未挂载\n");
        std::fflush(stderr);
        return;
    }
    TrackData* td = (trackIndex >= 0 && trackIndex < (int)m_tracks.size())
                        ? m_tracks[trackIndex] : nullptr;
    if (!td) return; // 越界：交给基类打 qDebug（行为一致）
    const int prev_cnt = (int)td->clips.size();

    // 基类：插件工厂造片段 + beginInsertRows/endInsertRows + ClipIdRole
    BaseTimeLineModel::onAddClip(trackIndex, startUs);

    // 基类失败路径（插件缺失/类型未注册）不产生新行——静默放弃（无业务可写）
    if ((int)td->clips.size() <= prev_cnt) return;
    AbstractClipModel* mc = td->clips.back();

    // ---- 唯一 ms id：clip-N 线性探测（nextClipId 单点维护）----
    const std::string id = nextClipId();
    mc->setNoteName(QString::fromStdString(id)); // ms id ← 模型标识映射
    // 生命周期：基类造的片段由基类删除（onDeleteTrack/析构裸 delete），
    // 不做 setParent（避免所有权歧义）。
    // 面板工厂注入：基类造的片段同样要带自定义面板（否则双击报
    // "Cannot add a null widget"——三条创建路径统一约定）。

    // ---- Text 轨道：自动创建默认文本素材并绑定（2026-11-05）----------------
    // 文本随场景 pb 分发（无文件），"双击指定媒体文件"交互对文本轨不适用
    // ——Add Item 即自动生成默认文本素材（雅黑 48px 白字"字幕文本"），
    // 用户在片段属性面板的文本区改内容/样式（写回 → sceneEdited → 下发）。
    // 非文本轨道保持既有占位语义（asset_id 空，双击绑文件）。
    std::string text_asset_id;
    if (kindForType(td->type) == AssetKind::Text) {
        Asset ta;
        ta.kind           = AssetKind::Text;
        ta.text_content   = "字幕文本";
        ta.text_font      = "MicrosoftYaHei"; // 默认字体（映射表中文兜底）
        ta.text_font_size = 48;
        ta.text_color     = 0xFFFFFF;
        // id：text-1、text-2…线性探测（对既有素材表防重）
        {
            int  next  = 1;
            bool clash = true;
            while (clash) {
                clash = false;
                ta.id = "text-" + std::to_string(next++);
                for (const auto& x : work_->assets)
                    if (x.id == ta.id) { clash = true; break; }
            }
        }
        // 测量回填自然尺寸（与 makeAssetFromFile/TextMediaSource 同一
        // 渲染入口测量——三处同源，无漂移）
        const TextLayout lay =
            TextRasterizer::measure(TextRasterizer::styleFromAsset(ta));
        ta.width  = lay.width;
        ta.height = lay.height;
        work_->assets.push_back(ta);
        text_asset_id = ta.id;
        std::fprintf(stderr,
                     "[adapter] 文本轨自动创建默认文本素材: %s（在片段属"
                     "性面板文本区改内容/样式）\n", ta.id.c_str());
        std::fflush(stderr);
    }

    // ---- Live 轨道：自动创建默认实况素材并绑定（2026-11-06）----------------
    // 实时源与文本同款"无文件素材"（随场景 pb 分发）——"双击指定媒体
    // 文件"交互对实况轨不适用：Add Item 即自动生成默认 Live 素材（源名
    // 空 = 等待用户在片段属性面板实况区选 NDI 源），1280x720 默认布局
    // 尺寸（画布占位；连接后用户可改片段变换适配真实分辨率）。
    std::string live_asset_id;
    if (kindForType(td->type) == AssetKind::Live) {
        Asset la;
        la.kind             = AssetKind::Live;
        la.live_source_name = "";      // 空 = 未选源（占位语义，produce 无帧）
        la.live_ingest      = 0;       // ndi
        la.live_latency     = 0;       // 最低延迟
        la.width            = 1280;    // 默认布局尺寸（画布占位）
        la.height           = 720;
        // id：live-1、live-2…线性探测（对既有素材表防重）
        {
            int  next  = 1;
            bool clash = true;
            while (clash) {
                clash = false;
                la.id = "live-" + std::to_string(next++);
                for (const auto& x : work_->assets)
                    if (x.id == la.id) { clash = true; break; }
            }
        }
        work_->assets.push_back(la);
        live_asset_id = la.id;
        std::fprintf(stderr,
                     "[adapter] 实况轨自动创建默认实况素材: %s（在片段属"
                     "性面板实况区选 NDI 源）\n", la.id.c_str());
        std::fflush(stderr);
    }

    if (auto* sc = dynamic_cast<SceneClipModel*>(mc)) {
        if (!text_asset_id.empty())
            sc->setAssetId(QString::fromStdString(text_asset_id));
        if (!live_asset_id.empty())
            sc->setAssetId(QString::fromStdString(live_asset_id));
        configureClipPanel(sc, id, td->type);
    }

    // ---- 工作副本同步：占位片段（起点/时长按模型帧域换算回微秒）----
    Track& tr = work_->timeline.tracks[trackIndex];
    ms::Clip c;
    c.id          = id;
    // 素材绑定：Text/Live 轨道=Add Item 自动创建的素材（2026-11-09 修：
    // 此前只回填 text_asset_id，live_asset_id 漏绑——工作副本 asset_id 空
    // → 双击面板 findAsset 落空 → 实况区不追加 + 实况写回链路断裂）；
    // 其余轨道=未绑定占位（双击绑文件）
    c.asset_id    = !text_asset_id.empty() ? text_asset_id : live_asset_id;
    c.start_us    = mc->start(); // μs 同基直传（2026-11-10 微秒原生）
    // 时长 = 模型默认片段长度（μs 差值；至少 1μs 防除零/零长片段）
    c.duration_us = std::max<std::int64_t>(
        1, (std::int64_t)mc->end() - mc->start());
    tr.clips.push_back(c);

    // ---- 几何信号双写（拖拽/改长 → 工作副本回写）----
    connect(mc, &AbstractClipModel::timelinePositionChanged, this,
            [this, mc](int) { onClipGeometryChanged(mc); });
    connect(mc, &AbstractClipModel::lengthChanged, this,
            [this, mc](int) { onClipGeometryChanged(mc); });

    const std::string bound_id =
        !text_asset_id.empty() ? text_asset_id : live_asset_id;
    if (bound_id.empty()) {
        std::fprintf(stderr,
                     "[adapter] 时间轴新增占位片段 %s（轨道 %d）：未绑定素材"
                     "——双击片段指定媒体文件\n", id.c_str(), trackIndex);
    } else {
        std::fprintf(stderr,
                     "[adapter] 时间轴新增片段 %s（轨道 %d）：绑定 %s\n",
                     id.c_str(), trackIndex, bound_id.c_str());
    }
    std::fflush(stderr);
    emit sceneEdited();
}

// ---------------------------------------------------------------------------
// 拖入媒体文件（资源管理器拖拽 → 视图 dropEvent）：
//   导入素材（hooks_.import_media = importAssetForTrack：扩展名推断/入路径
//   策略/元数据探测/轨道类型校验/入表——拖入文件类型已由视图
//   isMimeAcceptable 校验与轨道匹配，kind 取轨道类型双重校验无害）
//   → 占位片段创建（onAddClip 同链：模型行 + 工作副本 push + 面板工厂）
//   → 素材绑定（SceneClipModel 纯镜像 + 工作副本 asset_id 手写）+
//   时长联动（素材 duration_us >0 时 setEnd 直写——不设 loading_，
//   lengthChanged → onClipGeometryChanged 自动回写工作副本，与面板
//   adoptImportedAsset 链同款语义）。
// 导入失败/钩子未注入 → 纯占位（onAddClip 原语义，不阻断拖入）。
// 【2026-11-11 音频片段时长联动】旧 dropEvent 丢弃文件路径只建 1 帧占位
//   ——拖入文件实际等于什么都没干；现在拖入即绑定即真实长度（对标
//   主流 NLE：拖音频进时间轴直接出现完整长度片段）。
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::onDropMediaFile(int trackIndex, qint64 startUs,
                                             const QString& file) {
    // 防御：与 onAddClip 同款（工作副本未挂载时无法导入/绑定）
    if (!work_) {
        std::fprintf(stderr, "[adapter] 拖入导入失败：工作副本未挂载\n");
        std::fflush(stderr);
        return;
    }
    TrackData* td = (trackIndex >= 0 && trackIndex < (int)m_tracks.size())
                        ? m_tracks[trackIndex] : nullptr;
    if (!td) return; // 越界（视图已校验落点在轨道上；防御）

    // ---- 导入素材（面板导入钩子同源；失败回退占位）-----------------------
    Asset a;
    if (hooks_.import_media) {
        std::string err;
        a = hooks_.import_media(file, kindForType(td->type), err);
        if (a.id.empty()) {
            // [E1] 导入失败：不再静默留 1 帧空占位（用户误以为导入成功，
            // 播放时才发现黑屏）——发可见提示信号（宿主状态栏弹示）并放弃
            // 本次拖入；轨道/工作副本零副作用
            std::fprintf(stderr, "[adapter] 拖入导入失败: %s\n", err.c_str());
            std::fflush(stderr);
            emit mediaImportFailed(file, QString::fromStdString(err));
            return;
        }
    }
    if (a.id.empty()) {
        // 钩子未注入（遗留路径）：保留原占位语义（不阻断拖入流程）
        onAddClip(trackIndex, startUs); // 纯占位（原语义）
        return;
    }

    // ---- 占位片段创建（模型行 + 工作副本 push + 面板工厂 + 信号接线）----
    const int prev_cnt = (int)td->clips.size();
    onAddClip(trackIndex, startUs);
    if ((int)td->clips.size() <= prev_cnt) return; // 基类失败（插件缺失等）
    AbstractClipModel* mc = td->clips.back();
    const QString note = mc->noteName();

    // ---- 素材绑定 + 时长联动 ---------------------------------------------
    // asset 镜像：纯写无信号（setClipAssetFromScene 同款），工作副本手写；
    // 时长：setEnd 直写（不设 loading_）→ lengthChanged →
    // onClipGeometryChanged 自动回写工作副本 duration（μs 同基直传）。
    if (auto* sc = dynamic_cast<SceneClipModel*>(mc))
        sc->setAssetId(QString::fromStdString(a.id));
    if (a.duration_us > 0)
        mc->setEnd(mc->start() + a.duration_us);
    if (Clip* c = locateWorkClip(note))
        c->asset_id = a.id;

    std::fprintf(stderr,
                 "[adapter] 拖入片段 %s（轨道 %d）：导入 %s（时长 %lld us）\n",
                 note.toStdString().c_str(), trackIndex, a.id.c_str(),
                 (long long)a.duration_us);
    std::fflush(stderr);
    emit sceneEdited(); // 终态（绑定+时长）补发；onAddClip 已 emit 过占位版本
}

// ---------------------------------------------------------------------------
// 原生"删除片段"覆写：工作副本先删（业务真相源）→ 基类删模型行
// （基类 delete 模型后行号/指针失效，工作副本定位必须在此之前）
// ---------------------------------------------------------------------------
void QtTimelineModelAdapter::onDeleteClip(QModelIndex clipIndex) {
    // 先按 noteName（= ms clip id）解析待删片段（趁模型行还有效）
    QString note;
    if (clipIndex.isValid() && clipIndex.parent().isValid()) {
        const int track_row = clipIndex.parent().row();
        if (track_row >= 0 && track_row < (int)m_tracks.size() &&
            m_tracks[track_row] && clipIndex.row() >= 0 &&
            clipIndex.row() < (int)m_tracks[track_row]->clips.size())
            note = m_tracks[track_row]->clips[clipIndex.row()]->noteName();
    }

    if (!note.isEmpty() && work_) {
        const std::string id = note.toStdString();
        for (auto& tr : work_->timeline.tracks) {
            auto it = std::find_if(tr.clips.begin(), tr.clips.end(),
                                   [&id](const Clip& c) { return c.id == id; });
            if (it != tr.clips.end()) {
                tr.clips.erase(it);
                dirty_clips_.erase(id); // 脏清单同步摘除（防 sync 阶段复活）
                emit sceneEdited();
                break;
            }
        }
    }
    BaseTimeLineModel::onDeleteClip(clipIndex);
}

} // namespace ms
