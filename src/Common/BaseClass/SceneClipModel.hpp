// ============================================================================
//  SceneClipModel —— 场景片段模型公共基类（编辑器媒体插件体系）
//
//  定位（第四迭代"媒体类型插件化"）：
//    QtTimeLine 的插件体系（ClipPlugInterface → createModel）负责把
//    "一种媒体类型"抽象成"一个插件文件"；本基类把编辑器业务侧的
//    ms::Clip 关键字段镜像进时间轴模型，供：
//      * SceneCanvasWidget 画布渲染（素材引用 / 画布 transform）
//      * 属性对话框联动（双击片段 → 按类型填充表单）
//    时间几何（start/end 帧）仍由 AbstractClipModel 承载，本类只镜像
//    ms 侧的"内容引用 + 画布变换"。
//
//  插件文件约定（与 QtTimeLine clips/ 目录的官方插件同构）：
//      VideoClipPlugin / ImageClipPlugin / TextClipPlugin / AudioClipPlugin
//    每个文件 = XxxClipModel（类型专属模型：默认色 + 类型标志 + 业务镜像）
//             + XxxClipPlugin（ClipPlugInterface 工厂：clipType/createModel）
//    新增媒体类型 = 新增一个插件目录（src/Clips/<Type>/，含 Section 可选）
//    + 编译产出 .plugin 落入 plugin/——纯磁盘驱动，无注册代码改动。
//
//  合并关键帧显示带（2026-11-09，DaVinci Resolve Inspector 模式，用户定案）：
//    曲线可视化与"每通道独立菱形"已退役——片段带改为【合并关键帧】语义：
//    * 本类仍持有 tween 通道的【μs 镜像】（TweenKeyDot/TweenChannelDots，
//      数据层 per-prop 通道不变），由适配器在装载/面板写回后经
//      setTweenChannels 重灌（us→帧换算在适配器侧，模型保持单位无关）；
//    * paint() 只画【全通道键时间并集】的合并菱形：一个时刻 = 一个菱形
//      （单参数键 = 该参数通道色，多参数合并 = 白）——所有参数在时间线上
//      合并为"这一帧有动画"，不再呈现独立通道曲线；
//    * 拖拽菱形 = 移动该时刻【全部通道】的键（时间联动；值只在属性面板
//      改——auto-key 打键，Resolve 同款）；点击空白不再加键（加键唯一
//      入口 = 属性面板改值）；右键菜单 = 删除该帧全部 + 按参数子菜单
//      （appendKeyframeContextMenu，两级删除）；
//    * 命中/拖拽走库的通用钩子（AbstractClipModel::hitKeyframe /
//      moveKeyframe）——绘制与命中共用 frameToNormX() 同源坐标公式
//      （"尺寸同源"纪律）；命中结果 channel = 【合并桶下标】、key 恒 -1；
//    * 落点经 tween_key_frame_move_ 闭包上行适配器写回工作副本（模型不
//      感知适配器，依赖单向：适配器 → 模型，同 panel_factory_）。
// ============================================================================

#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

#include <QObject>
#include <QString>
#include <QColor>
#include <QWidget>
#include <QPolygonF>
#include <QMenu>
#include <QIcon>
#include <QPixmap>

#include "AbstractClipModel.hpp"
#include "TweenPalette.h" // tweenPropShortName/Color（按参数删除菜单；
                          // 时间轴菱形已统一白，色表仅面板/菜单消费）
#include "BaseClassExport.h"

// TweenKeyDot / TweenChannelDots（μs 镜像数据结构）已迁至 TweenDots.h
// （2026-11-05 随 TweenPalette.h 一并独立；行头读数 provider 仍共用）。

