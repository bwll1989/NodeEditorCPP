// ============================================================================
//  TweenSection 实现 —— 变换与关键帧合并编辑区（方案 A + v3 五分组）
//
//  交互约定（与 TextClipSection / ClipPropertyPanel 同源）：
//    * 即时生效：菱形启停/值框编辑/重置 → applier()（适配器按 clip id
//      定位工作副本写回 c.tweens + transform 静态脏字段 + sceneEdited）；
//    * 值框【双模直写·打包 auto-key】——通道存在时改任一分量即整条
//      属性在播放头处写/改关键帧（一个键携带 v0/v1/v2 全部分量：编辑分量
//      取新值、其余分量取该行值框当前显示值=播放头处插值结果；与画布
//      拖拽打键同构，自动下发由 auto-publish 防抖合并）；无通道时改值
//      = 写静态基准 transform（分量级脏位合并）；
//    * 新关键帧插值恒 Linear（2026-11-09 v2 定案：行尾「插值▾」删除，
//      数据 per-key interp 字段保留——文件/协议/引擎不变）；
//    * ⟲ 重置钮（v3 DaVinci 同款）：值框灌回默认值后双模落盘——有通道
//      = 播放头处打包写默认值键（动画上"拉回默认"）；无通道 = 静态基准
//      直写默认值。与值框同一条双模纪律；尺寸行默认值 = 素材原始尺寸
//      （setSizeDefaultHint 注入，缺失时不动作）；
//    * 预填（构造）在信号连接之前——setValue 不触发写回；灌值
//      （refreshFrom）QSignalBlocker 包裹 + 焦点保护——灌显示与用户编辑
//      两条链路分离，不回环。
// ============================================================================

#include "TweenSection.hpp"

#include "AbstractClipInterface.h" // ClipTweenExposure 完整定义（P4-A 暴露面 schema）

#include <algorithm>
#include <cmath> // std::lround（Z 静态层级写回取整）
#include <cstdint>

#include <QCoreApplication>
#include <QComboBox>
#include <QDebug> // qWarning 流式（暴露面未知 key 漂移告警）
#include <QFontMetrics>
#include <QGridLayout>
#include <QHash> // resetGlyphIcon 按 dpr 缓存（UI 单线程，无锁）
#include <QLabel>
#include <QPainter>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath> // qDegreesToRadians（⟲ 弧端点/箭头三角计算）

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
#include "engine/TweenEngine.hpp"
#include "ClipSchema.h"    // P4-D：声明性数据唯一事实源（ms::kTweenSchema）
#include "TweenPalette.h" // tweenPropColor（行指示色与曲线同源；经 BaseClass include 根解析）

namespace ms {

namespace {

// ---------------------------------------------------------------------------
// 属性行定义表（行序即 UI 行序，也是 prop_index 语义）。
// 【P4-D 泛化 2026-11-18】声明性数据（显示名/值域/步长/decimals/后缀/
// 分量名/默认值/分组）已迁 BaseClass ms::kTweenSchema 唯一事实源——本表
// 瘦身为【通道映射 + UI 测试锚】三层本地知识：
//   * prop        = v3 打包属性（MediaCore TweenProp——通道寻址；
//                   BaseClass schema 不引 MediaCore，映射只能留本表）
//   * key         = schema join 键（ms::kTweenSchema 同行 key，static_assert
//                   锁两表行序与词汇不漂移）
//   * obj_suffix  = objectName 后缀（tween<suffix><分量>——QTest/UI
//                   定位锚，纯 UI 域词汇不进 schema）
// 行注释的行语义（打包分量构成/值域特判/重置语义）见原 kPropOrder 全表
// 注释（git 历史）与 ms::kTweenSchema 行注释（ClipSchema.h）。
// ---------------------------------------------------------------------------
struct PropDef {
    TweenProp   prop;       ///< v3 打包属性（Position/Size/…/CropB）
    const char* key;        ///< schema join 键（对齐 ms::kTweenSchema）
    const char* obj_suffix; ///< objectName 后缀（tween<suffix><分量>）
};
constexpr PropDef kPropOrder[15] = {
    {TweenProp::Position,   "position",   "Pos"},
    {TweenProp::Size,       "size",       "Size"},
    {TweenProp::Rotation,   "rotation",   "Rot"},
    {TweenProp::CropT,      "crop_top",   "CropT"},
    {TweenProp::CropB,      "crop_bottom", "CropB"},
    {TweenProp::CropL,      "crop_left",  "CropL"},
    {TweenProp::CropR,      "crop_right", "CropR"},
    {TweenProp::CornerLT,   "corner_lt",  "CornerLT"},
    {TweenProp::CornerRT,   "corner_rt",  "CornerRT"},
    {TweenProp::CornerRB,   "corner_rb",  "CornerRB"},
    {TweenProp::CornerLB,   "corner_lb",  "CornerLB"},
    {TweenProp::Opacity,    "opacity",    "Opa"},
    {TweenProp::AudioGain,  "audio_gain", "Gain"},
    {TweenProp::RgbGain,    "rgb_gain",   "RgbGain"},
    {TweenProp::Gamma,      "gamma",      "Gamma"},
};
/// 【两表对齐护栏】kPropOrder 行序/key 必须与 ms::kTweenSchema 逐行一致
///（buildRow 以行下标同时索引两表——漂移即静默错行）。constexpr 自写
/// 逐字符比较（qstrcmp 非 constexpr；字面量编译期可读）。
constexpr bool schemaKeysAligned() {
    for (int i = 0; i < 15; ++i) {
        const char* a = kPropOrder[i].key;
        const char* b = ms::kTweenSchema[i].key;
        while (*a != '\0' && *b != '\0' && *a == *b) { ++a; ++b; }
        if (*a != *b) return false;
    }
    return true;
}
static_assert(schemaKeysAligned(),
              "kPropOrder 与 ms::kTweenSchema 行序/key 漂移（P4-D 两表契约）");
/// 行首槽位表（静态暂存 static_vals_ 与脏位的槽位基准——行槽数 =
/// components，数据槽 0..25 共 26 个；末项 = 总槽数哨兵；锚点脏位
/// 避开全部数据槽，见 kSAnchor）
constexpr int kRowSlot[16] = {
    0,  // 行 0 位置 (x,y,z)        → 0..2（UI 两框 X/Y；槽 2 = 层级，
        //                          独立「层级 Z」行写同槽——applyTo 路径不变）
    3,  // 行 1 尺寸 (w,h)         → 3..4
    5,  // 行 2 旋转 (俯仰,偏航,滚转) → 5..7（v4 三维打包）
    8,  // 行 3 裁切上             → 8
    9,  // 行 4 裁切下             → 9
    10, // 行 5 裁切左             → 10
    11, // 行 6 裁切右             → 11
    12, // 行 7 角点LT (2)         → 12..13
    14, // 行 8 角点RT (2)         → 14..15
    16, // 行 9 角点RB (2)         → 16..17
    18, // 行 10 角点LB (2)        → 18..19
    20, // 行 11 不透明度          → 20
    21, // 行 12 音频增益（2026-11-10） → 21
    22, // 行 13 RGB 增益 (R,G,B)（2026-11-20） → 22..24
    25, // 行 14 Gamma（2026-11-20）   → 25
    26, // 哨兵 = 总槽数
};
/// 行序+分量 → 静态脏位（槽位即位号）
constexpr std::uint32_t sBit(int row, int comp) {
    return 1u << (kRowSlot[row] + comp);
}
/// 锚点（追加区静态属性）脏位——避开 26 个分量槽位
constexpr std::uint32_t kSAnchor = 1u << 26;
/// 深度 Z（2026-11-17 阶段③追加区静态属性）脏位——避开 26 槽 + 锚点位
constexpr std::uint32_t kSPosZ = 1u << 27;
/// 遮挡次序（2026-11-17 阶段③追加区静态属性）脏位
constexpr std::uint32_t kSStacking = 1u << 28;

/// 重置默认值（v3 ⟲ 语义）：位置 (0,0,0)、旋转三轴 0、裁切 0、
/// 角点 0、不透明度 1（DaVinci「恢复默认值」；锚点=中心在重置钮单独
/// 处理；尺寸行不走本表——语义=素材原始尺寸，见 onResetClicked 特判）。
/// 【P4-D 迁移 2026-11-18】默认值事实已迁 ms::FieldDef::default_value
///（ClipSchema.h）——本表退役，重置路径改读 schema 行。

/// 关键帧删除/编辑的"播放头处"判定容差（±半帧 @25fps = 20ms）——播放头
/// 帧量化（40000us/帧）与场景文件任意 us 关键帧之间的编辑容差
constexpr std::int64_t kKeyHitToleranceUs = 20'000;

/// 行尾小图标统一绘制（2026-11-09 QIcon pixmap 同源方案）：○/◇/◆ 三态
/// 菱形与 ⟲ 恢复弧全部由 QPainter 画进同规格方形 pixmap。字符方案两轮
/// 教训：不同 Unicode 区字形在字体中的占位/基线不可控——先是 QCheckBox
/// 指示器比 ⟲ 字形大 40%，换字符后 ⟲(U+27F2) 字形又反超 ◆(U+25C6) 且
/// 垂直中心错位（用户反馈"reset 比 key 高、不在同一行"）；pixmap 绘制
/// 像素级等大、居中一致、跨字体/跨平台稳定（DaVinci 原版即位图图标）。
constexpr int kTailBtnPx  = 18; // 行尾钮占位边长（菱形/⟲ 统一）
constexpr int kTailIconPx = 12; // 图标视觉边长（@18x18 钮内居中）

/// 高 DPI 感知的透明底 pixmap 绘制：dpr 从调用方控件取（devicePixelRatioF
/// ）——offscreen 探针 dpr=1.0、真机缩放 1.25/1.5 下边缘同样锐利。
/// 绘制区 0.5px 内缩防抗锯齿边缘被裁。
inline QPixmap tailIconPixmap(qreal dpr,
        const std::function<void(QPainter&, const QRectF&)>& draw) {
    const int px = int(std::lround(kTailIconPx * dpr));
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    // 0.5px 内缩：描边居中在 [0.5, size-0.5] 不越界
    draw(p, QRectF(0.5, 0.5, kTailIconPx - 1.0, kTailIconPx - 1.0));
    return pm;
}

/// 菱形三态图标：○ 未启用（灰描边圆）/ ◇ 启用无键（通道色描边）/
/// ◆ 当前帧有键（通道色填充）——旋转正方形对角线 = 圆直径，三态等大
inline QIcon diamondIcon(qreal dpr, int st, const QColor& edge) {
    return QIcon(tailIconPixmap(dpr, [st, edge](QPainter& p, const QRectF& rc) {
        p.setBrush(st == 2 ? QBrush(edge) : Qt::NoBrush);
        p.setPen(QPen(edge, 1.4));
        if (st == 0) {
            p.drawEllipse(rc); // ○
        } else {               // ◇/◆：正方形旋转 45°，对角线外接于 rc
            p.save();
            p.translate(rc.center());
            p.rotate(45.0);
            const qreal s = rc.width() / std::sqrt(2.0);
            p.drawRect(QRectF(-s / 2.0, -s / 2.0, s, s));
            p.restore();
        }
    }));
}

/// ⟲ 恢复图标（逆时针开口圆弧 + 实心箭头三角，缺口朝右上——DaVinci
/// 「恢复默认值」同款观感；灰 #888 与未启用菱形同灰阶）
/// 【缓存 2026-11-20】结果只依赖 dpr（UI 单线程访问，QHash 无需锁）；
/// 每面板 3+ 个 ⟲ 钮、每次 refreshFrom 重灌都重画（QPainter 现画约
/// 0.5ms/个）——按 dpr 缓存 QIcon（隐式共享，返回零成本）后重复装配
/// 零绘制。键=qreal dpr（实际取值仅 1.0/1.25/1.5 等少数档）。
inline QIcon resetGlyphIcon(qreal dpr) {
    static QHash<qreal, QIcon> cache;
    if (auto it = cache.constFind(dpr); it != cache.cend()) return *it;
    const QColor col(0x88, 0x88, 0x88);
    QIcon ic(tailIconPixmap(dpr, [col](QPainter& p, const QRectF& rc) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(col, 1.4, Qt::SolidLine, Qt::RoundCap));
        const QRectF arc = rc.adjusted(1.0, 1.0, -1.0, -1.0); // 弧径略缩
        // 开口圆弧：从 70° 逆时针扫 315° 终于 25°（缺口 25°..70° 朝右上）
        constexpr qreal kStartDeg = 70.0, kSpanDeg = 315.0;
        p.drawArc(arc, int(kStartDeg * 16), int(kSpanDeg * 16));
        // 箭头：弧末端（25° 方位点）沿逆时针切线画实心小三角——
        // P(θ)=c+(r·cosθ,−r·sinθ)（屏幕 y 向下）；逆时针切线 T=(−sinθ,−cosθ)
        const QPointF c  = arc.center();
        const qreal   rr = arc.width() / 2.0;
        const qreal   e  = qDegreesToRadians(kStartDeg + kSpanDeg); // ≡25°
        const QPointF ep(c.x() + rr * std::cos(e), c.y() - rr * std::sin(e));
        const QPointF T(-std::sin(e), -std::cos(e));
        const QPointF N(-T.y(), T.x());
        QPolygonF tri;
        tri << ep + T * 3.0 << ep + N * 1.9 << ep - N * 1.9;
        p.setBrush(QBrush(col));
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
    }));
    cache.insert(dpr, ic);
    return ic;
}