/// 场景片段模型公共基类：镜像 ms::Clip 的内容引用与画布变换。
/// （类型子类只负责差异部分：默认色 / 类型标志；几何仍走基类帧字段）
/// 【2026-11-12 收编 BaseClass.dll】类随 BaseClass.dll 导出——全工程唯一
/// staticMetaObject（原 header-only"每插件 DLL 各 moc 一份"语义退役，
/// qobject_cast 跨模块恢复可用；磁盘插件形态仍可 dynamic_cast）。
class BASECLASS_EXPORT SceneClipModel : public AbstractClipModel {
    Q_OBJECT
public:
    /// \param startUs   时间轴起始时刻（μs；2026-11-10 微秒原生）
    /// \param type      媒体类型串（与插件 clipType 一致："Video"…）
    /// \param asset_id  引用的素材 id（ms::Asset.id；空 = 未绑定）
    /// \param parent    Qt 父对象（适配器统一接管生命周期）
    SceneClipModel(qint64 startUs, const QString& type,
                   const QString& asset_id = {}, QObject* parent = nullptr)
        : AbstractClipModel(startUs, type, parent), asset_id_(asset_id) {}

    /// 业务镜像：引用的素材 id（画布渲染/素材分发依赖计算用）
    QString assetId() const { return asset_id_; }
    void    setAssetId(const QString& id) { asset_id_ = id; }

    // ---- 片段自定义属性面板（QtTimeLine 原生附加点）--------------------------
    // 库在通用面板（时间/颜色/备注）下方的 contentLayout 里附加
    // clipPropertyWidget() 返回的控件（懒创建并缓存于基类 m_clipPropertyWidget）。
    // 面板的构造依赖宿主上下文（素材候选/导入/写回回调），经工厂回调注入——
    // 模型本身不感知 EditorWindow/适配器（依赖单向：适配器 → 模型）。
    /// 注入面板工厂（适配器创建片段模型时设置；空 = 无自定义面板）
    void setClipPanelFactory(std::function<QWidget*()> factory) {
        panel_factory_ = std::move(factory);
    }
    /// 库回调：返回片段自定义属性面板（素材绑定 + 画布变换）；未注入返回 null
    QWidget* clipPropertyWidget() override {
        return panel_factory_ ? panel_factory_() : nullptr;
    }

    // ---- 合并关键帧显示带（Resolve 模式；2026-11-09）------------------------
    /// 键显示带高度（px，片段绘制矩形底部向上）——绘制与命中共用
    static constexpr int kKeyStripHeight = 30;
    /// 命中半径（px，欧氏距离；菱形绘制半径 3~4，命中放宽保证好抓）
    static constexpr double kKeyHitRadiusPx = 6.0;

    /// 重建关键帧镜像（适配器在装载/面板写回后调用）。emit appearanceChanged
    /// → BaseTimeLineModel::watchClip → dataChanged → 视图 updateClipGeometry
    /// 局部重绘（菱形变化即时不整页闪烁）。
    void setTweenChannels(std::vector<TweenChannelDots> channels) {
        tween_dots_ = std::move(channels);
        emit appearanceChanged();
    }
    const std::vector<TweenChannelDots>& tweenChannels() const { return tween_dots_; }
    /// 是否存在任何关键帧（无关键帧 = 不画带/不命中，绘制零开销）
    bool hasTweenKeys() const {
        for (const auto& ch : tween_dots_)
            if (!ch.keys.empty()) return true;
        return false;
    }
    /// 注入关键帧【合并移动】闭包（拖拽路径，高频；适配器注入）。
    /// 语义：把时刻 old_frame 处全部通道的键移动到 new_frame（Resolve 合并
    /// 关键帧语义：拖一个菱形 = 全参数联动）。返回 false = 适配器拒绝
    ///（如片段已删）；拖拽高频调用可忽略返回值（镜像先行跟手，拒绝时
    /// 下一拍重灌自然对齐）。
    void setTweenKeyFrameMoveHandler(
            std::function<bool(double old_frame, double new_frame)> handler) {
        tween_key_frame_move_ = std::move(handler);
    }
    /// 注入关键帧【删除】闭包（单通道单键：右键"按参数删除"子菜单路径，
    /// 低频）。channel/key = 镜像下标（与工作副本下标恒对应，适配器按
    /// noteName 定位）。返回 false = 拒绝。
    void setTweenKeyRemoveHandler(
            std::function<bool(int channel, int key)> handler) {
        tween_key_remove_ = std::move(handler);
    }