/// 菱形开关钮三态视觉（QIcon pixmap 方案，见 tailIconPixmap 注释）：
/// ○=未启用（灰）/ ◇=已启用但当前帧无键（通道色描边）/ ◆=当前帧有键
///（通道色实心）。状态存 property("st") 0/1/2（测试断言锚）；通道色
/// 由 buildRow 预存 property("edge")（setDiamondVisual 取色重绘图标）。
/// 状态未变化时跳过重绘（refreshFrom 每次广播灌 12 行、播放头移动期间
/// 高频调用——pixmap 重建含 QPainter/抗锯齿，不便宜）。
inline void setDiamondVisual(QToolButton* b, int st) {
    const QVariant cur = b->property("st");
    if (cur.isValid() && cur.toInt() == st) return; // 未变化：跳过重绘
    b->setProperty("st", st);
    QColor edge = b->property("edge").value<QColor>();
    if (!edge.isValid()) edge = QColor(0x80, 0x80, 0x86); // 未启用灰
    // ○ 未启用恒灰（视觉降级）——启用/有键才穿通道色（三态=形状+色双编码：
    // 圆=未启用 / 菱形描边=启用无键 / 菱形实心=有键）
    b->setIcon(diamondIcon(b->devicePixelRatioF(), st,
                           st == 0 ? QColor(0x80, 0x80, 0x86) : edge));
}

} // namespace

// ---------------------------------------------------------------------------
// 构造：五折叠组（变换 锚点+2 行展开 / 其余四组收）+ 预填 + 信号连接
//（预填在 connect 之前，不触发写回）。P4-A：暴露面 schema（插件
// tweenExposure() 声明）过滤属性行——未暴露行【不生成】（Row 骨架仍占
// 13 行定长契约），空组整组隐藏，锚点随组 0 同进退。
// ---------------------------------------------------------------------------
TweenSection::TweenSection(const Clip& clip, const ClipTweenExposure& expo,
                           QWidget* parent)
    : QWidget(parent) {
    clip_start_us_ = clip.start_us;
    clip_dur_us_   = clip.duration_us;
    channels_      = clip.tweens;

    // ---- 暴露面 schema（P4-A）：白名单 → 13 行暴露位 ----------------------
    // 空白名单 = 全暴露（既有 13 行行为，Video/Image/Text/Live/Mapping 零
    // 改动）；page_visible=false 防御性同义化全不暴露——适配器已在构造前
    // 整页拦截，此处兜底防直构出满页控件。未知 key = schema 漂移信号。
    if (expo.page_visible && expo.exposed_keys.isEmpty()) {
        for (bool& e : row_exposed_) e = true;
    } else {
        for (bool& e : row_exposed_) e = false;
        for (const QString& k : expo.exposed_keys) {
            bool matched = false;
            for (int i = 0; i < 15; ++i)
                if (k == QLatin1String(kPropOrder[i].key)) {
                    row_exposed_[i] = true;
                    matched = true;
                    break;
                }
            if (!matched)
                qWarning() << "tween-schema: 插件暴露面含未知 key（schema "
                              "漂移？）" << k;
        }
    }

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(4);

    // 【2026-11-13 三页式改版】本区块独立成「动画」页——页签承载标题，
    // 原 QGroupBox("动画") 框/线退役改普通容器（标题重复去冗余）
    auto* box = new QWidget(this);
    auto* lay = new QVBoxLayout(box);
    lay->setContentsMargins(6, 4, 6, 6);
    lay->setSpacing(4);

    // ---- 交互提示行已删（2026-11-13 用户定案：面板空间宝贵，说明文字
    //      去除——语义保留在各分组标题 tooltip 与属性行 tooltip）----------

    // ---- 折叠组（v3 五分组）：标题钮工厂 + 组装配。
    //      折叠 = 隐藏组内行容器（VBox 对 hidden 控件不保留占位），
    //      控件/通道/写回全链路不感知可见性——测试编程操作不受影响 ----
    auto makeGroupHeader = [&](const char* obj_name, const QString& title,
                               const QString& tip,
                               bool expanded) -> QToolButton* {
        auto* btn = new QToolButton(box);
        btn->setObjectName(QString::fromUtf8(obj_name));
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
        btn->setText(title);
        btn->setCheckable(true);
        btn->setChecked(expanded);
        btn->setToolTip(tip); // 折叠时唯一的语义说明位
        // 展开样式（2026-11-13 用户「再优化下展开样式」）：checked 态标题
        // 加深——当前展开组一目了然（收起组保持灰）；margin-top 拉开组间
        // 间隔（组内行距 3px < 标题-内容 6px < 组间 ~10px 三级递进）
        btn->setStyleSheet(
            QStringLiteral("QToolButton{border:none;text-align:left;"
                           "color:#5f5f5f;font-size:11px;padding:1px;"
                           "margin-top:4px;}"
                           "QToolButton:hover{color:#000;}"
                           "QToolButton:checked{color:#111;}"));
        return btn;
    };

    // ⟲ 重置钮工厂（v3 行尾；2026-11-10 新语义「删全部 key 帧+恢复默认」
    // ——见 onResetClicked；尺寸行恢复目标 = 素材原始尺寸，tooltip 特判）。
    // 固定 18x18 占位（与菱形钮统一）；图标为同规格
    // pixmap（⟲ 与 ◆ 视觉等大、垂直中心一致——2026-11-09 用户第三轮
    // 反馈"reset 比 key 高且不在同一行"根治）
    const QString tailBtnQss = QStringLiteral(
        "QToolButton{border:none;background:transparent;padding:0;}"
        "QToolButton:hover{background:rgba(0,0,0,24);}"); // hover 淡底反馈
    auto makeResetButton = [&](const PropDef& def) -> QToolButton* {
        auto* btn = new QToolButton(box);
        btn->setObjectName(
            QStringLiteral("tweenReset%1").arg(def.obj_suffix));
        btn->setFixedSize(kTailBtnPx, kTailBtnPx);
        btn->setIconSize(QSize(kTailIconPx, kTailIconPx)); // 防 16px 默认放大
        btn->setIcon(resetGlyphIcon(btn->devicePixelRatioF()));
        btn->setToolTip(
            def.prop == TweenProp::Size
                ? tr("重置尺寸：删除尺寸上全部关键帧（停用动画），"
                     "值恢复素材原始尺寸（绑定素材的像素宽高）")
                : tr("重置：删除该属性上全部关键帧（停用动画），"
                     "值恢复默认"));
        btn->setStyleSheet(tailBtnQss);
        return btn;
    };

    // ---- 共享行网格工具（v3.2 全行真对齐，2026-11-09 用户第二轮反馈
    //      "每行 key/⟲ 列对齐"）：同组所有行放【同一个 QGridLayout】——
    //      QGridLayout 列宽 = 该列全部单元格最大值，菱形/⟲ 组内天然对齐；
    //      旧版每行独立网格各行列宽独立计算，行名宽度不同（"位置" vs
    //     "滚转"）→ 值区起点漂移 → 行尾控件错位。列 0（行名）统一最小宽
    //      = 最宽行名"不透明度"（4 字），五组跨组也对齐。折叠仍按组：
    //      每组网格包一个 host 容器（QBoxLayout 对 hidden 控件不保留占
    //      位——组收起即收缩；QGridLayout 自身对隐藏 cell 保留占位，故
    //      不能全面板共用一个网格）--------------------------------------
    constexpr int kColVal = 1;     // 值框槽起始列（1..3）
    constexpr int kColDiamond = 4; // 菱形列（全行对齐）
    constexpr int kColReset = 5;   // ⟲ 列（全行右端）
    auto makeRowGrid = [&]() {
        auto* g = new QGridLayout();
        // 展开样式（2026-11-13 用户「再优化下展开样式」）：内容相对组标题
        // 左缩进 16px（对齐标题箭头后的文字——DaVinci Inspector 从属层次）
        // + 上下各 2px 呼吸（组间隔由组标题 margin-top 拉开，见
        // makeGroupHeader）；行距收紧 3px——行距 < 标题间距 < 组间距递进
        g->setContentsMargins(16, 2, 0, 2);
        g->setHorizontalSpacing(4);
        g->setVerticalSpacing(3);
        g->setColumnMinimumWidth(
            0, QFontMetrics(box->font())
                   .horizontalAdvance(QCoreApplication::translate(
                       "ClipSchema", "不透明度")) +
                   8);
        // 值区三列等 stretch（多余空间均分）；名/菱形/⟲ 列收缩随内容
        g->setColumnStretch(0, 0);
        g->setColumnStretch(1, 1);
        g->setColumnStretch(2, 1);
        g->setColumnStretch(3, 1);
        g->setColumnStretch(kColDiamond, 0);
        g->setColumnStretch(kColReset, 0);
        return g;
    };
    // 组收尾：网格装入 host → 主 VBox → 归组折叠表（每组单容器，
    // hide/show 即整组折叠——QWidget 包裹后 VBox 对 hidden 不保留占位）。
    // host 垂直 Maximum（2026-11-09 用户反馈"每分组等高疏密不均"根治）：
    // 面板被拉高时 QVBox 把多余高度均分给 Preferred 项——项多的组挤、
    // 项少的组被拉稀疏；Maximum 不参与拉伸（最高只到自身 sizeHint），
    // 各组保持紧凑自然高度，余量交给 lay 末尾弹簧吸收
    auto finishGroup = [&](int g, QGridLayout* grid) {
        auto* host = new QWidget(box);
        host->setLayout(grid);
        host->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        lay->addWidget(host);
        grp_rows_[g].push_back(host);
    };
    // 【P4-A 暴露面】空组隐藏：组内零暴露行 → 标题+容器都不占版面（标题
    // 钮仍创建——grp_btn_ 定长契约与折叠联动 connect 不感知可见性；
    // 隐藏钮永不 toggled，行为安全）
    auto hideGroupIfEmpty = [&](int g, int placed_rows) {
        if (placed_rows > 0) return;
        grp_btn_[g]->setVisible(false);
        for (auto* w : grp_rows_[g]) w->setVisible(false);
    };

    // ---- 每属性一行：[名] [数值...] [◆] [⟲]（v3 行布局定案：菱形移行
    //      尾 + 重置钮）。14 属性行同构共用 lambda；控件加入【调用方提供
    //     的组共享网格】（见下 makeRowGrid——同组同网格才跨行真对齐）。
    //      返回 bool = 行已落网格（P4-A 暴露面过滤：未暴露行只推 Row
    //      骨架即返 false，调用方行号不自增）----
    auto buildRow = [&](int i, QGridLayout* grid, int row) -> bool {
        const PropDef& def = kPropOrder[i];
        // 【P4-D】声明性数据行（显示名/值域/步长/decimals/后缀/分量名/
        // 默认值/resettable）经行下标直达 schema——行序对齐由
        // schemaKeysAligned static_assert 锁定
        const ms::FieldDef* fd = tweenFieldAt(i);
        Row r;
        r.prop       = def.prop;
        r.name       = fd ? QCoreApplication::translate("ClipSchema", fd->label)
                          : QString::fromUtf8(def.key);
        r.components = fd ? fd->components : 1;
        // 【2026-11-18 语义终定】位置行 UI 两分量（X/Y）——第三数据槽
        //（v2=叠放层级）独立「层级 Z」行写同槽 static_vals_[2]（applyTo
        // 路径不变）、深度 Z 静态框占位置行第三值位（pos_z_spin_，装配
        // 处特判）。打键 v2 改携当前基准层级——老工程 Z 通道求值不变、
        // 新键不打零。
        if (def.prop == TweenProp::Position) r.components = 2;

        // 【P4-A 暴露面过滤】schema 未暴露的属性行【不生成】（2026-11-17
        // 用户定案：不是隐藏而是不生成——Row 骨架仍入 rows_ 保持 13 行
        // 定长契约，控件全 nullptr；网格不落位）。既有读侧护栏覆盖：
        // refreshFrom 灌值/updateRowState 菱形三态均判空；applyTo 静态
        // 脏位驱动（退役行永不置位）；recordKeyframe 值框判空（P4-A 新增
        // ——公开 API，测试/快捷键可直达）。
        if (!row_exposed_[i]) {
            rows_.push_back(r);
            return false;
        }

        // 菱形打键钮（2026-11-09 用户定案改版：打键/删键切换——DaVinci
        // 语义；QIcon pixmap 三态——2026-11-09 用户三轮反馈根治"行尾两钮
        // 大小/基线不一"：字符方案两轮均因 Unicode 字形占位不可控失败
        //（QCheckBox 指示器大 → ⟲ 字形反超 ◆），终改 QPainter 同规格
        // pixmap 绘制，与 ⟲ 像素级等大）。三态语义：
        //   ○ = 未启用（无通道，灰） / ◇ = 已启用但当前帧无键（通道色）
        //   ◆ = 当前帧有关键帧（通道色实心——颜色与时间轴曲线同源）
        r.enable = new QToolButton(box);
        r.enable->setObjectName(
            QStringLiteral("tweenEnable%1").arg(def.obj_suffix));
        const QColor cc = tweenPropColor((int)def.prop);
        // 近白通道色（不透明度 #F2F2F2，调色板面向深色时间轴）在白底
        // 面板上不可辨——过亮时降亮（同曲线关键帧点"深描边"方法论）
        const QColor edge = cc.lightness() > 200 ? cc.darker(140) : cc;
        r.enable->setFixedSize(kTailBtnPx, kTailBtnPx);
        // 显式图标尺寸 = pixmap 视觉边长（不设则样式默认 PM_SmallIconSize
        // 16px 会把 12px pixmap 放大——模糊且视觉偏大）
        r.enable->setIconSize(QSize(kTailIconPx, kTailIconPx));
        r.enable->setProperty("edge", edge); // 通道色（setDiamondVisual 取色）
        r.enable->setStyleSheet(tailBtnQss); // hover 淡底（描色在 pixmap 内）
        r.enable->setToolTip(
            tr("%1 动画：点击 = 播放头处打键（◆ 时点击 = "
               "删除该处键；删光全部键即停用动画）。"
               "◆ = 当前帧有键；◇ = 已启用但当前帧无键；"
               "○ = 未启用（纯静态）")
                .arg(r.name));
        connect(r.enable, &QToolButton::clicked, this,
                [this, i] { onDiamondClicked(i); });
        setDiamondVisual(r.enable, 0); // 初始未启用（refreshFrom 再校准）

        // 名称标签（行首；属性行语义说明——3D 透视/裁切/角点各自附注）
        auto* name_lbl = new QLabel(r.name, box);
        if (def.prop == TweenProp::Position)
            name_lbl->setToolTip(
                tr("位置 = 画布坐标 X/Y + 深度 Z（第三值位，3D "
                   "空间真实位置）：深度走透视投影 s=F/(F−Z)，"
                   "正 = 朝观察者 Eye/舞台向上（画面放大）。三维打包一条属性："
                   "一个键携带 X/Y 全部分量（深度 Z 为静态属"
                   "性不参与关键帧；叠放层级在下方「层级 Z」行）"));
        if (def.prop == TweenProp::Rotation)
            name_lbl->setToolTip(
                tr("三轴旋转（一键携 X/Y/Z 三分量）：X = 俯仰绕"
                   "画布 X 轴（正 = 顶边后仰）、Y = 偏航绕画布 "
                   "Y 轴（正 = 右缘远离）、Z = 滚转绕画布 Z 轴"
                   "（2D 旋转，顺时针）——三轴旋转【应用完成后】"
                   "透视投影到 XY 平面（针孔压扁，近大远小），"
                   "绕锚点旋转。透视轴值域 ±89°"));
        if (def.prop >= TweenProp::CropT && def.prop <= TweenProp::CropR)
            name_lbl->setToolTip(
                tr("裁切：可见矩形各边向内收（画布 px，内容不"
                   "缩放不回写位置/尺寸读数）；四边各一条属性"
                   "单独打键。全部归 0 = 不裁切"));
        if (def.prop >= TweenProp::CornerLT &&
            def.prop <= TweenProp::CornerLB)
            name_lbl->setToolTip(
                tr("角点位移：相对未变形位置的末端修正偏移"
                   "（画布 px；LT 左上 / RT 右上 / RB 右下 / "
                   "LB 左下，每角单独打键）。旋转非 0 时位移"
                   "恒沿屏幕方向不随画面转；不影响位置/尺寸 "
                   "读数；全部归 0 = 无变形。支持关键帧动画"));

        if (def.prop == TweenProp::Opacity)
            name_lbl->setToolTip(
                tr("不透明度 [0,1]：与素材自身 alpha 相乘（1=不"
                   "透明，0=全透明），整条画面统一淡变；支持关"
                   "键帧动画"));
        if (def.prop == TweenProp::AudioGain)
            name_lbl->setToolTip(
                tr("音频增益 [0,2]（线性倍率，2026-11-10 追加）："
                   "1.0=不衰减、0=静音、2.0=+6dB（上限防增益曲"
                   "线异常把混音推入持续削波）。有 AudioGain 通"
                   "道时整条接管静态 audio_gain；消费端 = 混音"
                   "引擎 resolveGain，支持关键帧做音量自动化"));
        if (def.prop == TweenProp::RgbGain)
            name_lbl->setToolTip(
                tr("RGB 增益 [0,2]（三通道线性倍率，2026-11-20 追加）："
                   "采样后逐通道相乘（1=不变，0=该通道压黑，>1 "
                   "提升亮度/调色）。三通道打包一条属性：一个键携带 R/G/B "
                   "全部分量。偏色校正/色彩氛围动画用，支持关键帧"));
        if (def.prop == TweenProp::Gamma)
            name_lbl->setToolTip(
                tr("Gamma 伽马校正 [0.1,4]（2026-11-20 追加）："
                   "渲染端 pow(颜色, 1/γ)——γ>1 提亮中间调、"
                   "<1 压暗；1.0 = 不校正（零开销）。支持关键帧"
                   "做曝光/氛围渐变"));

        // 值框（三维行三框 / 二维行双框 / 标量行单框）；编辑 → 打包双模直写
        // （2026-11-12 用户定案：面板数值控件统一 DragValue——拖拽/滚轮步进、
        // 双击精确输入，交互与节点面板一致）
        for (int c = 0; c < r.components; ++c) {
            FloatDragValueWidget* sp = new FloatDragValueWidget(box);
            sp->setObjectName(QStringLiteral("tween%1%2")
                                  .arg(def.obj_suffix,
                                       fd ? fd->comp_names[c] : ""));
            sp->setRange(fd ? fd->vmin : 0.0, fd ? fd->vmax : 0.0);
            // 旋转行透视轴分量（X 俯仰/Y 偏航）独立值域 ±89°——90° 起画面
            // 翻面透视退化（Z 滚转保持行值域 ±3600°）
            if (def.prop == TweenProp::Rotation && c < 2)
                sp->setRange(-89.0, 89.0);
            sp->setDecimals(fd ? fd->decimals : 2);
            sp->setSingleStep(fd ? fd->step : 1.0);
            const char* unit_str = fd ? fd->unit : "";
            if (unit_str && unit_str[0])
                sp->setSuffix(QString::fromUtf8(unit_str));
            sp->setToolTip(
                tr("播放头处 %1 的%2%3（通道 = 插值结果；未启用"
                   " = 静态基准）。通道启用时修改任一分量即在播"
                   "放头位置打包写关键帧")
                    .arg(r.name,
                         r.components >= 2
                             ? tr("分量 ")
                             : QStringLiteral(""),
                         r.components >= 2
                             ? QString::fromUtf8(fd ? fd->comp_names[c]
                                                    : "")
                             : QString()));
            connect(sp, &FloatDragValueWidget::valueChanged, this,
                    [this, i, c](double v) { onValueEdited(i, c, v); });
            r.value[c] = sp;
        }

        // ⟲ 重置钮（尺寸行 nullptr——无默认值语义）
        if (!fd || fd->resettable) {
            r.reset = makeResetButton(def);
            connect(r.reset, &QToolButton::clicked, this,
                    [this, i] { onResetClicked(i); });
        }

        // 控件入组共享网格（v3.2，2026-11-09 用户第二轮反馈"每行 key/⟲
        // 列对齐"）：列 0=行名 / 列 1..3=值框槽（等宽）/ 列 4=菱形 / 列
        // 5=⟲。旧版每行独立 QGridLayout——各行列宽独立计算，行名宽度不
        // 同（"位置" vs "滚转"）→ 值区起点漂移 → 菱形/⟲ 行间错位；同组
        // 同网格后 QGridLayout 列宽 = 该列全部单元格最大值，天然对齐。
        // 标量行值框跨三列（值区全宽）；尺寸/角点双值行占列 1-2（第 3
        // 列空置——与位置行 X/Y/Z 列边界对齐）。
        grid->addWidget(name_lbl, row, 0);
        if (r.components == 1) {
            grid->addWidget(r.value[0], row, kColVal, 1, 3); // 标量行跨三列
        } else {
            grid->addWidget(r.value[0], row, kColVal + 0);
            grid->addWidget(r.value[1], row, kColVal + 1);
            if (r.value[2]) grid->addWidget(r.value[2], row, kColVal + 2);
        }
        // 行尾两钮显式 AlignVCenter（2026-11-09 第三轮反馈"不在同一行"：
        // 18x18 定高钮与 ~22px 值框同行，Qt 布局对不能拉伸的控件默认
        // 对齐依样式而异——显式垂直居中保证菱形/⟲ 与值框同中轴）
        grid->addWidget(r.enable, row, kColDiamond, Qt::AlignVCenter);
        if (r.reset) grid->addWidget(r.reset, row, kColReset, Qt::AlignVCenter);

        rows_.push_back(r);
        return true;
    };
    // ---- 五组装配（v4 五分组，DaVinci Inspector 同构；行连续切分）：
    //      组 0「变换」锚点+位置+尺寸（默认展开）/ 组 1「姿态」行 2 / 
    //      组 2「裁切」行 3..6 / 组 3「形变」行 7..10 / 组 4「合成」行 11
    //      ——除组 0 外默认收（零值噪音收进折叠，与 v2 渐进折叠同精神）
    grp_btn_[0] = makeGroupHeader(
        "tweenGroupXform", tr("变换（锚点 / 位置 / 层级 / 尺寸）"),
        tr("旋转缩放锚点 + 位置（X/Y/深度 Z，3D 空间真实位置）+ 叠放层级与"
           "尺寸（画布 px）：改值即编辑画面；配合菱形开关关键帧动画"),
        true);
    lay->addWidget(grp_btn_[0]);
    {
        auto* g0 = makeRowGrid();

        // 【P4-A 暴露面】组 0 是否有暴露行（位置/尺寸）——锚点属变换域，
        // 随组 0 同进退（Audio 片段 g0_has=false：锚点也不生成）。
        // 【2026-11-18 语义终定】深度 Z / 遮挡次序同属变换域静态行，
        // 一并纳入组 0 门控——组 0 零暴露行时（Audio）整组不生成，
        // 维持「Audio 动画页仅音频增益」定案（透视/遮挡对纯音频无意义）。
        const bool g0_has = row_exposed_[0] || row_exposed_[1];
        int gr = 0; // 运行网格行号（未暴露行不落位——行号收紧无空洞）

        if (g0_has) {
            // ---- 锚点行（组 0 首行——2026-11-09 用户指令"锚点选项移到位置
            //      之上"：锚点是旋转/缩放的枢轴，语义先于位置/尺寸）。静态属
            //      性非 tween：列 4 空（无菱形），⟲ 在列 5 与属性行右端对齐
            auto* lbl = new QLabel(tr("锚点"), box);
            lbl->setToolTip(
                tr("旋转与缩放的枢轴：中心或四个角点。改变锚点不"
                   "补偿位置——旋转/缩放非恒等时画面会绕新枢轴"
                   "重排（再调位置分量对位即可）"));
            anchor_combo_ = new QComboBox(box);
            anchor_combo_->setObjectName(QStringLiteral("tweenAnchor"));
            anchor_combo_->addItem(tr("中心")); // 0 = 默认（历史行为）
            anchor_combo_->addItem(tr("左上")); // 1
            anchor_combo_->addItem(tr("右上")); // 2
            anchor_combo_->addItem(tr("右下")); // 3
            anchor_combo_->addItem(tr("左下")); // 4
            // 预填在 connect 之前（不触发写回）
            anchor_val_ = std::clamp(clip.transform.anchor, 0, 4);
            anchor_combo_->setCurrentIndex(anchor_val_);
            connect(anchor_combo_,
                    QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this](int idx) {
                        anchor_val_ = idx;
                        static_dirty_ |= kSAnchor;
                        if (applier_) applier_();
                    });
            // ⟲ 重置（恢复中心；静态直写——锚点非 tween 属性）。
            // 18x18 占位 + 同规格 pixmap 图标（与属性行两钮统一——同轮反馈）
            auto* rst = new QToolButton(box);
            rst->setObjectName(QStringLiteral("tweenResetAnchor"));
            rst->setFixedSize(kTailBtnPx, kTailBtnPx);
            rst->setIconSize(QSize(kTailIconPx, kTailIconPx)); // 防 16px 默认放大
            rst->setIcon(resetGlyphIcon(rst->devicePixelRatioF()));
            rst->setToolTip(tr("恢复默认锚点（中心）——静态属性"
                               "无动画通道，直接写回静态基准"));
            rst->setStyleSheet(tailBtnQss);
            connect(rst, &QToolButton::clicked, this, [this] {
                anchor_val_ = 0;
                {
                    const QSignalBlocker b(anchor_combo_);
                    anchor_combo_->setCurrentIndex(0);
                }
                static_dirty_ |= kSAnchor;
                if (applier_) applier_();
            });
            g0->addWidget(lbl, gr, 0);
            g0->addWidget(anchor_combo_, gr, kColVal, 1, 3); // combo 跨值区三列
            g0->addWidget(rst, gr, kColReset, Qt::AlignVCenter);
            ++gr;
        } // if (g0_has)

        // ---- 位置行（【2026-11-20 2D 化】位置 = 两分量 X/Y——第三值位
        //      "深度 Z" 框退役不再生成（pos_z_spin_ 恒空，更新/写回路径
        //      均判空安全）。数据字段 transform.pos_z 保留：旧工程加载时
        //      统一归零（ProjectSnapshotBuilder / EditorRuntime），UI 无
        //      任何 Z 编辑/显示入口 --------------------------------------
        const int pos_row = gr;
        if (buildRow(0, g0, gr)) {
            ++gr;
        }

        // ---- 层级 Z 行（【2026-11-18 语义终定】纯遮挡/叠放键，非空间
        //      坐标——3D 视口与渲染端一律不再把它当高度轴使用）：叠放
        //      层级 transform.z（整数无单位），遮挡次序=图层次序时的排
        //      序键。写同槽 static_vals_[2]（sBit(0,2)）——applyTo →
        //      transform.z 路径不变。静态属性：列 4 空无菱形，⟲ 列 5 与
        //      属性行右端对齐（同锚点行规格）。【P4-A 门控】随组 0 同进
        //      退（g0_has=false 不生成）--------------------------------------
        if (g0_has) {
            auto* llbl = new QLabel(tr("层级 Z"), box);
            llbl->setToolTip(
                tr("叠放层级（整数，无单位——不是空间坐标）：多"
                   "片段重叠时的绘制先后——小值先绘（底层）、"
                   "大值后绘（顶层）；遮挡次序=图层次序时的排"
                   "序键。素材在三维中的 z 位置在位置行「深度"
                   " Z」，两者互不相干"));
            z_layer_spin_ = new IntDragValueWidget(box);
            z_layer_spin_->setObjectName(QStringLiteral("tweenLayerZ"));
            z_layer_spin_->setRange(-1000, 1000);
            // 预填在 connect 之前（不触发写回）
            z_layer_spin_->setValue(std::clamp(clip.transform.z, -1000, 1000));
            connect(z_layer_spin_, &IntDragValueWidget::valueChanged, this,
                    [this](int v) {
                        static_vals_[2] = v; // 槽 2 = 位置行 v2 槽（层级）
                        static_dirty_ |= sBit(0, 2);
                        if (applier_) applier_(); // applyTo → transform.z
                    });
            // ⟲ 重置（恢复 0；静态直写同锚点行纪律）
            auto* lrst = new QToolButton(box);
            lrst->setObjectName(QStringLiteral("tweenResetLayerZ"));
            lrst->setFixedSize(kTailBtnPx, kTailBtnPx);
            lrst->setIconSize(QSize(kTailIconPx, kTailIconPx));
            lrst->setIcon(resetGlyphIcon(lrst->devicePixelRatioF()));
            lrst->setToolTip(tr("恢复默认层级（0）——静态属性"
                                "无动画通道，直接写回静态基准"));
            lrst->setStyleSheet(tailBtnQss);
            connect(lrst, &QToolButton::clicked, this, [this] {
                {
                    const QSignalBlocker b(z_layer_spin_);
                    z_layer_spin_->setValue(0);
                }
                static_vals_[2] = 0.0;
                static_dirty_ |= sBit(0, 2);
                if (applier_) applier_();
            });
            g0->addWidget(llbl, gr, 0);
            g0->addWidget(z_layer_spin_, gr, kColVal, 1, 3); // 标量跨值区三列
            g0->addWidget(lrst, gr, kColReset, Qt::AlignVCenter);
            ++gr;

            // ---- 遮挡次序行（阶段③ per-cue stacking；WO7 对标——
            //      By Layer=层级 Z（历史）/ By Z=几何深度 pos_z 远先绘
            //      近后绘）。静态属性非 tween（列 4 空无菱形；无 ⟲——
            //      模式枚举无"默认值重置"语义，历史默认即图层级）。
            auto* slbl = new QLabel(tr("遮挡次序"), box);
            slbl->setToolTip(
                tr("本片段参与渲染排序的键（WATCHOUT Stacking "
                   "语义）：图层次序 = 按上方「层级 Z」升序（历"
                   "史行为）；深度次序 = 按位置行「深度 Z」升序"
                   "（远的先画、近的遮挡远的——透视场景物理遮挡）"));
            stacking_combo_ = new QComboBox(box);
            stacking_combo_->setObjectName(QStringLiteral("tweenStacking"));
            stacking_combo_->addItem(tr("图层次序")); // 0=默认
            stacking_combo_->addItem(tr("深度次序")); // 1
            stacking_val_ = clip.stacking == kStackingPositionZ ? 1 : 0;
            stacking_combo_->setCurrentIndex(stacking_val_);
            connect(stacking_combo_,
                    QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this](int idx) {
                        stacking_val_ = idx;
                        static_dirty_ |= kSStacking;
                        if (applier_) applier_();
                    });
            g0->addWidget(slbl, gr, 0);
            g0->addWidget(stacking_combo_, gr, kColVal, 1, 3);
            ++gr;
        } // if (g0_has)

        if (buildRow(1, g0, gr)) ++gr; // 尺寸
        finishGroup(0, g0);
        hideGroupIfEmpty(0, gr);
    }

    // ---- 组 1「姿态」：旋转一行三维打包（2026-11-20 修订：三轴旋转
    //      应用完成后透视投影到 XY 平面——近大远小）------------------------
    grp_btn_[1] = makeGroupHeader(
        "tweenGroupPose", tr("姿态（绕 X / Y / Z 轴旋转）"),
        tr("三轴旋转一条通道：X = 俯仰、Y = 偏航、Z = 滚转（2D 旋转）——应用"
           "完成后透视投影到 XY 平面（针孔压扁，近大远小）。点击标题展开"),
        false);
    lay->addWidget(grp_btn_[1]);
    {
        auto* g1 = makeRowGrid();
        int gr = 0;
        if (buildRow(2, g1, gr)) ++gr;
        finishGroup(1, g1);
        hideGroupIfEmpty(1, gr);
    }

    // ---- 组 2「裁切」：上下左右（v3 数据链：可见矩形收缩 + 帧采样窗）------
    grp_btn_[2] = makeGroupHeader(
        "tweenGroupCrop", tr("裁切（上下左右向内收）"),
        tr("裁切 = 可见矩形各边向内收（画布 px）：内容不缩放，裁掉部分从"
           "画面移除；不影响位置/尺寸读数。点击标题展开"),
        false);
    lay->addWidget(grp_btn_[2]);
    {
        auto* g2 = makeRowGrid();
        int gr = 0;
        for (int i = 3; i < 7; ++i) if (buildRow(i, g2, gr)) ++gr;
        finishGroup(2, g2);
        hideGroupIfEmpty(2, gr);
    }

    // ---- 组 3「形变」：角点 4 行（每角一条二维属性，每角单独打键）----------
    //      行 7..10 分量直映 Transform.corner[0..7]（CornerIdx
    //      [LT,RT,RB,LB] × (dx,dy) 同序）；Transform.corner 已在求值/
    //      渲染全链路，通道求值结果直接驱动既有 corner 管线
    grp_btn_[3] = makeGroupHeader(
        "tweenGroupCorner", tr("形变（角点 corner pin 偏移）"),
        tr("四角位移变形（corner pin 末端修正，画布 px，每角单独打键）——点击展开"),
        false);
    lay->addWidget(grp_btn_[3]);
    {
        auto* g3 = makeRowGrid();
        int gr = 0;
        for (int i = 7; i < 11; ++i) if (buildRow(i, g3, gr)) ++gr;
        finishGroup(3, g3);
        hideGroupIfEmpty(3, gr);
    }

    // ---- 组 4「合成」：不透明度 + 音频增益（行 12）+ 颜色合成（行 13/14，
    //      2026-11-20 追加 RGB 增益/Gamma——schema/写回/读回同轮就绪，
    //      组装配漏 buildRow 导致用户"合成组看不到颜色设置"，补上）----
    grp_btn_[4] = makeGroupHeader(
        "tweenGroupComp", tr("合成（不透明度 / 颜色 / 音频增益）"),
        tr("不透明度 [0,1]（与素材自身 alpha 相乘）；RGB 增益 [0,2]（R/G/B "
           "线性增益，1.0=不变）；Gamma [0.1,4]（1.0=不校正，pow(c,1/γ)）；"
           "音频增益 [0,2]（线性，1.0=不衰减，0=静音）——点击标题展开"),
        false);
    lay->addWidget(grp_btn_[4]);
    {
        auto* g4 = makeRowGrid();
        int gr = 0;
        if (buildRow(11, g4, gr)) ++gr;
        if (buildRow(12, g4, gr)) ++gr;
        if (buildRow(13, g4, gr)) ++gr; // RGB 增益（三维 R/G/B 打包打键）
        if (buildRow(14, g4, gr)) ++gr; // Gamma 伽马校正（标量打键）
        finishGroup(4, g4);
        hideGroupIfEmpty(4, gr);
    }

    // 初始折叠态：组 0 展开（构造时 checked=true）、组 1..4 收起——直接
    // 调可见性（不触发 toggled：setChecked 不改状态）
    for (int g = 1; g < 5; ++g)
        for (auto* w : grp_rows_[g]) w->setVisible(false);
    for (int g = 1; g < 5; ++g) grp_btn_[g]->setArrowType(Qt::RightArrow);

    // 组可见性联动（header 点击 → 组内行显隐 + 箭头翻转）。必须在五组
    // 装配【之后】连接——早于创建时 grp_btn_ 为空，connect 静默失效
    //（2026-11-09 探针教训：Qt connect 对空 sender 只警告不报错）
    for (int g = 0; g < 5; ++g) {
        connect(grp_btn_[g], &QToolButton::toggled, this,
                [this, g](bool on) {
                    for (auto* w : grp_rows_[g]) w->setVisible(on);
                    grp_btn_[g]->setArrowType(on ? Qt::DownArrow
                                                 : Qt::RightArrow);
                });
    }

    root->addWidget(box);

    // 面板余量兜底：容器分配高度大于内容 sizeHint 时，多余空间全部沉到
    // QGroupBox 底部（组框内空白），五组保持紧凑自然高度不再均分拉稀
    //（与 finishGroup 的 host Maximum 配套——2026-11-09 用户反馈根治）
    lay->addStretch(1);

    // 预填显示（构造时无法拉播放头——适配器随后 setPlayheadFn + refreshFrom）
    refreshFrom(clip);
}