    // ---- AbstractClipModel 绘制/交互钩子覆写 --------------------------------

    /// 片段绘制：背景 → 键显示带（暗带 + 合并菱形）→ 标题（限制在上部
    /// 区域，避免被键带覆盖；基类 drawTitle 是整矩形居中，直接传上部矩形
    /// 即可复用其字体/省略逻辑）。
    void paint(QPainter* painter, const QRect& rect, bool selected) const override {
        drawBackground(painter, rect, selected); // 先画既有外观（背景+边框）
        const bool paintable = hasTweenKeys() && canHitKeys(rect);
        if (paintable) {
            painter->save();
            // 带内几何统一【局部坐标系】（与 hitKeyframe 的 local 契约同系）
            // ——translate 后 band/菱形全按相对坐标画。drawBackground/drawTitle
            // 在平移外用绝对 rect（基类契约）。
            painter->translate(rect.topLeft());
            const QRect band = keyBand(rect);
            // 底部半透明暗带：给键区一个稳定的衬底（浅色片段填充上也看得清）
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0, 0, 0, 70));
            painter->drawRect(band);

            // 合并关键帧菱形：全通道键时间并集（一个时刻 = 一个菱形）。
            // 颜色语义（2026-11-09 v2 用户定案）：菱形【不分属性、统一样式】
            // ——全部白色（0xF2F2F2），属性区分只存在于右键"按参数删除"
            // 菜单（色块+属性名）。菱形形状统一（Hold 方块区分已随曲线
            // 退役——插值语义呈现入口留待后续版本）。
            const int r = selected ? 4 : 3; // 选中时标记放大一号（WATCHOUT 选中语义）
            const double len_f = static_cast<double>(qMax(1, length()));
            const int cy = band.center().y();
            const QColor diaColor(0xF2, 0xF2, 0xF2); // 合并关键帧·统一白
            for (const double bf : mergedKeyFrames()) {
                if (bf > len_f) continue; // 剪裁遮蔽：出点外不画（数据保留）
                if (channelKeysAt(bf).empty()) continue;
                const int cx = qRound(frameToNormX(bf, rect.width()));
                painter->setPen(QPen(QColor(0, 0, 0, 160), 1)); // 深描边：任何填充色上都可辨
                painter->setBrush(diaColor);
                QPolygonF dia;
                dia << QPointF(cx, cy - r) << QPointF(cx + r, cy)
                    << QPointF(cx, cy + r) << QPointF(cx - r, cy);
                painter->drawPolygon(dia);
            }
            painter->restore();
        }
        // 标题最后画（永远在键带之上）；矩形限制在键带上方，基类
        // drawTitle 的 AlignCenter 于是落在上部标题区而非键带中间。
        const QRect titleRect = paintable
            ? QRect(rect.left(), rect.top(), rect.width(), rect.height() - kKeyStripHeight)
            : rect;
        drawTitle(painter, titleRect);
    }

    /// 合并菱形命中：仅底部键带参与（标题区仍是片段拖拽），按【合并桶】
    /// 最近匹配——水平距离为主（菱形恒在带垂直中心一行，垂直分量仍参与
    /// 欧氏距离防误抓带内空白）。返回 channel = 桶下标、key = -1（合并
    /// 模式无单通道键语义；moveKeyframe/removeKeyframe 按此解释）。
    KeyframeHit hitKeyframe(const QPoint& local, const QRect& rect) const override {
        if (!canHitKeys(rect) || !hasTweenKeys())
            return {};
        const QRect band = keyBand(rect);
        // 垂直约束：只认底部键带（含 2px 宽容；上方整块留给片段拖拽/边缘 resize）
        if (local.y() < band.top() - 2)
            return {};
        const auto buckets = mergedKeyFrames();
        const double len_f_hit = static_cast<double>(qMax(1, length()));
        int best = -1;
        double best_d = kKeyHitRadiusPx; // 命中半径契约：欧氏距离 ≤ kKeyHitRadiusPx
        for (int b = 0; b < static_cast<int>(buckets.size()); ++b) {
            if (buckets[b] > len_f_hit) continue; // 剪裁遮蔽：出点外不可命中
            const double dx = frameToNormX(buckets[b], rect.width()) - local.x();
            const double dy = static_cast<double>(band.center().y() - local.y());
            const double d  = std::sqrt(dx * dx + dy * dy);
            if (d < best_d) {
                best_d = d;
                best = b;
            }
        }
        if (best < 0)
            return {};
        return { true, best, -1 };
    }

    /// 合并菱形拖拽（Resolve 语义）：水平 = 帧移动【该时刻全部通道的键】
    /// （值只在属性面板改——垂直分量忽略）。钳制窗口 = 桶级：片段内
    /// [0, length] ∩ (前桶+1, 后桶-1)——桶序保持 ⇒ 通道内键序保持
    ///（loader R8 纪律：键序升序、同通道无同刻键），无需逐通道邻键钳制。
    /// 镜像先行（拖拽跟手依赖），再经注入闭包写回工作副本（适配器做
    /// 帧量化 + sceneEdited）。
    void moveKeyframe(int channel, int key, double localFrame,
                      double localY = -1.0, const QRect& rect = QRect()) override {
        Q_UNUSED(key);     // 合并模式恒 -1（无单通道键下标语义）
        Q_UNUSED(localY);  // 垂直改值已退役：值唯一编辑入口 = 属性面板 auto-key
        Q_UNUSED(rect);
        const auto buckets = mergedKeyFrames();
        if (channel < 0 || channel >= static_cast<int>(buckets.size()))
            return;
        const double old_f = buckets[channel];
        // 桶级钳制窗口：片段内 [0, length] ∩ (前桶+1μs, 后桶-1μs)
        double lo = 0.0;
        double hi = static_cast<double>(qMax(1, length()));
        if (channel > 0)                lo = qMax(lo, buckets[channel - 1] + 1.0);
        if (channel + 1 < (int)buckets.size())
                                        hi = qMin(hi, buckets[channel + 1] - 1.0);
        if (hi < lo) hi = lo; // 邻桶贴脸：钉在原位
        const double new_f = qBound(lo, localFrame, hi);
        const double delta = new_f - old_f;
        if (delta == 0.0) return;

        // 镜像先行（跟手）：所有通道中该时刻的键同步平移（同通道同刻键
        // 不存在——R8 纪律，故每通道至多一键命中）
        const long long old_round = std::llround(old_f);
        for (auto& ch : tween_dots_)
            for (auto& k : ch.keys)
                if (std::llround(k.t_us) == old_round) k.t_us += delta;

        // 工作副本落盘（适配器做帧量化 + 片段区间钳制 + sceneEdited）；
        // 拒绝时下一拍 syncClipTweenDots 重灌自然对齐（拖拽高频路径）。
        if (tween_key_frame_move_)
            tween_key_frame_move_(old_f, new_f);
    }

    /// 点击带内空白不加键（2026-11-09 合并关键帧模式退役该入口）：加键
    /// 唯一路径 = 属性面板改值 auto-key（播放头定位 + 值框输入）。保留
    /// 虚函数覆写仅为显式声明语义（基类默认亦返回 {}）。
    KeyframeHit addKeyframeAt(const QPoint& local, const QRect& rect) override {
        Q_UNUSED(local);
        Q_UNUSED(rect);
        return {};
    }

    /// 删除单通道单键（右键"按参数删除"子菜单路径）：适配器先行确认
    ///（工作副本删键/删空通道），成功后镜像同删（保持下标对应）。空通道
    /// 删除会改变通道总数——后续命中/绘制自然按新集合进行，无悬留下标
    ///（操作已结束才删）。
    bool removeKeyframe(int channel, int key) override {
        if (channel < 0 || channel >= static_cast<int>(tween_dots_.size()))
            return false;
        const auto& keys = tween_dots_[channel].keys;
        if (key < 0 || key >= static_cast<int>(keys.size()))
            return false;
        if (tween_key_remove_ && !tween_key_remove_(channel, key))
            return false; // 适配器拒绝（片段已删等）：不动镜像
        auto& mut_keys = tween_dots_[channel].keys;
        mut_keys.erase(mut_keys.begin() + key);
        if (mut_keys.empty())
            tween_dots_.erase(tween_dots_.begin() + channel); // 空通道删除（R8 非空纪律）
        emit appearanceChanged();
        return true;
    }

    /// 合并关键帧右键菜单（Resolve 两级删除，2026-11-09 用户定案）：
    /// 「删除关键帧（全部参数）」+「按参数删除」子菜单（该时刻有键的参数
    /// 逐项，通道色点 + 短名）。动作触发即删（removeKeyframe 内部 emit
    /// appearanceChanged 驱动视图局部重绘，无需宿主参与）。
    bool appendKeyframeContextMenu(QMenu& menu, const KeyframeHit& kf) override {
        const auto buckets = mergedKeyFrames();
        if (kf.channel < 0 || kf.channel >= static_cast<int>(buckets.size()))
            return false;
        const double bf = buckets[kf.channel]; // kf.key 恒 -1：合并桶下标语义
        const double len_f = static_cast<double>(qMax(1, length()));
        if (bf > len_f) return false; // 剪裁遮蔽：出点外不可操作
        const auto cks = channelKeysAt(bf);
        if (cks.empty()) return false;

        QAction* del_all = menu.addAction(QObject::tr("删除关键帧（全部参数）"));
        del_all->setIcon(QIcon(":/icons/icons/trash.png"));
        QObject::connect(del_all, &QAction::triggered, [this, bf]() {
            removeMergedKeyAt(bf);
        });
        if (cks.size() > 1) {
            QMenu* sub = menu.addMenu(QObject::tr("按参数删除"));
            for (const auto& [ch, kidx] : cks) {
                const int prop = tween_dots_[ch].prop;
                QPixmap pm(10, 10);
                pm.fill(tweenPropColor(prop));
                QAction* a = sub->addAction(
                    QString::fromUtf8("%1 键").arg(tweenPropShortName(prop)));
                a->setIcon(QIcon(pm));
                // 下标捕获（菜单模态 exec 内同步触发，无中间态风险）
                QObject::connect(a, &QAction::triggered, [this, ch, kidx]() {
                    removeKeyframe(ch, kidx);
                });
            }
        }
        return true;
    }

    /// 删除该时刻全部通道的键（右键"删除关键帧（全部参数）"路径；公开
    /// 供菜单与单测直调）。从后往前遍历（通道/键双逆序——removeKeyframe
    /// 删空通道会 shift 后续通道下标，逆序保证下标不失效）。返回是否删除
    /// 了至少一键。
    bool removeMergedKeyAt(double frame) {
        bool any = false;
        for (int ch = static_cast<int>(tween_dots_.size()) - 1; ch >= 0; --ch) {
            auto& keys = tween_dots_[ch].keys;
            for (int k = static_cast<int>(keys.size()) - 1; k >= 0; --k) {
                if (std::llround(keys[k].t_us) != std::llround(frame)) continue;
                if (removeKeyframe(ch, k))
                    any = true;
                break; // 该通道该时刻至多一键
            }
        }
        return any;
    }