// ---------------------------------------------------------------------------
// 行访问器（越界防御返回 nullptr——测试下标错误快速暴露而非崩溃）
// ---------------------------------------------------------------------------
int TweenSection::propCount() { return 15; } // v4 五分组 13 属性 + 2026-11-20 颜色合成追加 2 行（rgb_gain/gamma；Z 行/俯仰/偏航退役合并）

FloatDragValueWidget* TweenSection::valueSpin(int i, int component) const {
    if (i < 0 || i >= (int)rows_.size()) return nullptr;
    if (component < 0 || component >= rows_[i].components) return nullptr;
    return rows_[i].value[component];
}
QToolButton* TweenSection::enableWidget(int i) const {
    return (i >= 0 && i < (int)rows_.size()) ? rows_[i].enable : nullptr;
}

// ---------------------------------------------------------------------------
// 播放头 → 片段局部时间（钳制 [0, duration_us]；无钩子 = 0 = 片段起点）
// ---------------------------------------------------------------------------
std::int64_t TweenSection::localPlayheadUs() const {
    if (!playhead_fn_) return 0;
    const std::int64_t local = playhead_fn_() - clip_start_us_;
    return std::clamp(local, std::int64_t{0},
                      std::max(clip_dur_us_, std::int64_t{0}));
}

// ---------------------------------------------------------------------------
// 通道定位（channels_ 线性查找；通道数 ≤14，开销可忽略）
// ---------------------------------------------------------------------------
TweenChannel* TweenSection::findChannel(TweenProp prop) {
    for (auto& ch : channels_)
        if (ch.prop == prop) return &ch;
    return nullptr;
}
const TweenChannel* TweenSection::findChannel(TweenProp prop) const {
    for (const auto& ch : channels_)
        if (ch.prop == prop) return &ch;
    return nullptr;
}

// ---------------------------------------------------------------------------
// "播放头处"关键帧定位：精确命中优先，最近 ±20ms 容差（帧量化编辑容差）
// ---------------------------------------------------------------------------
int TweenSection::keyIndexAtPlayhead(const TweenChannel& ch) const {
    const std::int64_t t = localPlayheadUs();
    int best = -1;
    std::int64_t best_dist = kKeyHitToleranceUs + 1;
    for (int i = 0; i < (int)ch.keys.size(); ++i) {
        const std::int64_t d = std::int64_t(std::abs(ch.keys[i].t_us - t));
        if (d < best_dist) { best_dist = d; best = i; }
    }
    return best; // -1 = 容差内无关键帧
}

// ---------------------------------------------------------------------------
// 打点（auto-key 落点，打包语义）：该行值框当前显示值 → 播放头处关键帧
//（存在则更新 / 不存在则插入，通道不存在则建通道单键起步）。打包属性
// 一个键携带全部分量（位置行 v0/v1/v2 = X/Y/Z 三值框；二维行 v0/v1；标
// 量 v0）——⚠️ 聚合构造必须五字段；新键插值恒 Linear（v2 定案：行尾插
// 值下拉已删）。值框 valueChanged 的 auto-key 即本函数转发；菱形建通道
// 后首次打点同源。
// ---------------------------------------------------------------------------
void TweenSection::recordKeyframe(int i) {
    if (i < 0 || i >= (int)rows_.size()) return;
    Row& r = rows_[i];
    if (!r.value[0]) return; // P4-A：未暴露行无控件（schema 外属性不参与
                             // 动画编辑；公开 API 直达时静默忽略）
    const std::int64_t t = localPlayheadUs();
    // 行显示值即数据层值（绝对 px 直通——显示/数据一个口径）
    const double v0 = r.value[0]->value();
    const double v1 = r.components >= 2 ? r.value[1]->value() : 0.0;
    // v2 携带源（2026-11-18 Z 语义归位）：位置行 UI 减为 X/Y 后第三分量
    // 无行内值框——新键携带当前基准层级（z_shown_，refreshFrom 持续校准）
    // 而非打零：老工程 Z 通道求值不变，动画中打键不突变
    double v2 = 0.0;
    if (r.components >= 3 && r.value[2]) v2 = r.value[2]->value();
    else if (r.prop == TweenProp::Position) v2 = z_shown_;

    TweenChannel* ch = findChannel(r.prop);
    if (!ch) {
        // 建通道（单关键帧起步；插值恒 Linear；五字段聚合——interp 勿
        // 隐转塞进 v2）
        TweenChannel c;
        c.prop = r.prop;
        c.keys.push_back(Keyframe{t, v0, v1, v2, TweenInterp::Linear});
        channels_.push_back(std::move(c));
        updateRowState(i);
        if (applier_) applier_();
        return;
    }

    // 已有通道：命中既有关键帧则更新（全分量同写），否则按 t_us 升序插入
    const int hit = keyIndexAtPlayhead(*ch);
    if (hit >= 0) {
        ch->keys[hit].v0    = v0;
        ch->keys[hit].v1    = v1;
        ch->keys[hit].v2    = v2;
        ch->keys[hit].t_us  = t; // 容差内吸附到播放头
        ch->keys[hit].interp = TweenInterp::Linear;
    } else {
        const Keyframe k{t, v0, v1, v2, TweenInterp::Linear};
        auto& keys = ch->keys;
        keys.insert(
            std::lower_bound(keys.begin(), keys.end(), k.t_us,
                             [](const Keyframe& a, std::int64_t tt) {
                                 return a.t_us < tt;
                             }),
            k);
    }
    updateRowState(i);
    if (applier_) applier_();
}