private:
    /// 合并关键帧时刻并集（升序去重；μs 取整桶化——镜像键 t_us 与工作
    /// 副本同基，拖拽中的小数残余在 llround 后并桶）。绘制/命中/拖拽/菜单
    /// 四处共用同一函数——"一处聚合、处处同源"。
    std::vector<double> mergedKeyFrames() const {
        std::vector<double> frames;
        for (const auto& ch : tween_dots_)
            for (const auto& k : ch.keys)
                frames.push_back(static_cast<double>(std::llround(k.t_us)));
        std::sort(frames.begin(), frames.end());
        frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
        return frames;
    }
    /// 某时刻（帧取整后）各通道的键下标列表（同通道同刻至多一键——R8
    /// 重复时间点纪律）。右键菜单/合并删除按此定位（channel, key_idx）。
    std::vector<std::pair<int, int>> channelKeysAt(double frame) const {
        const long long f = std::llround(frame);
        std::vector<std::pair<int, int>> out;
        for (int ch = 0; ch < static_cast<int>(tween_dots_.size()); ++ch) {
            const auto& keys = tween_dots_[ch].keys;
            for (int k = 0; k < static_cast<int>(keys.size()); ++k) {
                if (std::llround(keys[k].t_us) == f) {
                    out.emplace_back(ch, k);
                    break; // 同通道同刻至多一键
                }
            }
        }
        return out;
    }