// ---------------------------------------------------------------------------
// 菱形点击（2026-11-09 用户定案改版：打键/删键切换——DaVinci 语义）：
// 无通道 → 建通道（播放头处单关键帧，值 = 值框当前显示值——打包全分量
// 同取）；有通道 → 播放头处【打键/删键】切换——命中(±20ms)删该处键
//（删空连通道删，R8 非空纪律），未命中按当前值打键。旧语义"已启用点击
// = 整通道移除"退役——用户多点一下即丢全部键（2026-11-09 实测踩坑：
// "打了多个 key 帧之后只显示一个点"根因之一）。显示态由 updateRowState
// 统一落位（QSignalBlocker 防 toggled 链再入）。
// ---------------------------------------------------------------------------
void TweenSection::onDiamondClicked(int i) {
    if (i < 0 || i >= (int)rows_.size()) return;
    Row& r = rows_[i];
    TweenChannel* ch = findChannel(r.prop);

    if (!ch) {
        // 建通道（单关键帧起步；插值恒 Linear；五字段聚合——interp 勿
        // 隐转塞进 v2）
        TweenChannel c;
        c.prop = r.prop;
        c.keys.push_back(Keyframe{
            localPlayheadUs(), r.value[0]->value(),
            r.components >= 2 ? r.value[1]->value() : 0.0,
            // 位置行 v2 携带基准层级（UI 无第三框——防老工程 Z 通道打零）
            r.components >= 3 ? r.value[2]->value()
                              : (r.prop == TweenProp::Position ? z_shown_
                                                               : 0.0),
            TweenInterp::Linear});
        channels_.push_back(std::move(c));
        updateRowState(i);
        if (applier_) applier_();
        return;
    }

    // 有通道：播放头处打键/删键切换
    const int hit = keyIndexAtPlayhead(*ch);
    if (hit >= 0) {
        ch->keys.erase(ch->keys.begin() + hit);
        if (ch->keys.empty()) // 删空 → 通道连删（键带消失 = 通道生命终点）
            channels_.erase(std::remove_if(channels_.begin(), channels_.end(),
                                           [p = r.prop](const TweenChannel& c) {
                                               return c.prop == p;
                                           }),
                            channels_.end());
    } else {
        const std::int64_t t = localPlayheadUs();
        const Keyframe k{t, r.value[0]->value(),
                         r.components >= 2 ? r.value[1]->value() : 0.0,
                         // 同建通道：位置 v2 携带基准层级
                         r.components >= 3 ? r.value[2]->value()
                                           : (r.prop == TweenProp::Position
                                                  ? z_shown_
                                                  : 0.0),
                         TweenInterp::Linear};
        auto& keys = ch->keys;
        keys.insert(
            std::lower_bound(keys.begin(), keys.end(), k.t_us,
                             [](const Keyframe& a, std::int64_t tt) {
                                 return a.t_us < tt;
                             }),
            k);
    }
    updateRowState(i);
    if (applier_) applier_();
}