private:
    /// 曲线编辑带几何（片段绘制矩形 → 底部带）。paint/hit/add 共用同源公式。
    /// 【坐标系铁律（2026-11-05 探针实证）】band 一律是【相对片段矩形左上角】
    /// 的局部坐标——hitKeyframe/addKeyframeAt/moveKeyframe 收到的 local/
    /// localY 都是视图契约的相对坐标（pos - clipRect.topLeft()）；paint 侧
    /// 由 paint() 开头 translate(rect.topLeft()) 后同样落入局部系。此前
    /// band.top 用 rect.bottom()（绝对值）导致"局部输入 vs 绝对几何"纵向
    /// 错位：单测用 (0,0) 原点矩形恰好两系重合恒绿，真实视图（rect.top≠0）
    /// 命中/加键/拖值全部静默失效——探针网格扫描抓出。
    static QRect keyBand(const QRect& rect) {
        return QRect(1, rect.height() - kKeyStripHeight,
                     qMax(1, rect.width() - 2), kKeyStripHeight);
    }
    /// 命中/加键的前置条件（带存在 + 几何足够）
    static bool canHitKeys(const QRect& rect) {
        return rect.width() >= 12 && rect.height() > kKeyStripHeight + 2;
    }
    /// 关键帧 x 坐标（相对 rect.left 的偏移，帧→带内线性映射）。
    /// paint 与 hitKeyframe/addKeyframeAt 必须同用此函数——两端各写一份一旦
    /// 漂移就是"画在 A 点、命中在 B 点"的静默错位。
    double frameToNormX(double frame, int rect_width) const {
        const double len = static_cast<double>(qMax(1, length()));
        const double u   = qBound(0.0, frame / len, 1.0);
        return 1.0 + u * static_cast<double>(qMax(1, rect_width - 2));
    }
    // 【2026-11-09 合并关键帧模式退役】normXToFrame（加键反查）、
    // valueToBandY/bandNormToValue/localYToValueNorm（值域几何）、
    // drawChannelCurve（per-prop 曲线）、kBandYPad——曲线可视化与垂直
    // 改值交互一并退役；行头读数的同源换算仍走 TweenPalette（适配器侧）。

private:
    QString asset_id_; // ms::Clip.asset_id 镜像（ms 侧仍是权威真相源）
    std::function<QWidget*()> panel_factory_; // 属性面板工厂（适配器注入）
    // ---- Tween 关键帧镜像（帧域；适配器重灌，模型不自写 us 换算）----------
    std::vector<TweenChannelDots> tween_dots_;
    /// 关键帧写回闭包组（适配器注入；空 = 编辑只动镜像不落盘——单测场景）。
    /// 两个操作各一个注入点（合并移动/单通道删除语义不同，签名即文档）。
    /// 【合并移动】old_frame → new_frame：工作副本侧把该时刻全部通道的键
    /// 平移（桶级钳制已保证 R8 不变量，适配器只做片段区间钳制）。
    std::function<bool(double old_frame, double new_frame)>
        tween_key_frame_move_;
    std::function<bool(int channel, int key)> tween_key_remove_;
};