// ---------------------------------------------------------------------------
// 值框编辑（双模直写）：通道存在 → 打包 auto-key（整条属性播放头处写/
// 改键，全分量同落——与时间轴拖键/画布拖拽同语义）；无通道 → 静态基准
// 分量编辑（static_vals_ + 脏位，applyTo 落 transform）。
// ---------------------------------------------------------------------------
void TweenSection::onValueEdited(int i, int comp, double v) {
    if (i < 0 || i >= (int)rows_.size()) return;
    if (comp < 0 || comp >= rows_[i].components) return;
    Row& r = rows_[i];
    // 静态暂存存【行显示值】= 画布绝对 px（显示/数据一个口径；与
    // refreshFrom 的显示值口径一致，外部改动不被陈旧暂存回退）
    static_vals_[kRowSlot[i] + comp] = v;
    if (findChannel(r.prop)) {
        recordKeyframe(i); // 打包 auto-key：命中更新/插入（内含 applier）
        return;
    }
    static_dirty_ |= sBit(i, comp);
    if (applier_) applier_(); // 静态写回（applyTo 覆盖 transform 对应分量）
}

// ---------------------------------------------------------------------------
// ⟲ 重置钮（2026-11-10 用户定案新语义）：删除该属性上【全部】key 帧
//（通道连删——与行尾菱形删空连通道删同一通道生命终点纪律）+ 值恢复默认
// 静态直写。旧语义"通道存在 = 播放头处打包写默认值键"退役——默认值键
// 插进动画中间会把曲线拉直成"跳到默认再回来"的假动画，且用户体感
// reset 应该是"清掉这个属性的动画"。
// 尺寸行特判（2026-11-09 补设 ⟲）：恢复目标 = 素材原始尺寸——默认值取
// 适配器注入的素材宽高 hint（schema 默认 0 无意义）；hint 缺失
//（Live/未绑定素材）无默认可恢复，点击不动作。
// ---------------------------------------------------------------------------
void TweenSection::onResetClicked(int i) {
    if (i < 0 || i >= (int)rows_.size()) return;
    Row& r = rows_[i];
    const bool is_size = (r.prop == TweenProp::Size);
    if (is_size && size_hint_w_ <= 0.0 && size_hint_h_ <= 0.0) return;
    // 分量默认值：尺寸行取注入 hint（W/H），其余行走 schema 行
    // default_value（P4-D：默认值唯一事实在 ms::kTweenSchema）
    const ms::FieldDef* fd = tweenFieldAt(i);
    auto defVal = [&](int c) {
        return is_size ? (c == 0 ? size_hint_w_ : size_hint_h_)
                       : (fd ? fd->default_value[c] : 0.0);
    };
    for (int c = 0; c < r.components; ++c) {
        if (r.value[c]) {
            const QSignalBlocker b(r.value[c]);
            r.value[c]->setValue(defVal(c));
        }
    }
    // 新语义：有通道 → 整条通道连删（= 删该属性全部 key 帧），动画停用
    channels_.erase(std::remove_if(channels_.begin(), channels_.end(),
                                   [p = r.prop](const TweenChannel& c) {
                                       return c.prop == p;
                                   }),
                    channels_.end());
    // 静态基准恢复默认（删通道后该属性回归静态——统一走静态直写）
    for (int c = 0; c < r.components; ++c) {
        static_vals_[kRowSlot[i] + c] = defVal(c);
        static_dirty_ |= sBit(i, c);
    }
    updateRowState(i); // 菱形回 ○（通道已删）
    if (applier_) applier_();
}

// ---------------------------------------------------------------------------
// 行内状态刷新：菱形三态——增删关键帧后调用。菱形态：无通道=○（灰）；
// 通道命中播放头=◆；未命中=◇。（v2 定案：计数标签/插值下拉已删——行内
// 状态仅剩菱形三态。字符钮无状态信号，无需 QSignalBlocker。）
// ---------------------------------------------------------------------------
void TweenSection::updateRowState(int i) {
    Row& r = rows_[i];
    const TweenChannel* ch = findChannel(r.prop);
    const bool has = ch && !ch->keys.empty();
    const int hit = has ? keyIndexAtPlayhead(*ch) : -1;
    if (r.enable)
        setDiamondVisual(r.enable, !has ? 0 : (hit >= 0 ? 2 : 1));
}

// ---------------------------------------------------------------------------
// 控件态 → clip（tweens 无条件覆盖 + transform 静态脏字段合并）
// ---------------------------------------------------------------------------
void TweenSection::applyTo(Clip& c) const {
    c.tweens = channels_; // tween 域唯一写者：无条件覆盖
    // 静态基准脏字段（槽位 = kRowSlot[行]+分量 → transform 字段；仅覆盖
    // 用户动过的）。全部值框显示值 = 画布绝对 px，直通写回
    if (static_dirty_ & sBit(0, 0)) c.transform.x = static_vals_[0];
    if (static_dirty_ & sBit(0, 1)) c.transform.y = static_vals_[1];
    if (static_dirty_ & sBit(0, 2))
        c.transform.z = std::lround(static_vals_[2]); // 层级取整
    if (static_dirty_ & sBit(1, 0)) c.transform.w = static_vals_[3];
    if (static_dirty_ & sBit(1, 1)) c.transform.h = static_vals_[4];
    // 旋转行（v4 三维打包）：X 俯仰 / Y 偏航 / Z 滚转
    if (static_dirty_ & sBit(2, 0)) c.transform.rot_x = static_vals_[5];
    if (static_dirty_ & sBit(2, 1)) c.transform.rot_y = static_vals_[6];
    if (static_dirty_ & sBit(2, 2)) c.transform.rotation = static_vals_[7];
    if (static_dirty_ & sBit(3, 0)) c.transform.crop[kCropT] = static_vals_[8];
    if (static_dirty_ & sBit(4, 0)) c.transform.crop[kCropB] = static_vals_[9];
    if (static_dirty_ & sBit(5, 0)) c.transform.crop[kCropL] = static_vals_[10];
    if (static_dirty_ & sBit(6, 0)) c.transform.crop[kCropR] = static_vals_[11];
    for (int k = 0; k < 4; ++k) { // 角点行 7..10 → corner[0..7]
        if (static_dirty_ & sBit(7 + k, 0))
            c.transform.corner[k * 2] = static_vals_[12 + k * 2];
        if (static_dirty_ & sBit(7 + k, 1))
            c.transform.corner[k * 2 + 1] = static_vals_[12 + k * 2 + 1];
    }
    if (static_dirty_ & sBit(11, 0)) c.transform.opacity = static_vals_[20];
    // 音频增益静态基准（行 12，2026-11-10）：直写 clip.audio_gain（非
    // transform 域属性；有 AudioGain 通道时本值被通道整条接管）
    if (static_dirty_ & sBit(12, 0)) c.audio_gain = static_vals_[21];
    // 颜色合成静态基准（行 13/14，2026-11-20）：直写 transform 颜色域
    // 字段（有 RgbGain/Gamma 通道时被通道整条接管——resolveTransform
    // 同一 switch 语义）
    if (static_dirty_ & sBit(13, 0)) c.transform.rgb_gain[0] = static_vals_[22];
    if (static_dirty_ & sBit(13, 1)) c.transform.rgb_gain[1] = static_vals_[23];
    if (static_dirty_ & sBit(13, 2)) c.transform.rgb_gain[2] = static_vals_[24];
    if (static_dirty_ & sBit(14, 0)) c.transform.gamma = static_vals_[25];
    // 锚点（追加区：静态属性直写 transform）
    if (static_dirty_ & kSAnchor) c.transform.anchor = anchor_val_;
    // 深度 Z（2026-11-17 阶段③追加区：静态属性直写 transform.pos_z——
    // 透视投影几何深度，非叠放层级）
    if (static_dirty_ & kSPosZ) c.transform.pos_z = pos_z_val_;
    // 遮挡次序（2026-11-17 阶段③追加区：直写 clip.stacking——渲染排序
    // 键模式，TimelineEngine 消费；非 transform 域属性）
    if (static_dirty_ & kSStacking)
        c.stacking = stacking_val_ == 1 ? kStackingPositionZ
                                        : kStackingRenderOrder;
}

// ---------------------------------------------------------------------------
// 【双向同步·模型→面板向】片段当前值/播放头位置 → 控件显示
// ---------------------------------------------------------------------------
void TweenSection::refreshFrom(const Clip& c) {
    clip_start_us_ = c.start_us;
    clip_dur_us_   = c.duration_us;

    // 播放头处解算值（无播放头钩子 → 片段起点兜底）：
    // 通道启用的属性显示 TweenEngine 插值结果，未启用属性即静态值——
    // 一个 resolveTransform 同时覆盖两种来源
    const std::int64_t playhead_us =
        playhead_fn_ ? playhead_fn_() : c.start_us;
    const Transform shown = TweenEngine::resolveTransform(c, playhead_us);
    // 音频增益独立解算（行 12；Transform 无增益域——resolveGain 同源同
    // 语义：通道接管/静态基准两来源一个出口）
    const double gain_shown = TweenEngine::resolveGain(c, playhead_us);

    // 通道内存态与工作副本对齐（外部写回/场景重载后以工作副本为准）
    channels_ = c.tweens;

    for (int i = 0; i < (int)rows_.size(); ++i) {
        Row& r = rows_[i];
        const TweenChannel* ch = findChannel(r.prop);
        const bool has = ch && !ch->keys.empty();

        // 数值框：播放头处当前分量值（焦点保护：用户正在输入不灌）。
        // 全部行显示值 = 数据层值直通（绝对 px——显示/编辑一个口径）。
        // 行序直取（v4 五分组序 + 2026-11-10 音频增益追加）：
        //   0=位置(x,y；槽 2=层级→独立「层级 Z」行) 1=尺寸(w,h) 2=旋转(俯仰,偏航,滚转)
        //   3..6=裁切 上/下/左/右 7..10=角点（corner[(i-7)*2+分量]）
        //   11=不透明度 12=音频增益（resolveGain 独立解算）
        double v[3] = {0.0, 0.0, 0.0};
        switch (i) {
            case 0:
                v[0] = shown.x;
                v[1] = shown.y;
                // 【2026-11-18】v[2]（层级）不再灌位置行——层级行独立灌
                // 静态基准（见下方层级行灌值块）；槽 2 对齐同处处理
                break;
            case 1: // 尺寸行显示兜底（2026-11-20）：transform.w/h 未设（<=0，
                    // 新建/拖入片段默认态）且无 Size 通道时，显示素材原始
                    // 宽高（size_hint_，适配器从绑定素材注入）——与画布
                    // clipCanvasRect auto-size 兜底同源同值，属性面板不再
                    // 显示 0×0。数据层保持 0：display 端 SceneAssembler 对
                    // w=h=0 的画布 contain 适配语义不变；用户改值即双模
                    // 直写落 transform，自然显式化。
                v[0] = shown.w > 0 ? shown.w : (has ? 0.0 : size_hint_w_);
                v[1] = shown.h > 0 ? shown.h : (has ? 0.0 : size_hint_h_);
                break;
            case 2: // 旋转三维打包：X 俯仰/Y 偏航/Z 滚转
                v[0] = shown.rot_x;
                v[1] = shown.rot_y;
                v[2] = shown.rotation;
                break;
            case 3:  v[0] = shown.crop[kCropT];           break;
            case 4:  v[0] = shown.crop[kCropB];           break;
            case 5:  v[0] = shown.crop[kCropL];           break;
            case 6:  v[0] = shown.crop[kCropR];           break;
            case 7: case 8: case 9: case 10:
                v[0] = shown.corner[(i - 7) * 2];
                v[1] = shown.corner[(i - 7) * 2 + 1];
                break;
            case 11: v[0] = shown.opacity;                break;
            case 12: v[0] = gain_shown;                   break;
            case 13: // RGB 增益三维打包：R/G/B 线性增益
                v[0] = shown.rgb_gain[0];
                v[1] = shown.rgb_gain[1];
                v[2] = shown.rgb_gain[2];
                break;
            case 14: v[0] = shown.gamma;                  break;
        }
        for (int comp = 0; comp < r.components; ++comp) {
            if (r.value[comp] && !r.value[comp]->hasFocus()) {
                const QSignalBlocker b(r.value[comp]);
                r.value[comp]->setValue(v[comp]);
            }
            // 静态暂存对齐工作副本（脏位在位也写同值——外部改动不被陈旧
            // 暂存回退，同 ClipPropertyPanel::refreshFrom 的合并无扰动论证）
            if (!has) static_vals_[kRowSlot[i] + comp] = v[comp];
        }
        // 菱形三态（字符方案——无信号，无需 QSignalBlocker）
        const int hit = has ? keyIndexAtPlayhead(*ch) : -1;
        if (r.enable)
            setDiamondVisual(r.enable, !has ? 0 : (hit >= 0 ? 2 : 1));
    }

    // ---- 锚点灌值（静态属性；resolveTransform 结果含全部静态字段——
    //      一个 resolve 覆盖，绝无"两个值"）--------------------------------
    if (anchor_combo_) {
        const QSignalBlocker b(anchor_combo_);
        anchor_val_ = shown.anchor;
        anchor_combo_->setCurrentIndex(
            std::clamp(shown.anchor, 0, 4));
    }

    // ---- 层级 Z / 深度 Z 灌值（2026-11-18 语义终定）------------------------
    // 层级框恒显静态基准 c.transform.z（层级行无动画入口——与“静态属性”
    // 定位一致；老工程若有层级 tween，时间轴仍按通道动画，本框显示基准）。
    // 深度 Z 同锚点纪律——pos_z 不进 tween，shown.pos_z 恒等于静态基准。
    z_shown_ = shown.z; // Position 打键 v2 携带源（通道启用时 = 插值层级）
    if (z_layer_spin_) {
        const QSignalBlocker b(z_layer_spin_);
        z_layer_spin_->setValue(
            std::clamp((int)c.transform.z, -1000, 1000));
    }
    // 槽 2 对齐（原行循环口径迁移）：位置通道未启用时跟随工作副本
    {
        const TweenChannel* pos_ch = findChannel(TweenProp::Position);
        if (!(pos_ch && !pos_ch->keys.empty()))
            static_vals_[2] = shown.z;
    }
    if (pos_z_spin_) {
        const QSignalBlocker b(pos_z_spin_);
        pos_z_val_ = shown.pos_z;
        pos_z_spin_->setValue(shown.pos_z);
    }

    // ---- 遮挡次序灌值（2026-11-17 阶段③；clip.stacking 非 transform 域
    //      ——resolveTransform 不覆盖，直接取工作副本）--------------------
    if (stacking_combo_) {
        const QSignalBlocker b(stacking_combo_);
        stacking_val_ = c.stacking == kStackingPositionZ ? 1 : 0;
        stacking_combo_->setCurrentIndex(stacking_val_);
    }
}

} // namespace ms
