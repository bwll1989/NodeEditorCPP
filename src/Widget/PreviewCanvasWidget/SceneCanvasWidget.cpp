// ============================================================================
//  SceneCanvasWidget.cpp —— Scene 预览画布实现（第五迭代：3D 视口）
//
//  【3D 视口（2026-11-05 用户定案"整个画布预览控件改为三维的"）】
//  Blender 式正交 3D 视口：画布矩形躺在地面（h=0），片段按 pos_z
//  （深度 Z = 三维空间真实 z 位置）悬浮其上；叠放层级 transform.z
//  不再参与空间高度，只作遮挡排序键。地面网格 + 三轴参照线。
//    * 投影：正交（Camera3D.hpp）——水平面矩形投影后仍是平行四边形，
//      QTransform 仿射即可精确贴片段纹理，零 GL 管线改动；
//    * 相机：右键拖拽 orbit（yaw/pitch）、滚轮 dolly（光标锚定）、
//      中键/左键空白平移（光标下世界点跟随）、双击空白复位；
//    * 拾取/拖拽：视线与对象所在高度平面求交反投影回画布域——拖动/
//      缩放/旋转/吸附的几何数学与 2D 逐位复用（CanvasGeometry）；
//    * 贴图：GpuDeliver 逐项交付的 QImage 源帧按几何元组匹配片段；
//      CUDA native 项经 gpu_delivery_ok=false 协商回退下载路径（解码端
//      每帧检查旗标自动回退，零引擎改动）；无交付 = 棋盘格占位；
//    * 回退：MS_CANVAS_3D=0 恢复历史 2D fit 视图（GL 直通全链路保留）。
//
//  视觉约定（2D 回退路径 = 第四迭代历史形态，逐字节保留）：
//    * 控件底纯黑 = 合成输出底色：整个预览窗口即画布本体（无外留边、
//      无画布描边、无尺寸标注；fit 铺满客户区保持纵横比）；
//      坐标轴 = 过画布中心的十字线（2026-11-05 用户两轮定案：先简化为
//      仅两条参照线，再纠正为十字——边线 L 形只覆盖一个象限的方位，
//      十字才给自由缩放/平移的视图完整的位置参照；无刻度无数值，
//      随画布矩形联动）；
//    * screen：灰描边矩形 + 左上角标签（id → player）+ 在线圆点；
//    * 片段：棋盘格占位（"媒体存在、尚未实时渲染"的行业画法）+
//      轨道类型色边框 + 左上角标签（clip id）；
//    * 选中：红色虚线框 + 四角手柄 + 旋转拖柄。
//
//  交互约定（3D / 2D 共用骨架，仅坐标换算分发不同）：
//    * 左键按下：命中测试 → 选中重绘；命中即进入拖动（增量写工作副本）；
//    * 角点缩放：选中对象四角画手柄（旋转后角点位置）——压在手柄按下
//      进入缩放会话（优先于命中测试），锚定对角实时解算新外接框
//      （旋转对象走初始局部坐标系，见 model/CanvasGeometry.hpp）；
//      Shift = 等比约束（新框与原框相似，比例取包住鼠标的大轴）；
//    * 旋转拖柄：选中对象顶中外延圆点（随对象旋转）——拖动按"方向角
//      增量"实时改 rotation，Shift 吸附 15°；
//    * 松开：sceneEdited 信号（宿主置脏 + 自动下发链）；
//    * 拖动/缩放/旋转会话中：sceneEditedLive 高频（display 节流下发持续跟手）；
//    * 双击：片段 = 仅选中（弹属性面板已移除，2026-11-05 用户定案——
//      属性面板唯一入口 = timeline 双击）/ 屏幕 = 仅选中（属性对话框
//      双击入口已移除，2026-11-25 用户定案——唯一入口 = 右键菜单）；
//    * 右键：3D 模式 = 拖拽旋转视角（真位移）/ 无位移弹菜单；2D = 菜单；
//      菜单内容：空白 = 添加屏幕（统一 Screen 模型，姿态在属性里调）；屏幕 = 属性/
//      删除；片段 = 删除（单动作）。
// ============================================================================

#include "SceneCanvasWidget.hpp"

#include <QContextMenuEvent>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QOpenGLContext> // 析构 GL 资源释放前的上下文有效性检查
#include <QPainter>
#include <QPainterPath>
#include <QShortcut> // 场景撤销/重做（焦点作用域快捷键，2026-11-26）
#include <QApplication> // qApp focusChanged：节点图聚焦时禁用场景撤销快捷键（二义互斥）
#include <QPaintEvent>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <vector>

#include "PreviewRenderer.hpp" // 预览渲染（render 成功 → 真实合成帧上屏）
#include "model/CanvasGeometry.hpp" // 角点缩放/旋转命中纯函数（core 共享）
#include "engine/TweenEngine.hpp" // tween 通道求值纯函数（画布跟随曲线）
#include "GlRenderer.hpp"   // GL 合成后端（ExternalCtx 模式；GL 直通路径）
#include "Canvas3DGl.hpp"   // 3D 视口贴图层（阶段1 GL化：DXT 直传+透视校正）
#include "media/DxtDecode.hpp" // DXT 现场解码（QPainter 回退路径兜底；软解退役）
#include "../TimeLineWidget/MsRuntimeBridge.hpp" // 下发运行时桥（银幕操作执行通道）
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp" // Eye 对话框数值控件
#include "../TimeLineWidget/QtTimelineModelAdapter.hpp" // 模型对外接口（下发/同步委托目标）
#include "Common/TimeLineBase/BaseTimeLineWidget.h" // 时间轴控件（选中联动信号源；setTimelineWidget）
#include <QItemSelectionModel> // 时间轴选中 currentChanged（QModelIndex 换算入参）
#include "ScreenPropertyDialog.hpp" // 屏幕属性对话框（银幕右键/双击直调）
// 注：本 TU 不 include GlLoader.hpp——Qt GL 头（QOpenGLWidget→qopengl.h）
// 已定义 GL_* 宏，与 GlLoader 的常量枚举冲突；GL 诊断在 GlRenderer 内部做。

namespace ms {

namespace cg = canvas_geometry;

namespace {
// ---------------------------------------------------------------------------
// 轨道类型色（3D 标注层与 2D drawClip 两处同源共用——2026-11-06 收敛原
// 两份相同 switch；色值与媒体插件 */*ClipModel.hpp 的 setColor 同组，视觉
// 分组统一：插件侧属插件 API 不便引用，改色时仍需两处同步，画布内部已单源）
// ---------------------------------------------------------------------------
QColor trackKindColor(AssetKind kind) {
    switch (kind) {
        case AssetKind::Video: return QColor(0xE6, 0x8A, 0x33); // 橙
        case AssetKind::Image: return QColor(0x33, 0xB2, 0xA6); // 青
        case AssetKind::Text:  return QColor(0xD9, 0xC2, 0x3C); // 黄
        case AssetKind::Audio: return QColor(0x9B, 0x6F, 0xD1); // 紫
        case AssetKind::Live:  return QColor(0x3C, 0xC4, 0xD9); // 亮青（实时源）
        default:               return QColor(0x88, 0x88, 0x88); // 未知·灰
    }
}
/// 【画布隐形轨道】音频/Control 无图像（WATCHOUT Stage 语义：声音对象/
/// 指令片段不占视觉位）——绘制、3D prim 收集、命中测试四处同口径跳过
///（Control 2026-11-13 加入，与 Audio 同口径：画布完全隐形防挡点击）。
/// 【2026-11-15 片段类型感知】轨道去类型化后 track.kind 只是可选 hint
///（无类型轨恒 Unknown）——判定跟片段自身类型串走（kind 仅作旧文件兼容
/// 兜底）；Artnet/Mapping 同为无图像片段（2026-11-15 移植）一并隐形。
inline bool trackInvisibleOnCanvas(AssetKind kind,
                                   const std::string& clip_type = {}) {
    return kind == AssetKind::Audio || kind == AssetKind::Control ||
           kind == AssetKind::Artnet || kind == AssetKind::Mapping ||
           clip_type == "Audio" || clip_type == "Control" ||
           clip_type == "Artnet" || clip_type == "Mapping";
}
/// 片段矩形变形感知命中（2026-11-09 锚点/角点改造；原 hitTest 内联 lambda
/// 提升为文件级【2026-11-15 选中分工】——hitTest（screen-only）与"点击
/// 已选中 clip 进拖动"特判两处共用同一几何判定，防两处漂移）：
///   * 默认（中心锚无位移）→ rotatedRectContains（逐位不变）；
///   * 仅锚点变化 → 点绕锚点逆旋转回未旋转矩形局部系再判包含
///     （数学 = rotatedRectContains 平移锚点版）；
///   * 角点位移 / 3D 姿态 → 位移后四角 AABB（四边形可能非凸，AABB
///     简单可靠；编辑器命中精度足够——绘制边框仍是四边形本体）
/// 【阶段③ 命中精确化 2026-11-17】判定升级 pose3DActive（含 pos_z）+
/// quadCorners3DP 统一透视投影（几何深度 + 全场透视参数——命中框与绘制/
/// display 同源唯一真相；persp 三参由调用方从 Scene 透传）。
/// 【2026-11-20 修订】三轴旋转恢复透视投影（正交扁平化曾短暂替代，用户
/// 目验旋转观感后拍板透视）——命中框与绘制/合成同源唯一真相不变。
inline bool clipRectContains(const QRectF& r, const Transform& tf,
                             const QPointF& p, double persp_focal,
                             double persp_vp_x, double persp_vp_y) {
    if (r.width() <= 0 || r.height() <= 0) return false;
    if (tf.pose3DActive()) {
        // 3D 位姿（2026-11-10 姿态；2026-11-17 阶段③含 pos_z）：quadCorners3DP
        // 预投影四角 AABB——透视四边形可能非凸，AABB 简单可靠（与角点
        // 位移分支同策略）；内含滚转/锚点/位移/深度/全场透视全部语义。
        double wx[4], wy[4];
        cg::quadCorners3DP(r.x(), r.y(), r.width(), r.height(),
                           tf.pos_z, tf.rotation, tf.rot_x, tf.rot_y,
                           tf.anchor, tf.corner,
                           persp_focal, persp_vp_x, persp_vp_y, wx, wy,
                           nullptr);
        double minx = wx[0], miny = wy[0], maxx = wx[0], maxy = wy[0];
        for (int i = 1; i < 4; ++i) {
            minx = std::min(minx, wx[i]);
            maxx = std::max(maxx, wx[i]);
            miny = std::min(miny, wy[i]);
            maxy = std::max(maxy, wy[i]);
        }
        return p.x() >= minx && p.x() <= maxx && p.y() >= miny &&
               p.y() <= maxy;
    }
    if (tf.anchor == 0 && !tf.cornerActive())
        return cg::rotatedRectContains(r.x(), r.y(), r.width(),
                                       r.height(), tf.rotation, p.x(),
                                       p.y());
    double au = 0.5, av = 0.5;
    Transform::anchorUV(tf.anchor, au, av);
    if (tf.cornerActive()) {
        // 位移后四角 AABB——四角生成走 cg::quadCornersWithAnchor
        //（唯一真相，2026-11-09 X 形根治：此前手写副本 bit 模式
        // zigzag + i*2 直索引把 RB/LB 位移互换，拖右下角命中框错
        // 跟左下角）。四边形可能非凸，AABB 简单可靠。
        double wx[4], wy[4];
        cg::quadCornersWithAnchor(r.x(), r.y(), r.width(), r.height(),
                                  tf.rotation, tf.anchor, tf.corner,
                                  wx, wy);
        double minx = wx[0], miny = wy[0], maxx = wx[0], maxy = wy[0];
        for (int i = 1; i < 4; ++i) {
            minx = std::min(minx, wx[i]);
            maxx = std::max(maxx, wx[i]);
            miny = std::min(miny, wy[i]);
            maxy = std::max(maxy, wy[i]);
        }
        return p.x() >= minx && p.x() <= maxx && p.y() >= miny &&
               p.y() <= maxy;
    }
    // 仅锚点：local = R(-θ)·(p − anchorPos) + anchorLocal ∈ [0,w]×[0,h]
    const double rad = tf.rotation * 3.14159265358979323846 / 180.0;
    const double ct = std::cos(rad), stn = std::sin(rad);
    const double dx = p.x() - (r.x() + au * r.width());
    const double dy = p.y() - (r.y() + av * r.height());
    const double lx = ct * dx + stn * dy + au * r.width();
    const double ly = -stn * dx + ct * dy + av * r.height();
    return lx >= 0.0 && ly >= 0.0 && lx <= r.width() &&
           ly <= r.height();
}

/// 裁切语义（2026-11-17 预览对齐 display）：名义矩形收缩为可见矩形 +
/// 帧采样 UV 窗（归一化）——与 ScreenMapper::buildDrawItemsForScreen
/// 同式同源（几何全按裁后可见矩形解算 + uv 窗换算，内容不缩放：
/// 裁掉部分从采样窗移除，可见区仍铺满裁后矩形）。
/// \param tf  片段变换（tween 求值后）
/// \param r   [in/out] 名义画布矩形 → 裁切生效时替换为可见矩形
///            （Transform::visibleRect 同式：各边内收负值视 0、可见
///            宽高钳 ≥1px 过裁不翻转）
/// \param uv  [out] {u0,v0,u1,v1} 采样窗（未裁切 = 单位窗 {0,0,1,1}；
///            分母 = 未裁切名义尺寸，与 ScreenMapper eff_w/h 同口径）
/// \return true = 裁切生效（r 已替换）
inline bool clipCropApply(const Transform& tf, QRectF& r, double uv[4]) {
    uv[0] = 0.0; uv[1] = 0.0; uv[2] = 1.0; uv[3] = 1.0;
    if (!tf.cropActive()) return false;
    const double ew = r.width(), eh = r.height();
    if (ew <= 0.0 || eh <= 0.0) return false;
    // uv 窗（分母 = 未裁切名义尺寸）
    uv[0] = std::clamp(tf.crop[0] / ew, 0.0, 1.0);         // 左
    uv[1] = std::clamp(tf.crop[2] / eh, 0.0, 1.0);         // 上
    uv[2] = std::clamp(1.0 - tf.crop[1] / ew, uv[0], 1.0); // 右
    uv[3] = std::clamp(1.0 - tf.crop[3] / eh, uv[1], 1.0); // 下
    // 可见矩形（Transform::visibleRect 同式）
    const double cl = std::clamp(tf.crop[0], 0.0, std::max(0.0, ew - 1.0));
    const double cr = std::clamp(tf.crop[1], 0.0, std::max(0.0, ew - 1.0));
    const double ct = std::clamp(tf.crop[2], 0.0, std::max(0.0, eh - 1.0));
    const double cb = std::clamp(tf.crop[3], 0.0, std::max(0.0, eh - 1.0));
    r = QRectF(r.x() + cl, r.y() + ct, std::max(ew - cl - cr, 1.0),
               std::max(eh - ct - cb, 1.0));
    return true;
}

/// 交付项 → GL 帧视图双域填充（DXT/CPU；GpuCuda 域由调用方特判——合成
/// FBO 路径 native 前置分支）。消重自 composeGpuFrame 同构填充
///（2026-11-18 整理；屏面片路径已退役 2026-11-20 用户定案）。
/// \return true = fv 已填充（调用方可 draw）；false = native 域（frame
///         与 dxt 均空，调用方跳过或占位）
inline bool deliverFrameView(const PreviewRenderer::DeliverItem& d,
                             FrameView& fv) {
    if (d.dxt) {
        // ---- DXT 压缩域（HAP 直传；GlRenderer CpuDxt 段消费）------------
        // 裸块指针 + GL 格式 + 块行距直透；dxt_keep 保活 PublishedFrame
        //（draw 同步完成，交付项生命周期覆盖绘制全程）。
        fv.domain     = FrameDomain::CpuDxt;
        fv.dxt        = d.dxt;
        fv.dxt_format = d.dxt_format;
        fv.stride     = d.dxt_stride;
        fv.width      = d.gpu_w;
        fv.height     = d.gpu_h;
        fv.keep       = d.dxt_keep;
        return true;
    }
    if (d.frame.isNull()) return false;
    // ---- CPU 域（软解 / 硬解下载回退 / Live 快照）：RGBA8888 内存序 =
    // GL_RGBA 直传。keep 透传（零拷贝交付项——frame_keep 保活外部快照
    // 缓冲到绘制结束；深拷贝交付项为空，QImage 自持无悬垂窗口）
    fv.width  = d.frame.width();
    fv.height = d.frame.height();
    fv.stride = static_cast<std::uint32_t>(d.frame.bytesPerLine());
    fv.rgba   = d.frame.constBits();
    fv.domain = FrameDomain::CpuRgba;
    fv.keep   = d.frame_keep;
    return true;
}

/// 片段级调色（2026-11-20 预览对齐；QPainter 回退路径专用——GL 主路径
/// 在 Canvas3DGl shader / 2D 路径 GlRenderer uniform 里消费）。
/// 与 display 端 GlRenderer / SoftRenderer 同式：先逐通道乘线性增益
/// [0,2]，γ≠1 再 pow(clamp(c,0,1), 1/γ)（>1 提亮中间调、<1 压暗）。
/// 256 项 per-channel LUT 预解算（pow 每像素一次降为每帧 768 次）。
/// 调用方已过滤全默认——本函数恒产出独占副本（快照 QImage 被贴图表/
/// worker 共享，严禁原地改写）。
inline QImage applyFaceColor(const QImage& src, const double rgb_gain[3],
                             double gamma) {
    QImage out = src.format() == QImage::Format_RGBA8888
                     ? src
                     : src.convertToFormat(QImage::Format_RGBA8888);
    out.detach();
    std::uint8_t lut[3][256];
    const bool g_corr = std::fabs(gamma - 1.0) > 1e-6;
    for (int c = 0; c < 3; ++c)
        for (int v = 0; v < 256; ++v) {
            double x = (v / 255.0) * rgb_gain[c]; // 增益
            if (g_corr)
                x = std::pow(std::clamp(x, 0.0, 1.0), 1.0 / gamma); // 伽马
            lut[c][v] = static_cast<std::uint8_t>(
                std::clamp(x * 255.0 + 0.5, 0.0, 255.0));
        }
    for (int y = 0; y < out.height(); ++y) {
        auto* row = out.scanLine(y); // RGBA8888 恒 4B/px
        for (int x = 0; x < out.width(); ++x, row += 4) {
            row[0] = lut[0][row[0]];
            row[1] = lut[1][row[1]];
            row[2] = lut[2][row[2]];
        }
    }
    return out;
}
} // namespace

// ---------------------------------------------------------------------------
// 构造与外部接口
// ---------------------------------------------------------------------------
SceneCanvasWidget::SceneCanvasWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setMouseTracking(true); // 悬停也收 move（拖动顺畅）
    setMinimumSize(280, 200);
    // 键盘微调前提：单击画布即获得焦点（方向键调整大小/移动；不抢 Tab 焦点链
    // ——ClickFocus 只在鼠标点击后参与键盘事件分发）
    setFocusPolicy(Qt::ClickFocus);

    // 【场景撤销/重做快捷键（2026-11-26，场景域=时间轴+画布共享）】
    // WindowShortcut（父控件=画布→窗口=主窗口）：主窗口激活即生效——
    // 时间轴子树视图不保证持焦点（此前 WidgetShortcut 挂时间轴实测无效）。
    // ★动态互斥（2026-11-26 二义实测修复）：焦点进入节点图子树时禁用
    // 本快捷键。Qt 对同键候选不按作用域自动裁决——节点图 undo/redo
    // QAction（作用域已在源头 3rdParty/nodeeditor/src/GraphicsView.cpp
    // 补设为 WidgetShortcut）与本 WindowShortcut 快捷键并存时报
    // "QAction::event: Ambiguous shortcut overload: Ctrl+Z"，两侧都失
    // 灵。focusChanged 同步 setEnabled 后任意时刻同键只剩一个候选：节点
    // 图聚焦→节点撤销独占；其余（时间轴/画布/媒体库…）→场景撤销独占。识别节点图
    // 走 property("nodeEditorView")（CustomGraphicsView 构造时打标，
    // 画布不 include 节点侧头文件）。文本框聚焦走原生文本撤销
    // （QLineEdit ShortcutOverride）。执行在适配器 sceneUndo/sceneRedo
    // （画布装配枢纽：适配器未注入时零动作）。
    auto* sceneUndoShortcut =
        new QShortcut(QKeySequence::Undo, this); // Ctrl+Z
    auto* sceneRedoShortcut =
        new QShortcut(QKeySequence::Redo, this); // Ctrl+Y / Ctrl+Shift+Z
    sceneUndoShortcut->setContext(Qt::WindowShortcut);
    sceneRedoShortcut->setContext(Qt::WindowShortcut);
    connect(sceneUndoShortcut, &QShortcut::activated, this,
            [this] { execSceneUndoRedo(true); });
    connect(sceneRedoShortcut, &QShortcut::activated, this,
            [this] { execSceneUndoRedo(false); });
    const auto applyNodeFocusGate = [sceneUndoShortcut,
                                     sceneRedoShortcut](QWidget* now) {
        bool inNodeEditor = false;
        for (QWidget* w = now; w; w = w->parentWidget()) {
            if (w->property("nodeEditorView").toBool()) {
                inNodeEditor = true;
                break;
            }
        }
        sceneUndoShortcut->setEnabled(!inNodeEditor);
        sceneRedoShortcut->setEnabled(!inNodeEditor);
    };
    connect(qApp, &QApplication::focusChanged, this,
            [applyNodeFocusGate](QWidget*, QWidget* now) {
                applyNodeFocusGate(now);
            });
    applyNodeFocusGate(QApplication::focusWidget()); // 启动即同步一次

    // 【3D 视口开关（2026-11-05 画布 3D 化）】缺省开（用户定案"整个画布
    // 预览控件改为三维的"）；MS_CANVAS_3D=0 回退历史 2D fit 视图——
    // 排障/回归对照开关（2D 路径含 GL 直通全链路，逐字节保留）。
    if (qEnvironmentVariable("MS_CANVAS_3D") == QLatin1String("0"))
        view3d_ = false;

    // 【第八迭代：拉帧泵】预览窗口定时从引擎取帧——预览不管当前时间、
    // 不驱动渲染，只以固定帧率从引擎拿最新帧（对齐 display 端架构）。
    // 【显示端解耦】播放头推进也不在这里：preview_ 播放头由 EditorWindow
    // 的会话时钟回显（echoSessionClock → seekTo，~33ms）统一喂给——
    // 引擎只是"按当前时间渲染帧"的执行器，窗口只拉帧。泵内两步
    // （GUI 侧轻量操作，解码在引擎工作线程）：
    //   pump  —— 引擎检测输入变化提交快照（内置自适应节流：解码慢自动降频）；
    //   pull  —— 取最新完成帧，变化才 update()（静态画面零重绘）。
    pull_timer_ = new QTimer(this);
    pull_timer_->setTimerType(Qt::PreciseTimer); // 拉帧泵不许粗化（跳帧修复）
    connect(pull_timer_, &QTimer::timeout, this, [this] {
        if (!preview_ || !scene_) return;
        const QRectF cr   = canvasRect();
        const double dpr  = devicePixelRatioF();
        // 【合成降采样】目标合成尺寸 = 内容域控件映射的设备像素（fit 显示
        // 区域）。域随内容变化（素材拖出画布 → 域扩展），用最近一帧的域，
        // 内容出界后下一拍收敛（合成尺寸自动跟随内容包围盒）。
        const QRectF dom = frame_domain_.width() > 0 && frame_domain_.height() > 0
                               ? frame_domain_ : QRectF(0, 0, scene_->canvas_w,
                                                        scene_->canvas_h);
        const double k = cr.width() / scene_->canvas_w; // 控件像素/画布单位
        preview_->setTargetSize((int)std::lround(dom.width() * k * dpr),
                                (int)std::lround(dom.height() * k * dpr));
        const auto pump_t0 = std::chrono::steady_clock::now(); // 【诊断】泵拍计时
        preview_->pump(); // 状态变化 → 提交快照（自适应节流在引擎内）
        QImage f;
        QRectF  d;
        if (preview_->pull(f, &d) && !f.isNull() &&
            f.cacheKey() != latest_frame_key_) {
            latest_frame_     = f;
            latest_frame_key_ = f.cacheKey();
            frame_domain_     = d; // 帧与域同源更新（贴图按此映射）
            update(); // 新帧才重绘（标注层重绘由交互路径自行触发）
        }
        { // 【诊断 MS_CANVAS_DBG=1】泵拍耗时（pump+pull 全程）——33ms 定时器
          // 被饿到 250ms 的阻塞源二分：泵自身重（快照构建/QImage 拷贝）
          // 还是事件循环被其它活动（paintGL/发布槽）占死。
          // 【2026-11-05 修复】补上门控——此前注释声称门控实际无条件
          // 打印前 600 拍（约 20s 刷屏），用户误以为程序异常。
            static const bool s_dbg =
                std::getenv("MS_CANVAS_DBG") != nullptr;
            static int s_n = 0;
            if (s_dbg && ++s_n <= 600 && s_n % 10 == 0) {
                const auto us = std::chrono::duration_cast<
                    std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - pump_t0).count();
                std::fprintf(stderr, "canvas-dbg: pumpTick cost=%lldus\n",
                             (long long)us);
                std::fflush(stderr);
            }
        }
        { // 【诊断 MS_CANVAS_DBG=1】事件循环延迟探针——singleShot(0) 从
          // 投递到执行的滞后 = 事件循环被其它活动占死的直接度量（对拍
          // paintGL RAII 探针测不到的 swapBuffers/vsync 阻塞等 Qt 内部
          // 开销；lag>100ms 即事件循环存在 100ms 级阻塞活动）。
          // 【2026-11-05 修复】门控在外层——关闭时连 singleShot 都不
          // 投递，常态零开销零打印（此前无条件打印前 600 拍 ≈20s 刷屏）。
            static const bool s_dbg =
                std::getenv("MS_CANVAS_DBG") != nullptr;
            if (s_dbg) {
                const auto enq_us = std::chrono::duration_cast<
                    std::chrono::microseconds>(
                    std::chrono::steady_clock::now()
                        .time_since_epoch()).count();
                QTimer::singleShot(0, this, [enq_us] {
                    static int s_l = 0;
                    if (++s_l > 600 || s_l % 5 != 0) return;
                    const auto lag_ms = std::chrono::duration_cast<
                        std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch() -
                        std::chrono::microseconds(enq_us)).count();
                    std::fprintf(stderr, "canvas-dbg: loopLag=%lldms\n",
                                 (long long)lag_ms);
                    std::fflush(stderr);
                });
            }
        }
    });
    pull_timer_->start(33);

    // 【200ms UI 拍（2026-11-16 自宿主 msUiTimer 收编）】显示端状态心跳：
    //   ① displayRows → setDisplayOnlineStates 角标刷新（幂等，状态不变
    //      零重绘）；② 与上一拍在线集合（last_connected_）对比检出"新上
    //      线"→ 自动追平下发当前场景——覆盖"display 晚于项目打开上线"路
    //      径：注册应答只在有权威快照时单发，且此前编辑改动不会因 display
    //      上线而自动补发。广播对老端无害（同内容幂等）。
    //   runtime_/adapter_ 后注入（构造序晚于本控件）——未注入或 runtime
    //   未启动时空转（首行判空）。
    ui_timer_ = new QTimer(this);
    ui_timer_->setTimerType(Qt::CoarseTimer);
    connect(ui_timer_, &QTimer::timeout, this, [this] {
        // 【弹窗 display 名称行联动（2026-11-25）】各属性框开着 → 按各自
        // 当前绑定重查登记快照重设文案（display 打开期间上/下线弹窗内
        // 实时可见；纯展示零下发）。添加流（空 key）无 sid 跳过。
        for (auto& kv : prop_dlgs_) {
            if (kv.first.empty() || !scene_) continue;
            for (const auto& s : scene_->screens)
                if (s.id == kv.first) {
                    kv.second->setDisplayInfo(displayInfoLine(s.player_id));
                    break;
                }
        }
        if (!runtime_ || !runtime_->running()) return;
        const auto rows = runtime_->displayRows();
        std::map<std::string, bool> states;
        std::set<std::string> cur_conn;
        for (const auto& r : rows) {
            if (r.enabled)
                states.emplace(r.id.toStdString(), r.connected);
            if (r.connected && r.enabled)
                cur_conn.insert(r.id.toStdString());
        }
        setDisplayOnlineStates(states);
        bool newcomer = false;
        for (const auto& id : cur_conn)
            if (!last_connected_.count(id)) { newcomer = true; break; }
        if (newcomer && scene_ && adapter_ && adapter_->publishScene())
            statusTip("显示端上线，场景已自动追平下发");
        last_connected_ = std::move(cur_conn);
    });
    ui_timer_->start(200);

    // 【诊断 MS_CANVAS_SHOT=path】进程内抓帧：启动 3 秒后把 FBO 内容存为
    // PNG。此前用屏幕截图验证标注层绘制，但 editor 由后台进程启动时无法
    // 抢前台（Windows SetForegroundWindow 限制），ImageGrab 截到的是遮在
    // 上面的其他窗口——"轴线画不出来"的截图证据全部无效。改从
    // QOpenGLWidget::grabFramebuffer() 直接读 FBO（不依赖窗口可见性、不
    // 重入 paintGL），读到的是 GPU 后备缓冲的真实合成结果。
    if (const char* shot = std::getenv("MS_CANVAS_SHOT")) {
        const QString path = QString::fromLocal8Bit(shot);
        QTimer::singleShot(3000, this, [this, path] {
            const QImage img = grabFramebuffer();
            if (img.save(path))
                std::fprintf(stderr, "canvas-shot: 已保存 %s (%dx%d)\n",
                             path.toLocal8Bit().constData(), img.width(),
                             img.height());
            else
                std::fprintf(stderr, "canvas-shot: 保存失败: %s\n",
                             path.toLocal8Bit().constData());
            std::fflush(stderr);
        });
    }
}

// 【诊断（2026-11-05 seek 跳帧定位）】paint 事件全程计时——覆盖 Qt 私有的
// makeCurrent→paintGL→swapBuffers→doneCurrent 全链（paintGL 内 RAII 只测
// 用户绘制段；swap 若阻塞（vsync/DWM/驱动节流）即 WM_TIMER 被挡 250ms 元凶）
void SceneCanvasWidget::paintEvent(QPaintEvent* event) {
    const auto pe_t0 = std::chrono::steady_clock::now();
    QOpenGLWidget::paintEvent(event); // 基类全流程（GL 合成 + 标注 + 交换）
    static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
    if (s_dbg) {
        static int s_n = 0;
        // 每 5 次打 1 条；启动 600 次后降频（每 200 次打 1 条）——启动
        // 阶段 paint 风暴（窗口初始化/expose）会耗尽简单限幅的预算
        if (s_dbg && ++s_n % 5 == 0 &&
            (s_n <= 600 || s_n % 200 == 0)) {
            const auto wall = std::chrono::duration_cast<
                std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            const auto us = std::chrono::duration_cast<
                std::chrono::microseconds>(
                std::chrono::steady_clock::now() - pe_t0).count();
            std::fprintf(stderr,
                         "canvas-dbg: paintEvent#%d cost=%lldus wall=%lld\n",
                         s_n, (long long)us, (long long)wall);
            std::fflush(stderr);
        }
    }
}

// ---------------------------------------------------------------------------
// 析构（out-of-line）：unique_ptr<GlRenderer> 需要完整类型删除器——GL 对象
// 销毁在本 TU（已含 GlRenderer.hpp）。上下文此时可能已失效：glDelete*
// 在无 current context 时静默无效，驱动随上下文销毁统一回收，无泄漏。
// ---------------------------------------------------------------------------
SceneCanvasWidget::~SceneCanvasWidget() {
    // 【UAF 修复 2026-11-25（用户实测：开弹窗关程序 → 0xC0000374 堆损坏/
    // 挂起/忙鼠标）】本类成员（含 prop_dlgs_ map）在成员析构阶段先行销毁，
    // 而子对话框（QObject 子级）要等到 ~QObject::deleteChildren 才销毁——
    // 它们的 destroyed→本类擦除槽（openScreenProperty/addScreen 注册）会在
    // 成员已析构后遍历 prop_dlgs_ 自由内存 = UAF（Temp/uaf_test 实证：
    // bug 版 exit=127，析构先 disconnect 版 exit=0，各 3 次全一致）。
    // 修法：趁成员仍有效，断掉所有 子对话框→本类 连接；父链析构自然回收
    // 弹窗，不再触碰已亡成员。
    for (auto& kv : prop_dlgs_)
        if (kv.second) kv.second->disconnect(this);
    // GL 资源释放须在上下文 current 时进行（QOpenGLWidget 析构序列不保证
    // 上下文仍当前——显式 makeCurrent 兜底；从未显示过的控件无上下文，
    // 跳过即可：unique_ptr 析构 delete 空对象无害）
    if (context() && context()->isValid()) {
        makeCurrent();
        // 【崩溃修复 2026-11-18】gl3d_ 惰性创建（仅 3D 绘制路径 ever new），
        // 2D 会话析构时为空——此前无判空调用 shutdown() 读 [this+0xBC]
        // 直接 AV（崩溃 dmp 实证：~SceneCanvasWidget → Canvas3DGl::shutdown，
        // rip=+0x122106 rcx=0 fault=读 0xBC）。空 unique_ptr 跳过即可，
        // GL 资源本就未分配，无泄漏。
        if (gl3d_) gl3d_->shutdown(); // 3D 贴图层（纹理缓存/程序/VAO/VBO）
        gl_.reset();       // 2D GL 直通合成器
        doneCurrent();
    }
}

void SceneCanvasWidget::setScene(Scene* s) {
    scene_ = s;
    applySelection(Selection{}); // 场景指针切换，选中态作废
    dragging_ = false;
    resizing_ = false;    // 进行中的缩放/旋转会话同样作废（对象可能已不存在）
    rotating_ = false;
    resize_corner_ = -1;
    corner_drag_  = -1; // 素材角点/边线会话同样作废（2026-11-20）
    edge_drag_    = -1;
    corner_hover_ = -1;
    edge_hover_   = -1;
    // 【打开即铺满】场景加载/切换 → 视图复位（zoom=fit、pan=0）：预览视图
    // 恒定呈现"整个 scene 铺满预览区"（WATCHOUT 舞台语义）。没有这条，
    // 上次会话的滚轮缩放/平移会延续到新场景，打开即"画布缩在角落"。
    resetView();
    update();
}

void SceneCanvasWidget::setPreviewRenderer(PreviewRenderer* r) {
    preview_ = r; // 借用指针（EditorWindow 拥有；nullptr = 回退占位画法）
    // 【出帧重绘目标自注册（2026-11-16 自宿主回调收编）】本控件就是显示
    // 该渲染器帧的窗口——worker 出帧经 QMetaObject::invokeMethod 排队本
    // 控件 update（低延迟上屏；与 pull_timer_ 33ms 拉帧泵互补）。原宿主
    // setFrameReadyCallback 闭包捕获裸指针跨线程调用，现由渲染器内置
    // QPointer 目标托管。
    if (r) r->setRepaintTarget(this);
    // 【3D 模式贴图协商】3D 视口用 QPainter 逐片段贴 QImage 源帧——CUDA
    // 0 拷贝发布的 native AVFrame（显存指针）无法进 QPainter。协商机制：
    // 与解码端共享的 gpu_delivery_ok 旗标翻 false → FfmpegMediaSource
    // 每帧检查后自动回退下载路径 → DeliverItem.frame（QImage）有值。
    // 2D 模式不翻（GL 直通 0 拷贝链路保留）。翻旗只需一次（原子读写，
    // 解码端下一帧生效；首两帧可能仍到 native 项——该片段当拍棋盘格）。
    if (r && view3d_) {
        if (auto flag = r->gpuDeliveryFlag()) flag->store(false);
    }
    update();
}

void SceneCanvasWidget::refresh() {
    // 【卡死修复配套】工作副本内容变化（拖拽/属性编辑/增删）后，合成帧
    // 缓存必须失效——否则 render 的合成键（播放头/场景指针/尺寸）检测
    // 不到"同指针内容已变"，画布会一直显示旧帧。
    if (preview_) preview_->invalidate();
    update();
}

void SceneCanvasWidget::highlight(int kind, int a, int b) {
    applySelection(Selection{kind, a, b}); // 统一入口
}

// ---------------------------------------------------------------------------
// 选中统一入口：赋值 + 重绘。值未变化时静默返回——press 高频路径/外部
// highlight 重复调用不重复刷。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::applySelection(const Selection& s) {
    if (s == sel_) return;
    sel_ = s;
    vp_selected_ = false; // 对象选中收口：任何选中变化都让 VP Gizmo 退位
                          //（单选语义——时间轴联动/点对象/点空白同一入口）
    update();
}

// ---------------------------------------------------------------------------
// 视图复位：缩放回 100%（fit 适配）+ 平移归零
// 入口：双击空白处 / 画布右键菜单"重置视图"（滚轮/中键浏览后的"回家"）
// 3D 模式：相机归位（方位角 0 / 俯仰 35° / scale=fit / 注视点=画布中心）
// ---------------------------------------------------------------------------
void SceneCanvasWidget::resetView() {
    if (view3d_) {
        cam3d_         = Camera3D{}; // yaw=0 pitch=35（缺省视角）
        view3d_zoom_   = 1.0;        // 相对倍率复位（勿快照 fitScale——见 hpp 注释：
                                     //  setScene 布局前触发时 fitScale 是小窗口值，
                                     //  绝对比例由 paintGL3D 每帧按 fit×zoom 同步）
        cam3d_.tx = scene_ && scene_->canvas_w > 0 ? scene_->canvas_w / 2.0 : 0.0;
        cam3d_.tz = scene_ && scene_->canvas_h > 0 ? scene_->canvas_h / 2.0 : 0.0;
        update();
        return;
    }
    view_zoom_ = 1.0;
    view_pan_ = QPointF(0, 0);
    update();
}

// ---------------------------------------------------------------------------
// 视图"回家"状态判定（双击空白复位的触发条件 + 右键菜单条目显隐）
// 3D：相机任一参数偏离复位态；2D：zoom≠100% 或平移非零
// ---------------------------------------------------------------------------
bool SceneCanvasWidget::viewIsHome() const {
    if (view3d_) {
        const double htx = scene_ && scene_->canvas_w > 0
                               ? scene_->canvas_w / 2.0 : 0.0;
        const double htz = scene_ && scene_->canvas_h > 0
                               ? scene_->canvas_h / 2.0 : 0.0;
        const double eps = 1e-9;
        // 缩放比较用相对倍率 view3d_zoom_（cam3d_.scale 是派生值，随窗口变）
        return std::fabs(cam3d_.yaw_deg) < eps &&
               std::fabs(cam3d_.pitch_deg - Camera3D::kPitchHome) < eps &&
               std::fabs(view3d_zoom_ - 1.0) < eps &&
               std::fabs(cam3d_.tx - htx) < eps &&
               std::fabs(cam3d_.tz - htz) < eps;
    }
    return view_zoom_ == 1.0 && view_pan_.isNull();
}

// ---------------------------------------------------------------------------
// 右上角轴球视角控件（Blender navigation gizmo 同位同交互）：
//   * 三条属性轴按当前相机姿态投影成屏幕方向——轴球本身就是"现在朝哪看"
//     的罗盘。轴名与配色沿用【属性面板语义】（X=画布x 红 / Y=画布y 绿 /
//     Z=高度 蓝——与 ClipPropertyPanel"Z 位置"及地面三轴线同名同色）。
//     【命名注意 2026-11-05】Camera3D 内部世界系把高度记作 Y（画布 y 记
//     作 Z，见 Camera3D.hpp 文件头）——那是纯内部数学约定；用户可见的
//     轴球文案/视角提示一律按属性语义（Z=高度），两套名字差一个互换，
//     改这里时勿把内部变量名一起"纠正"（投影数学不动）。
//   * 端点球可点击：+X 右视 / −X 左视 / +Z 顶视（等价 2D 俯视，高度轴）/
//     +Y 正视 / −Y 背视（−Z 底视无意义不画——地面之下不可见）；
//   * 球体其它区域按下拖拽 = orbit（与右键拖拽同一会话通道）。
// 点击只改 yaw/pitch（注视点/缩放保持）——预设视角不破坏用户浏览状态。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::drawAxisGizmo(QPainter& p) const {
    const QPointF c(width() - 52.0, 64.0);   // 控件中心（HUD 文字下方）
    const double  R = 34.0;                  // 端点半径（球半径 = R+8）

    // 背景圆（半透明深底 + 淡边——与视口深色底协调）
    p.setPen(QPen(QColor(255, 255, 255, 38), 1.0));
    p.setBrush(QColor(0, 0, 0, 90));
    p.drawEllipse(c, R + 9.0, R + 9.0);

    // 三轴屏幕方向：单位轴向量 v 的屏幕 2D = (dot(v,R), −dot(v,U))
    double rx, rz, ux, uy, uz, fx, fy, fz;
    cam3d_.basis(&rx, &rz, &ux, &uy, &uz, &fx, &fy, &fz);
    struct AxisEnd { int id; QColor col; const char* label; QPointF dir; };
    // 【标签=属性语义】蓝球=高度轴标 "Z"（属性面板"Z 位置"=悬浮高度）、
    // 绿球=画布 y 标 "Y"——与地面三轴线（红X/绿Y/蓝Z）及 ClipPropertyPanel
    // 同名同色。dir 公式不变（与投影同源）：端点屏幕方向 = (dot(v,R),−dot(v,U))。
    const AxisEnd ends[5] = {
        {0, QColor(0xA8, 0x52, 0x52), "X", QPointF(rx,  -ux)},  // +X 画布右
        {1, QColor(0xA8, 0x52, 0x52), "",  QPointF(-rx,  ux)},  // −X 画布左
        {2, QColor(0x55, 0x78, 0xC0), "Z", QPointF(0.0, -uy)},  // +Z 高度（属性"Z 位置"）
        {3, QColor(0x52, 0xA0, 0x5E), "Y", QPointF(rz,  -uz)},  // +Y 画布下方
        {4, QColor(0x52, 0xA0, 0x5E), "",  QPointF(-rz,  uz)},  // −Y 画布背面
    };
    for (const AxisEnd& e : ends) {
        const QPointF end = c + e.dir * R;
        p.setPen(QPen(e.col, 1.6));
        p.setBrush(Qt::NoBrush);
        p.drawLine(c, end);
        if (e.label[0]) {
            // 正端：实心球 + 轴字母（可点击跳视角）
            p.setPen(Qt::NoPen);
            p.setBrush(e.col);
            p.drawEllipse(end, 8.0, 8.0);
            QFont f = p.font();
            f.setPointSize(7);
            f.setBold(true);
            p.setFont(f);
            p.setPen(QColor(255, 255, 255, 235));
            p.drawText(QRectF(end.x() - 8, end.y() - 8, 16, 16),
                       Qt::AlignCenter, e.label);
        } else {
            // 负端：空心球（同样可点击）
            p.setPen(QPen(e.col, 1.4));
            p.setBrush(QColor(30, 30, 30, 160));
            p.drawEllipse(end, 5.0, 5.0);
        }
    }
}

int SceneCanvasWidget::hitAxisGizmo(const QPointF& wp) const {
    const QPointF c(width() - 52.0, 64.0);
    const double  R = 34.0;
    // 三轴屏幕方向（与绘制同式——命中检测必须与视觉一致；端点编号语义：
    // 0/1=±X 画布左右，2=+Z 高度顶视，3/4=±Y 画布正/背面）
    double rx, rz, ux, uy, uz, fx, fy, fz;
    cam3d_.basis(&rx, &rz, &ux, &uy, &uz, &fx, &fy, &fz);
    const struct { int id; QPointF dir; } ends[5] = {
        {0, QPointF(rx,  -ux)}, {1, QPointF(-rx,  ux)},
        {2, QPointF(0.0, -uy)},
        {3, QPointF(rz,  -uz)}, {4, QPointF(-rz,  uz)},
    };
    // 端点球优先（取最近——低俯角时 +Y/−Z 端点屏幕方向接近，避免
    // 循环顺序造成的点击歧义）
    int    best    = -1;
    double best_d  = 9.0;
    for (const auto& e : ends) {
        const double d = QLineF(wp, c + e.dir * R).length();
        if (d <= best_d) { best_d = d; best = e.id; }
    }
    if (best >= 0) return best;
    if (QLineF(wp, c).length() <= R + 9.0) return -2; // 球内其它区 = 拖拽 orbit
    return -1;
}

void SceneCanvasWidget::applyStandardView(int axis_end) {
    // 端点编号 → 标准视角（只改 yaw/pitch，注视点 (tx,tz) 与 view3d_zoom_
    // 保持——预设视角不破坏用户浏览状态）。【命名=属性面板语义】Z=高度
    // （"Z 位置"）、Y=画布 y——与轴球标签/属性面板同名；相机内部仍记
    // 高度为 Y（Camera3D 世界系），此处只换文案不改数学。
    switch (axis_end) {
    case 0:  // +X 右视（相机在画布 +x 侧，向 −x 看）
        cam3d_.yaw_deg = 90.0;  cam3d_.pitch_deg = Camera3D::kPitchMin;
        statusTip("右视图（轴球 +X）· 双击空白复位");
        break;
    case 1:  // −X 左视
        cam3d_.yaw_deg = 270.0; cam3d_.pitch_deg = Camera3D::kPitchMin;
        statusTip("左视图（轴球 −X）· 双击空白复位");
        break;
    case 2:  // +Z 顶视（沿高度轴正俯视画布——与 2D 回退视图同向无镜像；
             // 高度轴即属性面板"Z 位置"）
        cam3d_.yaw_deg = 0.0;   cam3d_.pitch_deg = Camera3D::kPitchMax;
        statusTip("顶视图（轴球 +Z）· 双击空白复位");
        break;
    case 3:  // +Y 正视（贴地平视画布正面——画布 y 轴方向）
        cam3d_.yaw_deg = 0.0;   cam3d_.pitch_deg = Camera3D::kPitchMin;
        statusTip("正视图（轴球 +Y）· 双击空白复位");
        break;
    case 4:  // −Y 背视
        cam3d_.yaw_deg = 180.0; cam3d_.pitch_deg = Camera3D::kPitchMin;
        statusTip("背视图（轴球 −Y）· 双击空白复位");
        break;
    default:
        return;
    }
    update();
}

// ---------------------------------------------------------------------------
// 时间轴适配器注入（仅一次）：建立画布编辑→适配器的自发自收闭环——
// sceneEdited/sceneEditedLive 的发射者与消费者（adapter_/preview_）都是
// 本控件已持有的对象，宿主只负责注入、不中转（2026-11-16 自 MainWindow
// 接线收编）。lambda 调适配器方法需完整类型，故实现落 .cpp。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::setTimelineAdapter(QtTimelineModelAdapter* a) {
    adapter_ = a;
    hist_ = a->undoHistory(); // 场景撤销历史随适配器取（适配器值成员+执行点）
    // 编辑落定（拖拽松手/银幕编辑/键盘微调）：菱形镜像重灌（画布 auto-key
    // 只写工作副本，时间轴 SceneClipModel::tweenChannels 须重灌否则看不到
    // 菱形）+ 预览失效 + 节流下发汇入 + 属性面板联动灌值。
    connect(this, &SceneCanvasWidget::sceneEdited, this, [this] {
        if (adapter_) adapter_->syncAllClipTweenDots();
        if (preview_) preview_->invalidate();
        if (adapter_) adapter_->requestAutoPublish();
        notifySelectedClipPanels(); // 【2026-11-18】补齐面板广播腿
    });
    // live 编辑（拖拽会话 mouse-move 高频）：只做节流决策（O(1)）——预览
    // 失效不由本信号承担（会话中画布内部 update + invalidate 已自理）。
    // 【2026-11-18】+ 属性面板实时联动（拖拽中面板数值跟手——refreshFrom
    // 为非焦点控件灌值，QSignalBlocker 防回环；注册表空/非 clip 选中零开销）。
    connect(this, &SceneCanvasWidget::sceneEditedLive, this, [this] {
        if (adapter_) adapter_->requestAutoPublish();
        notifySelectedClipPanels();
    });
    // 时间轴模型侧编辑（拖拽/面板写回/导入落表——工作副本已变）→ 预览
    // 失效：画布显示的就是工作副本，下一拍拉帧即反映改动（下发汇入已由
    // 适配器构造内自连 requestAutoPublish，此处不重复）。
    connect(adapter_, &QtTimelineModelAdapter::sceneEdited, this, [this] {
        if (preview_) preview_->invalidate();
    });
    // 场景撤销/恢复落定（2026-11-26）：画布收尾统一挂本信号（执行点在
    // 适配器 sceneUndo/sceneRedo，入口 = 画布挂的快捷键）——清选中（恢复
    // 后可能越界/失效）、预览失效、重绘。
    connect(adapter_, &QtTimelineModelAdapter::sceneRestored, this, [this] {
        highlight(0, -1);          // 清画布选中（恢复后可能越界/失效）
        if (preview_) preview_->invalidate();
        refresh();
    });
}

// ---------------------------------------------------------------------------
// execSceneUndoRedo —— 场景撤销/重做快捷键落点（2026-11-26）：执行在适配器
// sceneUndo/sceneRedo（loadSceneToModel 全量重灌唯一恢复通道 + 菱形镜像 +
// 面板广播 + display 防抖追平，恢复抑制在适配器内闭环）。画布收尾统一经
// sceneRestored 信号（setTimelineAdapter 内 connect）。失败（空栈/会话中/
// 适配器未注入）静默。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::execSceneUndoRedo(bool undo) {
    if (!adapter_) return;
    undo ? adapter_->sceneUndo() : adapter_->sceneRedo();
}

// ---------------------------------------------------------------------------
// notifySelectedClipPanels —— 画布编辑 → 属性面板广播（2026-11-18 补腿）：
// 画布拖拽/键盘微调/auto-key 写的都是工作副本，但面板只有"面板自身写回"
// 与"时间轴编辑"两条触发链会刷新——画布这条腿在 2026-11-16 接线收编进
// setTimelineAdapter 时丢失（原 EditorWindow 接线在 refreshSceneViews 汇聚
// 点与画布 sceneEdited 两处调用 refreshClipPanels）。现由 sceneEdited/
// sceneEditedLive 两信号共同调用：拖拽会话中实时跟手 + 松手终值落定。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::notifySelectedClipPanels() {
    if (!adapter_ || !scene_ || sel_.kind != 4) return;
    if (sel_.a < 0 || sel_.a >= (int)scene_->timeline.tracks.size()) return;
    const auto& clips = scene_->timeline.tracks[sel_.a].clips;
    if (sel_.b < 0 || sel_.b >= (int)clips.size()) return;
    adapter_->refreshClipPanels(clips[sel_.b].id);
}

// ---------------------------------------------------------------------------
// 时间轴选中联动（2026-11-16 自宿主接线收编）：发射源=时间轴控件共享选中
// 模型、换算=适配器 workClipIndices、接收端=本画布 highlight——三方中画布
// 持有 adapter_，时间轴经本注入补齐（仅取其 selectionModel 建立连接，
// 不存成员；receiver=this，画布销毁连接自动断）。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::setTimelineWidget(BaseTimelineWidget* tw) {
    if (!tw || !tw->sharedSelectionModel()) return;
    connect(tw->sharedSelectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex& cur, const QModelIndex&) {
        if (!adapter_) return;
        int tr = -1, cl = -1;
        // current 离开片段（清空/轨道行/无效）→ 画布选中同步作废
        //（【2026-11-20 单片段选中卡死修复】配对时间轴端 mousePress
        // 显式清 current——否则画布/属性面板残留旧片段选中）
        if (!adapter_->workClipIndices(cur, &tr, &cl)) {
            highlight(0, -1);
            return;
        }
        highlight(4, tr, cl);
    });
}

// ---------------------------------------------------------------------------
// 显示端在线状态推送（EditorWindow::onRefresh 200ms 驱动）：整表替换 +
// 重绘——screen 标签旁的在线圆点随实时连接状态变化（连接测试联动）
// ---------------------------------------------------------------------------
void SceneCanvasWidget::setDisplayOnlineStates(
        const std::map<std::string, bool>& states) {
    // 状态真变化才重绘（onRefresh 高频调用；离线退避期间快照值恒定）
    if (states == display_online_) return;
    display_online_ = states;
    update();
}

void SceneCanvasWidget::setDisplayConnect(int screen_idx, bool connect_) {
    if (!runtime_ || !scene_ || screen_idx < 0 ||
        screen_idx >= static_cast<int>(scene_->screens.size()))
        return;
    runtime_->setDisplayEnabled(
        QString::fromStdString(scene_->screens[screen_idx].player_id),
        connect_);
}

void SceneCanvasWidget::statusTip(const QString& text) {
    // 画布操作提示静默（2026-11-20 用户要求取消 qDebug 打印）——
    // 调用点保留作提示汇聚点，未来接状态栏时可一处启用
    Q_UNUSED(text);
}

void SceneCanvasWidget::refreshViews() {
    if (preview_) preview_->invalidate();
    update();
    // 内容已变 → display 节流下发（sceneEditedLive 走适配器 requestAutoPublish 节流链）
    emit sceneEditedLive();
}

void SceneCanvasWidget::publishScene() {
    // 复合操作（兜底同步 → 音频快照 → 广播）已封装进适配器（模型对外接口）
    if (!adapter_ || !adapter_->publishScene()) {
        statusTip("下发场景：下发运行时未启动，忽略");
        return;
    }
    statusTip("场景已全量下发到显示端");
}

void SceneCanvasWidget::syncMedia() {
    if (!scene_ || !adapter_) return;
    const auto res = adapter_->syncMedia();
    if (!res.ok) {
        statusTip("同步媒体：下发运行时未启动（端口占用？）——素材无法分发");
        return;
    }
    statusTip(QStringLiteral("同步媒体：注册 %1 个素材 / 在线显示端 %2 个%3")
                  .arg(static_cast<int>(res.registered))
                  .arg(static_cast<int>(res.online))
                  .arg(res.online == 0
                           ? QStringLiteral("——清单未送达（显示端上线后请再次"
                                            "同步）")
                           : QStringLiteral("")));
}

void SceneCanvasWidget::syncPropDialog(const std::string& sid) {
    // 【2026-11-25 双向联动·画布→弹窗】该屏的属性框开着 → 场景当前几何+
    // 校色刷进控件。拖动/缩放/旋转/gizmo 会话每拍调用；syncFrom 内部
    // QSignalBlocker 防回环（不触发 geometry_live_ 再写回场景）。框未开
    //（一对一映射查无此项）/ 屏已删：静默跳过。
    if (!scene_) return;
    auto it = prop_dlgs_.find(sid);
    if (it == prop_dlgs_.end()) return;
    for (const auto& s : scene_->screens)
        if (s.id == sid) {
            it->second->syncFrom(s);
            return;
        }
}

void SceneCanvasWidget::erasePropDialogEntry(ScreenPropertyDialog* dlg) {
    // 属性弹窗 destroyed 槽共用体：按 dlg 值精确清除映射项——任一回收路径
    //（OK/取消/X）最终都走 deleteLater→destroyed；map 精确匹配互不误伤
    // 其他框（2026-11-25 一对一映射；openScreenProperty/addScreen 两处
    // 注册的同一逻辑收敛到唯一实现，防漂移）。
    for (auto it = prop_dlgs_.begin(); it != prop_dlgs_.end(); ++it)
        if (it->second == dlg) { prop_dlgs_.erase(it); break; }
}

void SceneCanvasWidget::activatePropDialog(ScreenPropertyDialog* dlg) {
    if (!dlg) return;
    dlg->raise();
    dlg->activateWindow();
}

QString SceneCanvasWidget::displayInfoLine(const std::string& player_id) const {
    // display 名称行富文本（「通用」页顶部只读行）：display 注册帧无独立
    // 名称——名称即绑定 player_id；状态按运行时登记快照。颜色对齐画布
    // 在线圆点语义：在线绿 / 离线灰 / 未登记橙 / 运行时未启动暗灰。
    const QString pid = QString::fromStdString(player_id);
    if (pid.isEmpty())
        return QStringLiteral("<span style='color:#888'>未绑定显示端"
                              "</span>");
    if (!runtime_ || !runtime_->running())
        return QStringLiteral("<b>%1</b> · <span style='color:#666'>"
                              "运行时未启动</span>").arg(pid);
    const auto rows = runtime_->displayRows();
    for (const auto& r : rows) {
        if (r.id != pid) continue;
        const QString addr = QStringLiteral("%1:%2").arg(r.host).arg(r.port);
        if (r.connected)
            return QStringLiteral("<b>%1</b> · <span style='color:#2EAD5A'>"
                                  "在线</span> · %2").arg(pid, addr);
        return QStringLiteral("<b>%1</b> · <span style='color:#999'>"
                              "离线</span> · %2").arg(pid, addr);
    }
    return QStringLiteral("<b>%1</b> · <span style='color:#C77B3C'>"
                          "未登记</span>（按绑定页 host:port 填写后 OK 登记）")
        .arg(pid);
}

void SceneCanvasWidget::openScreenProperty(int screen_idx) {
    if (!scene_ || screen_idx < 0 ||
        screen_idx >= static_cast<int>(scene_->screens.size()))
        return;
    Screen& scr = scene_->screens[screen_idx];
    // 【2026-11-25 用户定案：一对一】该屏已开属性框 → 提到前台不重复弹；
    // 其他屏的框不受影响（各 display 独立弹窗，取消还原/OK 写回各按自己
    // 的 sid 与备份，互不竞争）。
    if (auto it = prop_dlgs_.find(scr.id); it != prop_dlgs_.end()) {
        activatePropDialog(it->second);
        return;
    }
    // 当前显示端登记（属性对话框绑定区预填；未登记给本机默认地址）
    std::string cur_host   = "127.0.0.1";
    std::uint16_t cur_port = 17100;
    std::string cur_pairing;
    if (runtime_) {
        const auto rows = runtime_->displayRows();
        for (const auto& r : rows) {
            if (r.id.toStdString() == scr.player_id) {
                cur_host    = r.host.toStdString();
                cur_port    = static_cast<std::uint16_t>(r.port);
                cur_pairing = r.pairingCode.toStdString();
                break;
            }
        }
    }
    const std::string old_pid = scr.player_id;
    // warp 取消还原基准（实时预览：编辑中会先写工作副本）
    const ScreenWarp warp_backup = scr.warp;
    // 几何取消还原基准（同 warp 语义——取消时还原十一字段：几何八字段
    // 含屏姿态 + 输出校色四字段 2026-11-20）。std::array 便于值捕获进
    // 非模态结果槽（C 数组不可捕获）。
    const std::array<double, 11> geo_backup = {
        scr.x, scr.y, scr.w, scr.h, scr.rotation,
        scr.rot_x, scr.rot_y,
        scr.rgb_gain[0], scr.rgb_gain[1], scr.rgb_gain[2], scr.gamma};

    // 登记执行体（OK 对账用；2026-11-17 绑定简化为纯确认制）：prev_pid
    // 非空且 ≠ 新 pid 时注销旧条目（改绑清理）；同 id 登记三要素任一变化
    //（含配对码）触发重登记（先 remove 再 add，看门狗按新地址重连）；
    // 无变化不动（避免播放中连接闪断）。
    const auto applyBinding = [this](const std::string& prev_pid,
                                     const std::string& pid,
                                     const std::string& host,
                                     std::uint16_t    port,
                                     const std::string& pairing) {
        if (!runtime_ || !runtime_->running()) return;
        if (!prev_pid.empty() && prev_pid != pid)
            runtime_->removeDisplay(
                QString::fromStdString(prev_pid)); // 幂等：未登记忽略
        if (pid.empty()) return;
        const auto rows = runtime_->displayRows();
        for (const auto& r : rows) {
            if (r.id.toStdString() == pid) {
                if (r.host.toStdString() == host &&
                    r.port == static_cast<int>(port) &&
                    r.pairingCode.toStdString() == pairing)
                    return; // 无变化：不动（防闪断）
                break;
            }
        }
        runtime_->removeDisplay(QString::fromStdString(pid)); // 幂等
        runtime_->addDisplay(QString::fromStdString(pid),
                             QString::fromStdString(host),
                             static_cast<int>(port),
                             QString::fromStdString(pairing));
    };

    // 【2026-11-25 非模态化（用户报"属性弹窗影响画布操作"）】exec() 应用级
    // 模态阻塞画布 → show() 非模态：编辑属性期间画布可继续选中/拖动/缩放。
    // 结果语义不变：accepted = 原 OK 分支（写回 + 绑定对账 + 自动下发）；
    // rejected = 取消还原（warp + 几何实时预览中间态回滚）。生命周期：
    // 两槽末尾 deleteLater()（点按钮只 hide 不关对象）；X 关闭走
    // QDialog::closeEvent→rejected 同样回收；映射项经 destroyed 按值清除。
    const std::string sid = scr.id;
    auto* dlg = new ScreenPropertyDialog(scr, cur_host, cur_port, cur_pairing,
                                         this);
    dlg->setWindowModality(Qt::NonModal);
    // 标题 = display 名称行（setDisplayInfo 内部 setDialogTitle；
    // 2026-11-25 用户定案：标题栏直接展示绑定 id + 在线状态 + 地址）
    // 【实时预览·变形】编辑中每次变化 → 写工作副本 + 视图刷新
    dlg->setWarpLiveApply([this, sid](const ScreenWarp& w) {
        for (auto& s : scene_->screens)
            if (s.id == sid) { s.warp = w; break; }
        refreshViews();
    });
    // 【实时预览·几何+校色】同链——编辑器画布即时可见（含屏姿态
    // rot_x/rot_y——2026-11-17 阶段2，display 透视取景窗实时预览；
    // 输出校色 rgb_gain/gamma——2026-11-20 Display 级四字段）
    dlg->setGeometryLiveApply([this, sid](const Screen& g) {
        for (auto& s : scene_->screens)
            if (s.id == sid) {
                s.x = g.x; s.y = g.y; s.w = g.w; s.h = g.h;
                s.rotation = g.rotation;
                // s.z 不拷贝（2026-11-18 层级行退役——对话框不再提供 z）
                s.rot_x = g.rot_x; s.rot_y = g.rot_y;
                for (int i = 0; i < 3; ++i) s.rgb_gain[i] = g.rgb_gain[i];
                s.gamma = g.gamma;
                break;
            }
        refreshViews();
    });
    connect(dlg, &QObject::destroyed, this, [this, dlg] {
        erasePropDialogEntry(dlg); // 任一回收路径统一出映射（防悬垂）
    });

    // ---- OK：写回 + 绑定对账 + 收尾（按 sid 重查屏——对话框打开期间
    //      屏数组可能变化，不持有打开时的引用）-----------------------------
    connect(dlg, &QDialog::accepted, this,
            [this, sid, old_pid, cur_host, cur_port, cur_pairing,
             applyBinding, dlg] {
                auto it = std::find_if(
                    scene_->screens.begin(), scene_->screens.end(),
                    [&](const Screen& s) { return s.id == sid; });
                if (it == scene_->screens.end()) { // 屏已删：放弃写回
                    dlg->deleteLater();
                    return;
                }
                Screen& scr2 = *it;
                dlg->applyTo(scr2);

                const std::string     new_pid     = scr2.player_id;
                const std::string     new_host    = dlg->displayHost();
                const std::uint16_t   new_port    = dlg->displayPort();
                const std::string     new_pairing = dlg->displayPairingCode();
                const bool bind_changed =
                    old_pid != new_pid || cur_host != new_host ||
                    cur_port != new_port || cur_pairing != new_pairing;
                if (runtime_ && runtime_->running())
                    applyBinding(old_pid, new_pid, new_host, new_port,
                                 new_pairing);
                refreshViews();
                if (bind_changed && runtime_ && runtime_->running()) {
                    // 【绑定收尾（2026-11-17）】绑定有变 → 自动下发场景（无
                    // 需确认）：已连入的 display 立即收到新场景；尚未连入的
                    // display 在注册时由 EditorRuntime::handleRegister 自动
                    // 补发。手动路径仍保留（画布银幕右键「下发场景」）。
                    publishScene();
                    statusTip(QStringLiteral("屏幕 %1 已修改（绑定 %2@%3:%4）；"
                                             "场景已自动下发")
                                  .arg(QString::fromStdString(scr2.id),
                                       QString::fromStdString(new_pid),
                                       QString::fromStdString(new_host))
                                  .arg(new_port));
                } else {
                    statusTip(QStringLiteral(
                                  "屏幕 %1 已修改（绑定 %2@%3:%4；运行时未运行，"
                                  "启动后经画布银幕右键「下发场景」生效）")
                                  .arg(QString::fromStdString(scr2.id),
                                       QString::fromStdString(new_pid),
                                       QString::fromStdString(new_host))
                                  .arg(new_port));
                }
                dlg->deleteLater(); // 点 OK 只 hide——显式回收（配合 destroyed）
                // 撤销会话收口（OK：本框全部实时编辑收编为一条；注意画布
                // 拖拽会中途 beginEdit 顶掉本会话——彼时已先行收口）
                if (hist_) hist_->endEdit(*scene_);
            });

    // ---- 取消：还原 warp + 几何实时预览中间态（绑定纯确认制不处理）------
    connect(dlg, &QDialog::rejected, this,
            [this, sid, warp_backup, geo_backup, dlg] {
                for (auto& s : scene_->screens) {
                    if (s.id != sid) continue;
                    const bool warp_changed =
                        s.warp.mode != warp_backup.mode ||
                        s.warp.cols != warp_backup.cols ||
                        s.warp.rows != warp_backup.rows ||
                        s.warp.pts  != warp_backup.pts;
                    const bool geo_changed =
                        s.x != geo_backup[0] || s.y != geo_backup[1] ||
                        s.w != geo_backup[2] || s.h != geo_backup[3] ||
                        s.rotation != geo_backup[4] ||
                        s.rot_x != geo_backup[5] || s.rot_y != geo_backup[6] ||
                        s.rgb_gain[0] != geo_backup[7] ||
                        s.rgb_gain[1] != geo_backup[8] ||
                        s.rgb_gain[2] != geo_backup[9] ||
                        s.gamma != geo_backup[10];
                    if (warp_changed) s.warp = warp_backup;
                    if (geo_changed) {
                        s.x = geo_backup[0]; s.y = geo_backup[1];
                        s.w = geo_backup[2]; s.h = geo_backup[3];
                        s.rotation = geo_backup[4];
                        s.rot_x = geo_backup[5]; s.rot_y = geo_backup[6];
                        for (int i = 0; i < 3; ++i)
                            s.rgb_gain[i] = geo_backup[7 + i];
                        s.gamma = geo_backup[10];
                    }
                    if (warp_changed || geo_changed) refreshViews();
                    break;
                }
                prop_dlgs_.erase(sid); // 取消即出映射（同屏可立即重开）
                dlg->deleteLater();
                // 撤销会话收口（取消：还原后与开启基线一致 → 去重丢弃）
                if (hist_) hist_->endEdit(*scene_);
            });

    prop_dlgs_[sid] = dlg;
    // 【2026-11-25 display 名称行】通用页顶部展示绑定 id + 在线状态
    //（打开期间状态变化由 ui_timer_ 拍重设）
    dlg->setDisplayInfo(displayInfoLine(scr.player_id));
    // 撤销会话开启（打开即冻结 before 基线——本框实时编辑/取消还原/
    // OK 写回共用一条历史；非模态期间画布拖拽经 beginEdit 先行顶掉）
    if (hist_) hist_->beginEdit("屏幕属性", *scene_);
    dlg->show(); // 非模态：编辑期间画布可继续操作（2026-11-25 用户需求）
}

void SceneCanvasWidget::addScreen() {
    if (!scene_) return;
    Screen scr;
    // id 自动化：scr-N 线性探测（防删屏后重名；对齐原版命名）
    int n = static_cast<int>(scene_->screens.size()) + 1;
    auto idClash = [this](const std::string& v) {
        for (const auto& s : scene_->screens)
            if (s.id == v) return true;
        return false;
    };
    scr.id = "scr-" + std::to_string(n);
    while (idClash(scr.id)) scr.id = "scr-" + std::to_string(++n);
    // 默认几何：960×540，阶梯排布（避免完全重叠）
    scr.x = 60.0 * ((n - 1) % 4) + 20.0;
    scr.y = 60.0 * (((n - 1) / 4) % 4) + 20.0;
    scr.w = 960;
    scr.h = 540;
    // 显示端绑定默认值：player-N（编号与屏幕对齐）@ 本机 17100——仅供
    // 对话框预填，用户可直接改成远端 IP/端口/配对码
    scr.player_id = "player-" + std::to_string(n);

    // 【2026-11-23 交互改版（用户定案）】添加即弹属性对话框：预填默认值，
    // OK 才落地 + 按所填参数登记显示端；取消零副作用。取代旧「先添加
    // 127.0.0.1 默认屏再开属性改绑」两步流（旧流登记/日志恒写本机地址，
    // 远端接入隐蔽且误导）。新建模式不注入 warp/几何实时回调：屏尚未
    // 在场景中（回调按 id 查工作副本查不到 = 空操作），纯表单 OK 一次写回。
    // 【2026-11-25 非模态化】同属性框：show() 非模态 + accepted/rejected
    // 槽收尾（OK 才 push_back 落地；scr 按值捕获进槽——落地前无引用
    // 悬垂问题）。【一对一映射】添加流用空 key：已开添加框 → raise 不
    // 重复弹；编辑框（其他 key）不受影响、保持开着。
    if (auto it = prop_dlgs_.find(std::string{}); it != prop_dlgs_.end()) {
        activatePropDialog(it->second);
        return;
    }
    auto* dlg = new ScreenPropertyDialog(scr, "127.0.0.1", 17100, {}, this);
    dlg->setWindowModality(Qt::NonModal);
    // 标题 = display 名称行（同编辑流；未登记态自带提示文案）
    connect(dlg, &QObject::destroyed, this, [this, dlg] {
        erasePropDialogEntry(dlg); // 任一回收路径统一出映射（防悬垂）
    });
    connect(dlg, &QDialog::accepted, this, [this, scr, dlg] {
        Screen landed = scr; // 对话框表单值写进本地副本再落地
        dlg->applyTo(landed);
        const std::string     host    = dlg->displayHost();
        const std::uint16_t   port    = dlg->displayPort();
        const std::string     pairing = dlg->displayPairingCode();

        // 显示端登记：按对话框三要素（幂等——同名沿用；冲突返回 false 静默，
        // 与旧 addScreen 同口径；改绑/断开走银幕右键与属性对话框）
        if (runtime_ && runtime_->running() && !landed.player_id.empty())
            runtime_->addDisplay(QString::fromStdString(landed.player_id),
                                 QString::fromStdString(host),
                                 static_cast<int>(port),
                                 QString::fromStdString(pairing));

        scene_->screens.push_back(landed);
        if (preview_) preview_->invalidate();
        update();
        highlight(1, static_cast<int>(scene_->screens.size()) - 1);
        // 撤销收编（离散：新屏落地一条——绑定登记属运行时态不入快照）
        if (hist_) hist_->checkpoint("添加屏幕", *scene_);
        // 提示走 qDebug（对接 spdlog 日志存留）——如实打印所填绑定地址
        //（旧文案恒写 127.0.0.1，与实际登记脱节）。
        qDebug() << QStringLiteral("屏幕 %1 已加入（显示端 %2 @%3:%4）——"
                                   "显示端程序以 --id=%2 启动后自动接入；"
                                   "%5x%6@%7,%8")
                        .arg(QString::fromStdString(landed.id),
                             QString::fromStdString(landed.player_id),
                             QString::fromStdString(host))
                        .arg(port)
                        .arg(landed.w)
                        .arg(landed.h)
                        .arg(landed.x, 0, 'f', 0)
                        .arg(landed.y, 0, 'f', 0);
        dlg->deleteLater(); // 点 OK 只 hide——显式回收
    });
    connect(dlg, &QDialog::rejected, this, [this, dlg] {
        prop_dlgs_.erase(std::string{}); // 取消即出映射（可立即再点添加）
        dlg->deleteLater(); // 取消 = 不添加（scr 尚未入列，零副作用）
    });
    prop_dlgs_[std::string{}] = dlg; // 空 key = 添加流（同 key 防重）
    // 【2026-11-25 display 名称行】新建屏默认绑定 player-N：大概率未登记
    //（提示按绑定页填写）
    dlg->setDisplayInfo(displayInfoLine(scr.player_id));
    dlg->show();
}

void SceneCanvasWidget::deleteScreen(int screen_idx) {
    if (!scene_ || screen_idx < 0 ||
        screen_idx >= static_cast<int>(scene_->screens.size()))
        return;
    const std::string id = scene_->screens[screen_idx].id;
    scene_->screens.erase(scene_->screens.begin() + screen_idx);
    // 【非模态联动（2026-11-25）】删掉的屏正是属性框编辑对象 → 关框
    //（继续留着只会产出无效 OK/取消——按 sid 重查已落空）
    if (auto it = prop_dlgs_.find(id); it != prop_dlgs_.end())
        it->second->close(); // rejected → 还原(无效果) + 出映射 + deleteLater
    if (preview_) preview_->invalidate();
    update();
    if (hist_) hist_->checkpoint("删除屏幕", *scene_); // 撤销收编（离散）
    qDebug() << QStringLiteral("ms-canvas: 删除屏幕 %1")
                .arg(QString::fromStdString(id));
}

// ---------------------------------------------------------------------------
// 坐标换算：画布 → 控件（fit × 用户缩放 + 居中 + 平移，保持纵横比）。
// 【去虚拟画布化】fit 不再留外边距——整个预览窗口即画布，铺满客户区
// （纵横比不符时的窄边黑条与画布底色同色，视觉上不可见）
// ---------------------------------------------------------------------------
double SceneCanvasWidget::fitScale() const {
    if (!scene_ || scene_->canvas_w <= 0 || scene_->canvas_h <= 0)
        return 1.0;
    if (width() <= 0 || height() <= 0) return 1.0;
    return std::min((double)width() / scene_->canvas_w,
                    (double)height() / scene_->canvas_h);
}

QRectF SceneCanvasWidget::canvasRect() const {
    if (!scene_ || scene_->canvas_w <= 0 || scene_->canvas_h <= 0)
        return rect(); // 无场景：整个控件视作画布区域（画空壳提示）
    // 实际比例 = fit 基准 × 用户缩放；平移叠加在 fit 居中位置之上。
    // 缩放/平移只改这一个矩形——所有换算（canvasToWidget/widgetToCanvas）、
    // 命中（hitTest）、绘制（paintEvent/各 draw*）、合成尺寸（拉帧泵
    // setTargetSize）全部自动跟随，无需逐处适配。
    const double k = fitScale() * view_zoom_;
    return QRectF((width() - scene_->canvas_w * k) / 2.0 + view_pan_.x(),
                  (height() - scene_->canvas_h * k) / 2.0 + view_pan_.y(),
                  scene_->canvas_w * k, scene_->canvas_h * k);
}

QPointF SceneCanvasWidget::canvasToWidget(const QPointF& p) const {
    const QRectF cr = canvasRect();
    if (!scene_ || scene_->canvas_w <= 0 || scene_->canvas_h <= 0) return p;
    const double k = cr.width() / scene_->canvas_w;
    return QPointF(cr.left() + p.x() * k, cr.top() + p.y() * k);
}

QPointF SceneCanvasWidget::widgetToCanvas(const QPointF& p) const {
    const QRectF cr = canvasRect();
    if (!scene_ || scene_->canvas_w <= 0 || scene_->canvas_h <= 0 ||
        cr.width() <= 0)
        return QPointF(0, 0);
    const double k = scene_->canvas_w / cr.width();
    return QPointF((p.x() - cr.left()) * k, (p.y() - cr.top()) * k);
}

// ---------------------------------------------------------------------------
// 3D 换算辅助（画布 3D 化 2026-11-05）
//
// canvasToWidgetAt / widgetToCanvasAt：高度平面感知的双向换算——
//   * 2D 模式直接分发到 fit 映射（h 不参与，与历史行为逐位一致）；
//   * 3D 模式经 Camera3D 正交投影/反投影（h = 对象所在高度平面）。
// 交互复用的关键：拖动/缩放/旋转的几何数学活在画布域（CanvasGeometry
// 纯函数），3D 下只需把"鼠标点 → 画布点"换成"视线 ∩ 对象高度平面"，
// 下游 2D 数学零改动。
// ---------------------------------------------------------------------------
QPointF SceneCanvasWidget::canvasToWidgetAt(const QPointF& p, double h) const {
    if (!view3d_) return canvasToWidget(p);
    if (width() <= 0 || height() <= 0) return p;
    return cam3d_.project(p.x(), h, p.y(), width() / 2.0, height() / 2.0);
}

QPointF SceneCanvasWidget::widgetToCanvasAt(const QPointF& wp, double h) const {
    if (!view3d_) return widgetToCanvas(wp);
    if (width() <= 0 || height() <= 0 || cam3d_.scale <= 0) return QPointF(0, 0);
    double a = 0, b = 0;
    cam3d_.planeRayCoeffs(wp.x(), wp.y(), width() / 2.0, height() / 2.0, h,
                          &a, &b);
    return QPointF(cam3d_.tx + a, cam3d_.tz + b);
}

QPointF SceneCanvasWidget::canvasFromScreen(const QPointF& wp) const {
    // 拖动会话统一入口：反投影平面 = 按下时冻结的 drag_plane_h_
    // （被拖对象所在高度平面——拖动全程对象"贴"在光标下的体验来源）
    return widgetToCanvasAt(wp, view3d_ ? drag_plane_h_ : 0.0);
}

QPointF SceneCanvasWidget::project3(double x, double h, double z) const {
    if (width() <= 0 || height() <= 0) return QPointF(x, z);
    return cam3d_.project(x, h, z, width() / 2.0, height() / 2.0);
}

void SceneCanvasWidget::panAnchorTo(const QPointF& widget_pos,
                                    const QPointF& world) {
    // 平移锚定闭式解：planeRayCoeffs 的 A/B 与注视点无关 → 令
    // tx' + A(widget) = world.x 即得新注视点（光标下世界点跟随光标）
    if (width() <= 0 || height() <= 0) return;
    double a = 0, b = 0;
    cam3d_.planeRayCoeffs(widget_pos.x(), widget_pos.y(), width() / 2.0,
                          height() / 2.0, 0.0, &a, &b);
    cam3d_.tx = world.x() - a;
    cam3d_.tz = world.y() - b;
}

double SceneCanvasWidget::selectionPlaneH() const {
    if (!view3d_) return 0.0;
    if (sel_.kind == 4 && scene_ && sel_.a >= 0 &&
        sel_.a < (int)scene_->timeline.tracks.size() && sel_.b >= 0 &&
        sel_.b < (int)scene_->timeline.tracks[sel_.a].clips.size())
        return (double)scene_->timeline.tracks[sel_.a]
                   .clips[sel_.b].transform.pos_z; // 片段悬浮高度 = 深度 Z
                                                   // （3D 空间真实 z 位置；
                                                   //  层级 transform.z 非
                                                   //  空间坐标）
    return 0.0; // 屏幕/无选中：地面（屏幕是物理输出平面，恒贴地）
}

bool SceneCanvasWidget::vpHandleHit(const QPointF& wp) const {
    // VP 手柄仅在手动 Eye 已配置时存在（persp_focal<=0 时 VP 走画布
    // 派生默认——拖动无意义；配置入口 = 右键菜单「全局透视（Eye 点）」）
    if (!view3d_ || !scene_ || scene_->persp_focal <= 0.0) return false;
    const QPointF vp =
        project3(scene_->persp_vp_x, 0.0, scene_->persp_vp_y);
    return (vp - wp).manhattanLength() <= 10.0;
}

bool SceneCanvasWidget::eyeHandleHit(const QPointF& wp) const {
    // Eye 环与 VP 十字同一全局透视参数：点 Eye = 选中 VP（拖动归 VP 自由拖）
    if (!view3d_ || !scene_ || scene_->persp_focal <= 0.0) return false;
    const QPointF eye = project3(scene_->persp_vp_x, scene_->persp_focal,
                                 scene_->persp_vp_y);
    const double er = 6.0 + std::min(6.0, scene_->persp_focal / 20000.0);
    return (eye - wp).manhattanLength() <= er + 5.0;
}

void SceneCanvasWidget::openEyeDialog() {
    if (!scene_) return;
    QDialog dlg(this);
    dlg.setWindowTitle(tr("全局透视（Eye 点）"));
    auto* lay  = new QVBoxLayout(&dlg);
    auto* form = new QFormLayout;
    // 焦距 F：0 = 未配置（画布派生默认 F=2×对角线、VP=画布中心——
    // Scene::effectivePersp 语义）；>0 = 手动 Eye 生效
    auto* focal = new FloatDragValueWidget(&dlg);
    focal->setRange(0.0, 200000.0);
    focal->setDecimals(0);
    focal->setSingleStep(100.0);
    focal->setValue(scene_->persp_focal);
    focal->setToolTip(tr(
        "全局透视焦距 F（px）——观察眼点 Eye 在消隐点正上方 F 高度处。\n"
        "0 = 未配置——全场透视走画布派生默认\n"
        "（F = 2×画布对角线，消隐点 = 画布中心）；\n>0 = 手动 Eye 生效，"
        "3D 视口出现可拖动的 VP 标记与 Eye 圆环。"));
    auto* vpx = new FloatDragValueWidget(&dlg);
    vpx->setRange(-8192.0, 16384.0);
    vpx->setDecimals(1);
    vpx->setSingleStep(10.0);
    vpx->setValue(scene_->persp_vp_x);
    auto* vpy = new FloatDragValueWidget(&dlg);
    vpy->setRange(-8192.0, 16384.0);
    vpy->setDecimals(1);
    vpy->setSingleStep(10.0);
    vpy->setValue(scene_->persp_vp_y);
    form->addRow(tr("焦距 F："), focal);
    form->addRow(tr("消隐点 X："), vpx);
    form->addRow(tr("消隐点 Y："), vpy);
    lay->addLayout(form);
    // 【2026-11-18 用户反馈"改 Eye 无效果"根治】F=0（未配置）时 VP 根本
    // 不参与投影计算（effectivePersp 走画布派生默认）——VP 字段灰化 +
    // 动态提示当前模式，消除"改了没反应"的困惑。
    auto* mode_hint = new QLabel(&dlg);
    mode_hint->setWordWrap(true);
    mode_hint->setStyleSheet("color: #C8781C; font-size: 11px;");
    lay->addWidget(mode_hint);
    auto update_vp = [this, focal, vpx, vpy, mode_hint] {
        const bool on = focal->value() > 0.0;
        vpx->setEnabled(on);
        vpy->setEnabled(on);
        mode_hint->setText(
            on ? tr("手动 Eye 已生效：眼点悬浮于消隐点 VP 正上方高度 F=%1 px 处。"
                     "修改 F / VP 立即影响成品合成画面（素材升向 Eye 放大、远"
                     "离缩小）与 3D 视口 Eye 圆环。")
                   .arg(focal->value(), 0, 'f', 0)
               : tr("F=0 = 未配置：全场透视走画布派生默认（F=2×画布对角线，"
                     "消隐点 = 画布中心）——此时改 VP 无效果，请先把焦距 F "
                     "设为大于 0 的值。"));
    };
    update_vp();
    connect(focal, &FloatDragValueWidget::valueChanged, &dlg, update_vp);
    auto* hint = new QLabel(
        tr("消隐点也可在 3D 视口直接拖动橙色 VP 标记（需先配置焦距），"
           "Eye 圆环随焦距升降。透视只影响素材投影，不改 Display 取景窗。"), &dlg);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: gray; font-size: 11px;");
    lay->addWidget(hint);
    auto* reset = new QPushButton(tr("恢复默认（画布派生）"), &dlg);
    connect(reset, &QPushButton::clicked, &dlg, [focal, update_vp] {
        focal->setValue(0.0); // F=0 = 未配置 → effectivePersp 画布派生
        update_vp();
    });
    lay->addWidget(reset);
    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    lay->addWidget(btns);
    if (dlg.exec() != QDialog::Accepted) return;
    scene_->persp_focal = focal->value();
    scene_->persp_vp_x  = vpx->value();
    scene_->persp_vp_y  = vpy->value();
    if (preview_) preview_->invalidate(); // 透视参数变更重合成
    emit sceneEdited();
    update();
}

// ---------------------------------------------------------------------------
// 移动 Gizmo（2026-11-18，Blender 式三轴箭头）--------------------------------
// 用户反馈"位移很难操作"：自由拖在 3D 视口里要脑补高度平面，单轴精调更
// 是只能去属性面板输数字。Blender 的 Move Gizmo 把三轴可视化在对象锚点
// 上（红 X / 绿 Y / 蓝 Z=高度，与轴球/地面三轴线/属性面板同色同标签），
// 点轴=单轴约束拖动、拖中心区域=自由拖（走既有路径）。
// 轴向量 = 相机正交仿射映射下"世界 1 单位"的屏幕投影（按下冻结——拖动
// 全程视图不变），屏幕恒长轴杆不随缩放/距离变化（Blender 同风格）。
// ---------------------------------------------------------------------------

bool SceneCanvasWidget::moveGizmoAnchor(double& cx, double& h,
                                        double& cy) const {
    // 选中对象名义矩形中心（与 selectionCanvasRect/选中框同源——不受
    // 旋转/角位移影响，锚点稳定）；高度 = 片段 pos_z（屏幕恒贴地 h=0）
    if (!scene_ || sel_.kind == 0) return false;
    if (sel_.kind == 1 && sel_.a >= 0 &&
        sel_.a < (int)scene_->screens.size()) {
        const Screen& s = scene_->screens[sel_.a];
        cx = s.x + s.w * 0.5;
        cy = s.y + s.h * 0.5;
        h  = 0.0; // 屏幕是物理输出平面，恒贴地
        return true;
    }
    if (sel_.kind == 4 && sel_.a >= 0 &&
        sel_.a < (int)scene_->timeline.tracks.size() && sel_.b >= 0 &&
        sel_.b < (int)scene_->timeline.tracks[sel_.a].clips.size()) {
        // 【2026-11-20 交互优化】Gizmo 放视觉正中：quadCorners3DP 透视四
        // 角均值（滚转/俯仰/偏航/锚点/角位移/深度/全场透视全生效——透视
        // 四边形上四角均值与对角线交点高度接近，视觉"正中"足够；无姿态
        // 无位移时逐位等于名义矩形中心，历史行为零漂移）
        const Clip& c =
            scene_->timeline.tracks[sel_.a].clips[sel_.b];
        const Transform rt = clipResolvedTransform(c);
        const QRectF cr = clipCanvasRect(c); // 含 auto-size 兜底宽高
        double ef = 0.0, evx = 0.0, evy = 0.0;
        scene_->effectivePersp(ef, evx, evy);
        double qx[4], qy[4];
        cg::quadCorners3DP(cr.x(), cr.y(), cr.width(), cr.height(),
                           rt.pos_z, rt.rotation, rt.rot_x, rt.rot_y,
                           rt.anchor, rt.cornerActive() ? rt.corner : nullptr,
                           ef, evx, evy, qx, qy, nullptr);
        cx = (qx[0] + qx[1] + qx[2] + qx[3]) * 0.25;
        cy = (qy[0] + qy[1] + qy[2] + qy[3]) * 0.25;
        h  = rt.pos_z;
        return true;
    }
    return false;
}

QPointF SceneCanvasWidget::moveGizmoAxisVec(int axis, double cx, double h,
                                            double cy) const {
    // 轴"世界 1 单位"的屏幕投影向量；轴向 0=+画布X（红） 1=+画布Y（绿）
    // 2=+高度（蓝）——与地面三轴线/轴球绿球蓝球同标签同色系
    const double dx = (axis == 0) ? 1.0 : 0.0;
    const double dh = (axis == 2) ? 1.0 : 0.0;
    const double dz = (axis == 1) ? 1.0 : 0.0; // 画布 y = 世界 Z 轴向
    return project3(cx + dx, h + dh, cy + dz) - project3(cx, h, cy);
}

int SceneCanvasWidget::hitMoveGizmo(const QPointF& wp) const {
    if (!view3d_) return -1;
    double cx = 0, h = 0, cy = 0;
    if (!moveGizmoAnchor(cx, h, cy)) return -1;
    // 2D 化（2026-11-20）：仅 X/Y 双轴——蓝 Z 高度轴退役（pos_z 恒 0，
    // 屏幕本就贴地无高度轴；轴 2 命中通道保留但不可达）
    constexpr int max_axis = 1;
    return hitGizmoAxes(wp, cx, h, cy, max_axis, -10.0);
}

int SceneCanvasWidget::hitGizmoAxes(const QPointF& wp, double cx, double h,
                                    double cy, int max_axis,
                                    double min_t) const {
    // 轴命中：沿轴距离 ∈ [min_t, L+16] 且垂距 ≤ 10px（轴杆+锥一体宽容
    // 域，中心圈归自由拖不参与）；投影退化（|v|<1e-3 轴正对相机）不可
    // 选——Blender 同规则
    const QPointF o = project3(cx, h, cy);
    for (int axis = 0; axis <= max_axis; ++axis) {
        const QPointF v = moveGizmoAxisVec(axis, cx, h, cy);
        const double len = std::hypot(v.x(), v.y());
        // 退化判定宁低勿误杀：低倍率下 1 画布 px 的投影 ~0.5px 也完全
        // 可归一化（方向精确）——只拦近正对相机的趋零轴（2026-11-18
        // 首次实测：阈值 0.5 恰好卡在 scale=0.61×cos35°≈0.50 上，蓝
        // Z 高度轴被误跳过）
        if (len < 1e-3) continue;
        const QPointF n = v / len;
        const QPointF rel = wp - o;
        const double t = QPointF::dotProduct(rel, n);
        const double perp =
            std::hypot(rel.x() - n.x() * t, rel.y() - n.y() * t);
        if (t >= min_t && t <= 74.0 && perp <= 10.0) return axis;
    }
    return -1;
}

void SceneCanvasWidget::drawMoveGizmo(QPainter& p) const {
    // 三轴箭头（屏幕恒长 58px；hover/拖动轴高亮加粗）。恒最上层绘制
    //（HUD 前——选中对象的位移手柄优先可见，不做深度遮挡）。
    // 2D 化（2026-11-20）：仅画 X/Y 双轴（蓝 Z 同 hitMoveGizmo 退役）
    double cx = 0, h = 0, cy = 0;
    if (!moveGizmoAnchor(cx, h, cy)) return;
    constexpr int max_axis = 1;
    drawGizmoAxes(p, cx, h, cy, max_axis, gizmo_drag_axis_,
                  gizmo_hover_axis_);
}

void SceneCanvasWidget::drawGizmoAxes(QPainter& p, double cx, double h,
                                      double cy, int max_axis, int drag_axis,
                                      int hover_axis) const {
    const QPointF o = project3(cx, h, cy);
    struct AxisDef {
        int         id;
        QColor      base;
        QColor      hot;
        const char* label;
    };
    static const AxisDef kAxes[] = {
        {0, QColor(0xE0, 0x48, 0x48), QColor(0xFF, 0x74, 0x74), "X"},
        {1, QColor(0x52, 0xA0, 0x5E), QColor(0x74, 0xD4, 0x7C), "Y"},
        {2, QColor(0x55, 0x78, 0xC0), QColor(0x88, 0xAC, 0xFF), "Z"},
    };
    QFont f = p.font();
    f.setPointSize(7);
    f.setBold(true);
    constexpr double L = 58.0; // 轴长（屏幕 px，恒定）
    for (const auto& ax : kAxes) {
        if (ax.id > max_axis) continue; // 屏幕恒贴地无高度轴（VP 仅 X/Y）
        const QPointF v = moveGizmoAxisVec(ax.id, cx, h, cy);
        const double len = std::hypot(v.x(), v.y());
        if (len < 1e-3) continue; // 近正对相机（投影趋零）才退化——阈值
                                  // 宁低勿误杀（同 hitGizmoAxes 注释）
        const QPointF n = v / len; // 屏幕单位方向
        const bool   hot = ax.id == drag_axis || ax.id == hover_axis;
        const QColor   col = hot ? ax.hot : ax.base;
        const QPointF  tip = o + n * L;
        const QPointF  pv(-n.y(), n.x()); // 垂直方向（锥底宽）
        p.setPen(QPen(col, hot ? 3.2 : 2.2, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(col);
        p.drawLine(o + n * 9.0, tip - n * 12.0); // 轴杆（离心让位中心圈）
        // 锥形箭头（Blender 同款实心三角；沿 n 长 14、底宽 11）
        QPolygonF cone;
        cone << (tip + n * 2.0)
             << (tip - n * 12.0 + pv * 5.5)
             << (tip - n * 12.0 - pv * 5.5);
        p.drawPolygon(cone);
        p.setBrush(Qt::NoBrush);
        p.setFont(f);
        p.setPen(QPen(col, 1));
        p.drawText(tip + n * 9.0 + QPointF(-4.0, 4.0),
                   QLatin1String(ax.label));
    }
    // 中心锚（白圈 + 深底实点——Blender object origin 风格；点它附近
    // = 走既有自由拖路径）
    p.setPen(QPen(QColor(0xF2, 0xF2, 0xF2), 1.6));
    p.setBrush(QColor(0x30, 0x30, 0x30, 200));
    p.drawEllipse(o, 5.5, 5.5);
    p.setBrush(QColor(0xF2, 0xF2, 0xF2));
    p.drawEllipse(o, 1.6, 1.6);
    p.setBrush(Qt::NoBrush);
}

// ---------------------------------------------------------------------------
// VP Gizmo（2026-11-18）：用户反馈"VP 的移动没有这个 Gizmo"——对象位移
// 有三轴箭头，VP 拖动却只有一个小十字，方向感缺失。VP 恒贴地（画布域
// 透视参数），只画红 X/绿 Y 双轴；F>0 且 3D 视口恒显（VP 全局唯一，不
// 与选中对象 Gizmo 混淆）。压轴杆 = 单轴约束会话，压中心圈 = 自由拖。
// ---------------------------------------------------------------------------

int SceneCanvasWidget::hitVpGizmo(const QPointF& wp) const {
    // 选中 VP 后才可命中（与其他对象 Gizmo 同规则）；蓝 Z = Eye 高度 F
    if (!view3d_ || !scene_ || scene_->persp_focal <= 0.0 || !vp_selected_)
        return -1;
    // min_t=9：中心圈半径外才算轴命中——VP 中心 10px 归 vpHandleHit 自由拖
    return hitGizmoAxes(wp, scene_->persp_vp_x, 0.0, scene_->persp_vp_y,
                        2, 9.0);
}

void SceneCanvasWidget::drawVpGizmo(QPainter& p) const {
    if (!view3d_ || !scene_ || scene_->persp_focal <= 0.0 || !vp_selected_)
        return;
    drawGizmoAxes(p, scene_->persp_vp_x, 0.0, scene_->persp_vp_y, 2,
                  vp_drag_axis_, vp_hover_axis_);
}

QPolygonF SceneCanvasWidget::projectQuad(const QRectF& r, double rotation_deg,
                                         double h, int anchor,
                                         const double* corner) const {
    // 画布域矩形四角 → 3D 投影四边形。正交投影保持平行性 → 结果恒为平
    // 行四边形（纹理仿射贴图的数学前提；corner 位移破坏平行性 → 变形
    // 路径由绘制端改走两三角仿射分块）。
    QPolygonF poly(4);
    if (anchor == 0 && corner == nullptr) {
        // 历史路径（绕中心旋转）：与旧实现逐位一致（screen / 无变形 clip）
        for (int i = 0; i < 4; ++i) {
            double cx = 0, cy = 0;
            cg::rotatedCorner(r.x(), r.y(), r.width(), r.height(), rotation_deg, i,
                              &cx, &cy);
            poly[i] = project3(cx, h, cy);
        }
        return poly;
    }
    // 变形路径：绕锚点旋转 + 角点位移（Transform 语义，与 core 端同数学）
    QPointF cs[4];
    clipQuadCorners(r, rotation_deg, anchor, corner, cs);
    for (int i = 0; i < 4; ++i) poly[i] = project3(cs[i].x(), h, cs[i].y());
    return poly;
}

void SceneCanvasWidget::clipQuadCorners(const QRectF& r, double rotation_deg,
                                        int anchor, const double* corner,
                                        QPointF out[4]) const {
    // 片段四角（画布域）——数学已收敛 cg::quadCornersWithAnchor（唯一
    // 真相，2026-11-09 X 形根治）：角序 CornerIdx 顺时针、锚点旋转、
    // 位移沿画布轴，与 core ScreenMapper / GlRenderer 同源。本函数仅
    // 做类型适配（QRectF/QPointF ↔ double），调用点零改动。
    double ox[4], oy[4];
    cg::quadCornersWithAnchor(r.x(), r.y(), r.width(), r.height(),
                              rotation_deg, anchor, corner, ox, oy);
    for (int i = 0; i < 4; ++i) out[i] = QPointF(ox[i], oy[i]);
}

// ---------------------------------------------------------------------------
// screen / clip 的控件矩形
// ---------------------------------------------------------------------------
QRectF SceneCanvasWidget::screenRect(int i) const {
    if (!scene_ || i < 0 || i >= (int)scene_->screens.size() ||
        scene_->canvas_w <= 0 || scene_->canvas_h <= 0)
        return {};
    const Screen& s = scene_->screens[i];
    const double k = canvasRect().width() / scene_->canvas_w;
    const QPointF tl = canvasToWidget(QPointF(s.x, s.y));
    return QRectF(tl, QSizeF(s.w * k, s.h * k));
}

Transform SceneCanvasWidget::clipResolvedTransform(const Clip& c) const {
    // 【2026-11-05 画布跟随 tween】有 tween 通道的属性由曲线"整条接管"
    // （TweenEngine 语义），画布若直读静态 transform 会与预览合成输出
    // 分叉——暂停态改曲线画面不动（联测实证：pull item x 140.1→195.4
    // 而 texmap 恒 286）。此处统一求值：绘制/命中/吸附共用 → 显示与
    // 命中天然一致。时间源 = 预览播放头（暂停=冻结值；纯函数零状态，
    // 片段量级下成本可忽略——头文件同结论）。
    return (preview_ && !c.tweens.empty())
               ? TweenEngine::resolveTransform(c, preview_->playheadUs())
               : c.transform;
}

bool SceneCanvasWidget::clipActiveAtPlayhead(const Clip& c) const {
    // 【只显当前 2026-11-11 用户定案】预览画布只显示播放头时刻活跃的
    // 素材（WATCHOUT Stage 语义）：不活跃片段连"方框+棋盘格"占位也不
    // 画——旧模式下它是 matchTex 恒空（交付项只含活跃片段）时的唯一
    // 视觉，用户体感"没显示的素材也堆满画布"。可见性口径与合成端
    // TimelineEngine [start, start+duration) 开区间一致。无预览运行时
    // （preview_ 空 = 纯静态查看）恒活跃——无时间上下文，全显示。
    if (!preview_) return true;
    const std::int64_t ph = preview_->playheadUs();
    return c.start_us <= ph && ph < c.start_us + c.duration_us;
}

bool SceneCanvasWidget::tweenAutoKeyWrite(Clip& c, TweenProp prop,
                                          double v0, double v1, double v2) {
    // 播放头 → 片段局部时间（钳 [0, duration]；与 TweenSection
    // localPlayheadUs 同口径——出点外写键落端点，端点保持语义一致）
    const std::int64_t ph = preview_ ? preview_->playheadUs() : c.start_us;
    const std::int64_t t_local =
        std::clamp(ph - c.start_us, std::int64_t{0},
                   std::max(c.duration_us, std::int64_t{0}));
    // 键写入共用内核下沉 TweenEngine（与属性面板 auto-key 同一条路径，
    // 防两处语义漂移；无通道返回 false → 调用方走静态基准写入）。
        // v3 打包语义：Position 一键携带 v0/v1/v2 三分量（x/y/z——调用方传
    // 当前层级防拖拽把层级键成 0）
    return TweenEngine::autoKeyWrite(c, prop, t_local, v0, v1, v2);
}

QRectF SceneCanvasWidget::clipCanvasRect(const Clip& c) const {
    // 变换取统一入口（tween 通道在播放头时刻的求值结果；无通道=静态基准）
    const Transform tf = clipResolvedTransform(c);
    // 绘制宽高兜底：transform.w/h<=0 → 素材原始宽高 → 640x360
    // （与 EditorWindow 新建片段的 transform 初值约定一致）
    const Asset* a = scene_ ? scene_->findAsset(c.asset_id) : nullptr;
    const double w = tf.w > 0
                         ? tf.w
                         : (a && a->width > 0 ? (double)a->width : 640.0);
    const double h = tf.h > 0
                         ? tf.h
                         : (a && a->height > 0 ? (double)a->height : 360.0);
    return QRectF(tf.x, tf.y, w, h);
}

QRectF SceneCanvasWidget::clipRect(int track_idx, int clip_idx) const {
    if (!scene_ || scene_->canvas_w <= 0 || scene_->canvas_h <= 0 ||
        track_idx < 0 || track_idx >= (int)scene_->timeline.tracks.size() ||
        clip_idx < 0 ||
        clip_idx >= (int)scene_->timeline.tracks[track_idx].clips.size())
        return {};
    const Clip& c = scene_->timeline.tracks[track_idx].clips[clip_idx];
    const QRectF r = clipCanvasRect(c);
    const double k = canvasRect().width() / scene_->canvas_w;
    const QPointF tl = canvasToWidget(r.topLeft());
    return QRectF(tl, QSizeF(r.width() * k, r.height() * k));
}

// ---------------------------------------------------------------------------
// 命中测试【2026-11-15 选中分工定案】screen-only：画布只选 screen。
//
// 分工依据：画布"只显当前"（播放头时刻活跃素材的舞台快照，WATCHOUT
// Stage 语义）后，画布不是素材资源清单——在快照上做全量选择会让使用者
// 点到"此刻不存在"的对象而毫无反馈（"找不到"的根源）。片段是时间实体，
// 选中唯一入口 = 时间轴（永远找得到）；画布 = 空间域，只管恒存在恒显示
// 的 screen + 承担"时间轴已选中片段"的空间编辑（hitSelectedClip 特判，
// 见 mousePressEvent——时间轴选谁，画布摆哪）。
//
// 屏幕命中：多屏重叠取数组序靠后者（数组序 = 绘制序，覆盖关系一致）；
// 最近者（正交投影下"离相机近 = 视觉遮挡在上"）。
// 【重叠拣选三态 2026-11-25 两轮实锤定稿】kCycle=当前选中在命中集合内 →
// 轮转下一层（左键连点换选，Figma/PowerPoint 同款）；kSticky=命中集合内
// → 锁定当前选中（右键菜单专属——连点有意选中的下层屏，菜单必须服务它）；
// kTop=恒最上层（双击/缺省）。演化史：轮转无差别复用 → 右键菜单错层翻转
// player-1→player-2（真凶：菜单动作把轮转 hit 写回选中，"泄漏"不存在）；
// 恒最上层 → 连点选中的 player-2 右键又翻回 player-1。三态=两种场景各得
// 其所：菜单 hit 有状态（延续用户选中）但只延续不轮转。
// ---------------------------------------------------------------------------
SceneCanvasWidget::Selection SceneCanvasWidget::hitTest(
        const QPointF& widget_pos, HitPick pick) const {
    if (!scene_ || scene_->canvas_w <= 0 || scene_->canvas_h <= 0) return {};

    // 画布域判定（旋转发生在对象局部，与控件缩放无关；CanvasGeometry
    // 共享实现，与 core_tests 验证的数学逐位一致）
    const auto rectHit = [](const QRectF& r, double rot_deg,
                            const QPointF& p) -> bool {
        return cg::rotatedRectContains(r.x(), r.y(), r.width(), r.height(),
                                       rot_deg, p.x(), p.y());
    };

    if (view3d_) {
        // ---- 3D：屏幕贴地平面反投影 → 画布域包含 → 视深最近 ----
        // 【重叠循环选择】命中集合按视深升序（近→远，绘制覆盖序），
        // 当前选中在集合内 → 轮转下一个；否则取最近（=最上层）
        std::vector<std::pair<int, double>> hits; // (下标, 视深)
        for (int i = 0; i < (int)scene_->screens.size(); ++i) {
            const Screen& s = scene_->screens[i];
            if (s.w <= 0 || s.h <= 0) continue;
            const QPointF cp = widgetToCanvasAt(widget_pos, 0.0); // 屏幕贴地
            if (!rectHit(QRectF(s.x, s.y, s.w, s.h), s.rotation, cp)) continue;
            hits.emplace_back(
                i, cam3d_.depthOf(s.x + s.w / 2.0, 0.0, s.y + s.h / 2.0));
        }
        if (!hits.empty()) {
            std::sort(hits.begin(), hits.end(),
                      [](const auto& x, const auto& y) {
                          return x.second < y.second; // 视深升序=视觉前→后
                      });
            int chosen = hits.front().first; // 最近（最上层）
            if (pick != HitPick::kTop) {
                for (size_t k = 0; k < hits.size(); ++k) {
                    if (hits[k].first == sel_.a && sel_.kind == 1) {
                        // kCycle=轮转下一层；kSticky=锁定当前选中
                        chosen = (pick == HitPick::kCycle)
                                     ? hits[(k + 1) % hits.size()].first
                                     : sel_.a;
                        break;
                    }
                }
            }
            return Selection{1, chosen, -1};
        }
        return {};
    }

    const QPointF cp = widgetToCanvas(widget_pos);

    // ---- 2D 屏幕：重叠取数组序靠后（与绘制序一致）+ 重叠循环选择 ----
    // 姿态屏（俯仰/偏航非 0）：quadCorners3DP 透视压平四角 AABB（2026-11-20
    // 与素材统一走全局仪）——透视四边形可能非凸，AABB 简单可靠（与
    // clipRectContains 姿态分支同策略同源）
    std::vector<int> hits; // 命中集合（数组序 = 绘制覆盖序）
    // Show 全局透视仪（场景级常量——循环外求值一次）
    double hit_ef = 0.0, hit_evx = 0.0, hit_evy = 0.0;
    scene_->effectivePersp(hit_ef, hit_evx, hit_evy);
    for (int i = 0; i < (int)scene_->screens.size(); ++i) {
        const Screen& s = scene_->screens[i];
        bool hit;
        if (s.rot_x != 0.0 || s.rot_y != 0.0) {
            double wx[4], wy[4];
            cg::quadCorners3DP(s.x, s.y, s.w, s.h, 0.0, s.rotation,
                               s.rot_x, s.rot_y, 0, nullptr,
                               hit_ef, hit_evx, hit_evy, wx, wy, nullptr);
            double minx = wx[0], miny = wy[0], maxx = wx[0], maxy = wy[0];
            for (int k = 1; k < 4; ++k) {
                minx = std::min(minx, wx[k]);
                maxx = std::max(maxx, wx[k]);
                miny = std::min(miny, wy[k]);
                maxy = std::max(maxy, wy[k]);
            }
            hit = cp.x() >= minx && cp.x() <= maxx && cp.y() >= miny &&
                  cp.y() <= maxy;
        } else {
            hit = rectHit(QRectF(s.x, s.y, s.w, s.h), s.rotation, cp);
        }
        if (hit) hits.push_back(i); // 数组序累计（后命中者覆盖前者）
    }
    if (!hits.empty()) {
        int chosen = hits.back(); // 首点选最上层（旧习惯）
        if (pick != HitPick::kTop && sel_.kind == 1) {
            for (size_t k = 0; k < hits.size(); ++k) {
                if (hits[k] == sel_.a) { // 当前选中命中集合
                    // kCycle=轮转下一层；kSticky=锁定当前选中
                    chosen = (pick == HitPick::kCycle)
                                 ? hits[(k + 1) % hits.size()]
                                 : sel_.a;
                    break;
                }
            }
        }
        return Selection{1, chosen, -1};
    }

    return {};
}

// ---------------------------------------------------------------------------
// 已选中片段命中特判（2026-11-15 选中分工）：点击点落在当前选中 clip 的
// 变形感知矩形内 = 画布进"空间编辑"角色（拖动/角点缩放/旋转手柄全走
// 既有会话逻辑，不依赖 hitTest）。片段此刻不活跃（画布不可见）= 不可
// 编辑目标——"画布摆哪"的前提是画布看得见。
// 3D 模式：片段所在高度平面反投影后判定（与绘制/原 clip 命中同口径）。
// ---------------------------------------------------------------------------
bool SceneCanvasWidget::hitSelectedClip(const QPointF& widget_pos) const {
    if (sel_.kind != 4 || !scene_ || scene_->canvas_w <= 0 ||
        scene_->canvas_h <= 0)
        return false;
    if (sel_.a < 0 || sel_.a >= (int)scene_->timeline.tracks.size())
        return false;
    const auto& clips = scene_->timeline.tracks[sel_.a].clips;
    if (sel_.b < 0 || sel_.b >= (int)clips.size()) return false;
    const Clip& c = clips[sel_.b];
    if (!clipActiveAtPlayhead(c)) return false; // 不可见 = 不可编辑
    const QRectF r = clipCanvasRect(c);
    if (r.width() <= 0 || r.height() <= 0) return false;
    const QPointF cp =
        view3d_ ? widgetToCanvasAt(widget_pos, (double)c.transform.pos_z)
                : widgetToCanvas(widget_pos); // 3D：片段悬浮平面 = 深度 Z
                                              // （pos_z；层级非空间坐标）
    // 【素材真 3D 化 2026-11-18】透视参数统一走 Scene::effectivePersp
    //（未配置 = 画布派生默认：F=2×对角线、vp=画布中心）
    double ef = 0.0, evx = 0.0, evy = 0.0;
    scene_->effectivePersp(ef, evx, evy);
    return clipRectContains(r, clipResolvedTransform(c), cp,
                            ef, evx, evy);
}

// ---------------------------------------------------------------------------
// 角点缩放：选中对象几何 / 手柄命中
// ---------------------------------------------------------------------------
bool SceneCanvasWidget::selectionCanvasRect(QRectF* r,
                                            double* rotation_deg) const {
    if (!scene_) return false;
    if (sel_.kind == 1 && sel_.a >= 0 &&
        sel_.a < (int)scene_->screens.size()) {
        const Screen& s = scene_->screens[sel_.a];
        *r = QRectF(s.x, s.y, s.w, s.h);
        *rotation_deg = s.rotation;
        return s.w > 0 && s.h > 0;
    }
    if (sel_.kind == 4 && sel_.a >= 0 &&
        sel_.a < (int)scene_->timeline.tracks.size() && sel_.b >= 0 &&
        sel_.b < (int)scene_->timeline.tracks[sel_.a].clips.size()) {
        const Clip& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
        *r = clipCanvasRect(c); // 含 w/h<=0 兜底（auto-size 语义取有效框）
        *rotation_deg = clipResolvedTransform(c).rotation; // tween 接管语义
        return true;
    }
    return false;
}

int SceneCanvasWidget::hitResizeHandle(const QPointF& widget_pos) const {
    // 只对"已选中对象"生效：手柄语义 = 操作当前选中（与选中框同现同隐）
    // 【2026-11-20 交互优化】素材四角手柄改义为角点位移拖拽（hitClipCorner
    // Handle）——矩形角点缩放仅保留银幕（Screen 无角点位移语义）
    if (sel_.kind == 4) return -1; // 素材缩放柄退役恒 -1（角点/边线柄见
                                   // hitClipCornerHandle/hitClipEdgeHandle）
    QRectF r;
    double rot = 0.0;
    if (!selectionCanvasRect(&r, &rot)) return -1;
    if (r.width() <= 0 || r.height() <= 0) return -1;
    // 姿态屏（俯仰/偏航非 0）：柄随绘制同隐（透视四边形无矩形柄语义，
    // 与 drawScreen 姿态分支同现同隐；拖动平移不受影响）
    if (sel_.kind == 1) {
        const Screen& s = scene_->screens[sel_.a];
        if (s.rot_x != 0.0 || s.rot_y != 0.0) return -1;
    }
    // 容差用控件像素（视觉恒定，不随画布 fit 缩放变化）：手柄方块 6px，
    // 命中半径 5px 略大于半块，点击手感宽松
    const double tol = 5.0;
    const double h = selectionPlaneH(); // 3D：手柄在对象高度平面上投影
    for (int corner = 0; corner < 4; ++corner) {
        double cx = 0, cy = 0; // 角点（画布坐标，旋转后位置）
        cg::rotatedCorner(r.x(), r.y(), r.width(), r.height(), rot, corner,
                          &cx, &cy);
        const QPointF wp = canvasToWidgetAt(QPointF(cx, cy), h);
        const double dx = wp.x() - widget_pos.x();
        const double dy = wp.y() - widget_pos.y();
        if (dx * dx + dy * dy <= tol * tol) return corner;
    }
    return -1;
}

bool SceneCanvasWidget::hitRotateHandle(const QPointF& widget_pos) const {
    // 只对"已选中对象"生效（手柄与选中框同现同隐）
    QRectF r;
    double rot = 0.0;
    if (!selectionCanvasRect(&r, &rot)) return false;
    if (r.width() <= 0 || r.height() <= 0) return false;
    // 变形片段：旋转柄隐藏（变形四边形上柄位未定义——姿态编辑走属性面
    // 板；与 drawClip 手柄分支同判据同现同隐）
    if (sel_.kind == 4) {
        const Clip& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
        const Transform tf = clipResolvedTransform(c);
        if (tf.anchor != 0 || tf.cornerActive() || tf.pose3DActive())
            return false;
    }
    // 姿态屏：旋转柄同缩放柄一起隐藏（见 hitResizeHandle）
    if (sel_.kind == 1) {
        const Screen& s = scene_->screens[sel_.a];
        if (s.rot_x != 0.0 || s.rot_y != 0.0) return false;
    }
    // 柄外延量用控件像素（视觉恒定）→ 换算成画布单位传给几何函数
    const double k = view3d_ ? cam3d_.scale
                             : (canvasRect().width() /
                                (scene_ && scene_->canvas_w > 0
                                     ? scene_->canvas_w : 1.0));
    const double offset_canvas = (k > 0 ? 22.0 / k : 22.0); // 柄心离顶边 22px
    double hx = 0, hy = 0;
    cg::rotateHandlePos(r.x(), r.y(), r.width(), r.height(), rot,
                        offset_canvas, &hx, &hy);
    const QPointF wp = canvasToWidgetAt(QPointF(hx, hy), selectionPlaneH());
    const double dx = wp.x() - widget_pos.x();
    const double dy = wp.y() - widget_pos.y();
    return dx * dx + dy * dy <= 6.0 * 6.0; // 命中半径 6px（柄半径 4px+余量）
}

// ---------------------------------------------------------------------------
// 素材画布手柄（2026-11-20 交互优化）----------------------------------------
// 选中片段：四角 = 角点位移拖拽（corner pin，写 Transform.corner）、四边 =
// 尺寸缩放（写 w/h，对边视觉固定）；全程经 sceneEditedLive → 属性面板实
// 时灌值。视觉四角 = quadCorners3DP 透视投影（与绘制/合成同源唯一真相
// ——2026-11-20 修订恢复透视），数学全部活在画布域，控件域只做换算与命
// 中容差。
// ---------------------------------------------------------------------------

QPolygonF SceneCanvasWidget::clipVisualQuadCanvas(const Clip& c,
                                                  const Transform& tf) const {
    const QRectF cr = clipCanvasRect(c); // 画布域（含 auto-size 兜底宽高）
    double ef = 0.0, evx = 0.0, evy = 0.0;
    scene_->effectivePersp(ef, evx, evy);
    double qx[4], qy[4];
    cg::quadCorners3DP(cr.x(), cr.y(), cr.width(), cr.height(),
                       tf.pos_z, tf.rotation, tf.rot_x, tf.rot_y, tf.anchor,
                       tf.cornerActive() ? tf.corner : nullptr,
                       ef, evx, evy, qx, qy, nullptr);
    QPolygonF quad;
    quad.reserve(4);
    for (int i = 0; i < 4; ++i) quad << QPointF(qx[i], qy[i]);
    return quad;
}

int SceneCanvasWidget::hitClipCornerHandle(const QPointF& widget_pos) const {
    if (sel_.kind != 4 || !scene_ || scene_->canvas_w <= 0 ||
        scene_->canvas_h <= 0 || sel_.a < 0 ||
        sel_.a >= (int)scene_->timeline.tracks.size() || sel_.b < 0 ||
        sel_.b >= (int)scene_->timeline.tracks[sel_.a].clips.size())
        return -1;
    const Clip& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
    if (!clipActiveAtPlayhead(c)) return -1; // 不可见 = 不可编辑（同 hitSelectedClip）
    const QPolygonF quad = clipVisualQuadCanvas(c, clipResolvedTransform(c));
    constexpr double tol = 8.0; // 控件像素（视觉恒定；角点方块 8px 手感宽松）
    for (int k = 0; k < 4; ++k) {
        const QPointF wp = canvasToWidgetAt(quad[k], selectionPlaneH());
        const double dx = wp.x() - widget_pos.x();
        const double dy = wp.y() - widget_pos.y();
        if (dx * dx + dy * dy <= tol * tol) return k; // CornerIdx 0..3
    }
    return -1;
}

int SceneCanvasWidget::hitClipEdgeHandle(const QPointF& widget_pos) const {
    if (sel_.kind != 4 || !scene_ || scene_->canvas_w <= 0 ||
        scene_->canvas_h <= 0 || sel_.a < 0 ||
        sel_.a >= (int)scene_->timeline.tracks.size() || sel_.b < 0 ||
        sel_.b >= (int)scene_->timeline.tracks[sel_.a].clips.size())
        return -1;
    const Clip& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
    if (!clipActiveAtPlayhead(c)) return -1;
    const QPolygonF quad = clipVisualQuadCanvas(c, clipResolvedTransform(c));
    // 角点让位区：距任一角 < 14px 时边线不响应（press 先查角点，此处
    // 防其外圈"再瞄准"带误抓边线）
    constexpr double kCornerKeep = 14.0;
    for (int k = 0; k < 4; ++k) {
        const QPointF wp = canvasToWidgetAt(quad[k], selectionPlaneH());
        const double dx = wp.x() - widget_pos.x();
        const double dy = wp.y() - widget_pos.y();
        if (dx * dx + dy * dy < kCornerKeep * kCornerKeep) return -1;
    }
    // 边 e = 角 e → 角 (e+1)%4（0=上 1=右 2=下 3=左）；点到线段距离
    constexpr double tol = 6.0; // 控件像素（边线两侧各 6px 可抓）
    for (int e = 0; e < 4; ++e) {
        const QPointF p1 = canvasToWidgetAt(quad[e], selectionPlaneH());
        const QPointF p2 =
            canvasToWidgetAt(quad[(e + 1) & 3], selectionPlaneH());
        const QPointF v = p2 - p1;
        const double len2 = v.x() * v.x() + v.y() * v.y();
        double t = len2 > 1e-9
                       ? QPointF::dotProduct(widget_pos - p1, v) / len2
                       : 0.0;
        t = std::clamp(t, 0.0, 1.0);
        const QPointF proj = p1 + v * t;
        const double dx = proj.x() - widget_pos.x();
        const double dy = proj.y() - widget_pos.y();
        if (dx * dx + dy * dy <= tol * tol) return e;
    }
    return -1;
}

void SceneCanvasWidget::drawClipHandles(QPainter& p,
                                        const QPolygonF& quad_widget) const {
    if (quad_widget.size() != 4) return;
    // 四边线（实线=可拖感知；hover/拖动高亮蓝加粗）
    p.setBrush(Qt::NoBrush);
    for (int e = 0; e < 4; ++e) {
        const bool active =
            edge_drag_ == e || (edge_drag_ < 0 && edge_hover_ == e);
        p.setPen(QPen(active ? QColor(0x4F, 0xC3, 0xF7)
                             : QColor(0xFF, 0x33, 0x33),
                     active ? 2.6 : 1.8));
        p.drawLine(quad_widget[e], quad_widget[(e + 1) & 3]);
    }
    // 四角方块（白底黑描边，WATCHOUT 风格；hover/拖动高亮蓝）
    constexpr double hs = 4.0; // 半边长（8px 方块）
    for (int k = 0; k < 4; ++k) {
        const bool active =
            corner_drag_ == k || (corner_drag_ < 0 && corner_hover_ == k);
        p.setPen(QPen(active ? QColor(0x4F, 0xC3, 0xF7)
                             : QColor(0x20, 0x20, 0x20),
                      1.4));
        p.setBrush(active ? QColor(0x4F, 0xC3, 0xF7)
                          : QColor(0xFF, 0xFF, 0xFF));
        p.drawRect(QRectF(quad_widget[k].x() - hs, quad_widget[k].y() - hs,
                          hs * 2.0, hs * 2.0));
    }
}

void SceneCanvasWidget::tickClipCornerDrag(const QPointF& canvas_pos) {
    if (sel_.kind != 4 || !scene_ || corner_drag_ < 0 || sel_.a < 0 ||
        sel_.a >= (int)scene_->timeline.tracks.size() || sel_.b < 0 ||
        sel_.b >= (int)scene_->timeline.tracks[sel_.a].clips.size())
        return;
    auto& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
    const Transform tf = clipResolvedTransform(c);
    // 基准角 = 无角位移透视四角（本会话只动被拖角——x/y/w/h/姿态冻结，
    // 基准恒定；corner8 传空防自身反馈污染基准）
    const QRectF cr = clipCanvasRect(c);
    double ef = 0.0, evx = 0.0, evy = 0.0;
    scene_->effectivePersp(ef, evx, evy);
    double bx[4], by[4];
    cg::quadCorners3DP(cr.x(), cr.y(), cr.width(), cr.height(),
                       tf.pos_z, tf.rotation, tf.rot_x, tf.rot_y, tf.anchor,
                       nullptr, ef, evx, evy, bx, by, nullptr);
    const int k = corner_drag_;
    double dx = canvas_pos.x() - bx[k];
    double dy = canvas_pos.y() - by[k];
    // 角点吸附（2026-11-26）：被拖角落点（基准角+位移）对参考线吸附，
    // 命中修正位移——「角对边/角对角」贴合（品红方块，与移动会话同款
    // 视觉）。snapPoint 自包含重算 guide；开关关闭仅清残留显示。
    if (snapping_) {
        double tx = bx[k] + dx, ty = by[k] + dy;
        snapPoint(&tx, &ty);
        dx = tx - bx[k];
        dy = ty - by[k];
    } else {
        guide_x_.clear();
        guide_y_.clear();
        guide_pt_valid_ = false;
    }
    // auto-key 双模：角点位移通道接管即打包打键（v0=dx v1=dy）；无通道
    // 写静态基准 corner8（其他角偏移天然保留）
    if (!tweenAutoKeyWrite(c, (TweenProp)((int)TweenProp::CornerLT + k),
                           dx, dy, 0.0)) {
        c.transform.corner[k * 2 + 0] = dx;
        c.transform.corner[k * 2 + 1] = dy;
    }
    moved_ = true;
    if (preview_) preview_->invalidate();
    emit sceneEditedLive(); // 面板「形变」行实时灌值 + display 节流下发
    update();
}

void SceneCanvasWidget::tickClipEdgeDrag(const QPointF& canvas_pos) {
    if (sel_.kind != 4 || !scene_ || edge_drag_ < 0 || sel_.a < 0 ||
        sel_.a >= (int)scene_->timeline.tracks.size() || sel_.b < 0 ||
        sel_.b >= (int)scene_->timeline.tracks[sel_.a].clips.size())
        return;
    auto& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
    // 鼠标相对按下点的画布增量 → 沿被拖边方向轴投影（edge_dir 按下冻结
    // ——画布域局部轴单位向量）
    const QPointF d = canvas_pos - drag_start_canvas_;
    const double dotx = QPointF::dotProduct(d, edge_dir_x_);
    const double doty = QPointF::dotProduct(d, edge_dir_y_);
    double dw = 0.0, dh = 0.0;
    switch (edge_drag_) {
        case 0: dh = -doty; break; // 上边：向下拖 = 压扁
        case 1: dw = +dotx; break; // 右边：向外拖 = 加宽
        case 2: dh = +doty; break; // 下边：向下拖 = 拉高
        default: dw = -dotx; break; // 左边：向外拖 = 加宽
    }
    // 最小 8 画布单位：显式 resize 恒写正值（w/h≤0 会触发 auto-size 回退）
    const double nw = (std::max)(edge_orig_w_ + dw, 8.0);
    const double nh = (std::max)(edge_orig_h_ + dh, 8.0);
    // 新尺寸重解四角（冻结姿态/锚点，角偏移保持会话起始值不变）→ 对边
    // 中点一次回贴 x/y：四角像点随 x/y 平移严格等量平移（透视 s 只依赖
    // 局部坐标，平移不改局部）→ 中点差向量一次回贴精确锚定（对边视觉
    // 固定，任意姿态/透视下成立）
    const Transform tf = clipResolvedTransform(c); // corner 冻结（会话不写）
    double ef = 0.0, evx = 0.0, evy = 0.0;
    scene_->effectivePersp(ef, evx, evy);
    double qx[4], qy[4];
    cg::quadCorners3DP(edge_orig_x_, edge_orig_y_, nw, nh,
                       tf.pos_z, edge_orig_rot_, edge_orig_rx_,
                       edge_orig_ry_, edge_orig_anchor_,
                       tf.cornerActive() ? tf.corner : nullptr,
                       ef, evx, evy, qx, qy, nullptr);
    int i1 = 0, i2 = 1; // 对边两角（CornerIdx）
    switch (edge_drag_) {
        case 0: i1 = 3; i2 = 2; break; // 拖上→对边=下（LB,RB）
        case 1: i1 = 0; i2 = 3; break; // 拖右→对边=左（LT,LB）
        case 2: i1 = 0; i2 = 1; break; // 拖下→对边=上（LT,RT）
        default: i1 = 1; i2 = 2; break; // 拖左→对边=右（RT,RB）
    }
    const double mx = (qx[i1] + qx[i2]) * 0.5;
    const double my = (qy[i1] + qy[i2]) * 0.5;
    const double fx = edge_orig_x_ + (edge_opp_mid_.x() - mx);
    const double fy = edge_orig_y_ + (edge_opp_mid_.y() - my);
    // 写回：Size + Position 双 auto-key（tween 双模：通道接管即打包打键）
    if (!tweenAutoKeyWrite(c, TweenProp::Size, nw, nh)) {
        c.transform.w = nw;
        c.transform.h = nh;
    }
    const double cur_z = clipResolvedTransform(c).z;
    if (!tweenAutoKeyWrite(c, TweenProp::Position, fx, fy, cur_z)) {
        c.transform.x = fx;
        c.transform.y = fy;
    }
    moved_ = true;
    if (preview_) preview_->invalidate();
    emit sceneEditedLive(); // 面板尺寸/位置行实时灌值 + display 节流下发
    update();
}

// ---------------------------------------------------------------------------
// 绘制主过程（QOpenGLWidget::paintGL——Qt 提供上下文与 widget 尺寸 FBO）
//
// 【GL 直通（2026-11-04 预览管线 GL 化）】双层结构：
//   1. 合成层——GpuDeliver 模式且有新逐项交付时，先以裸 GL 调
//      GlRenderer(ExternalCtx) 合成（GPU shader alpha 混合 + 内容域
//      视口子矩形贴图）；背景清屏黑由 GL clear 完成。
//   2. 标注层——QPainter p(this)（Qt 官方"原生 GL + QPainter"混用模式：
//      QPainter 在已有 GL 内容之上自绘，互不干扰），负责 screens/clips
//      边框标签、选中框、吸附线；SoftCompose 模式/GL 失败回退/无渲染器
//      时它同时承担合成（黑底 + pull 帧贴图），与历史 paintEvent 等价。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::paintGL() {
    // 【诊断 MS_CANVAS_DBG=1】paintGL 耗时（RAII：任意出口均打点）——
    // 定位"播放期 UI 线程 250ms 一拍"的阻塞源：GL 合成/纹理上传是否
    // 占用 200ms 级（echo/pull 两只 33ms 定时器同时被饿的共同嫌疑）。
    struct PaintCostProbe {
        const std::chrono::steady_clock::time_point t0;
        const bool                                  on;
        PaintCostProbe()
            : t0(std::chrono::steady_clock::now()),
              on(std::getenv("MS_CANVAS_DBG") != nullptr) {}
        ~PaintCostProbe() {
            if (!on) return;
            static int s_n = 0;
            if (++s_n <= 600 && s_n % 10 == 0) {
                const auto us = std::chrono::duration_cast<
                    std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - t0).count();
                std::fprintf(stderr, "canvas-dbg: paintGL cost=%lldus\n",
                             (long long)us);
                std::fflush(stderr);
            }
        }
    } paint_probe;

    // ---- 0. 3D 视口路径（2026-11-05 画布 3D 化）--------------------------
    // 完全独立于 GL 直通链路：QPainter 正交投影渲染（网格地面 + 片段
    // 分层悬浮 + 标注），不建 GlRenderer、不占合成 shader。贴图数据仍取
    // GpuDeliver 逐项交付（QImage 源帧——CUDA native 经协商旗标回退下载）。
    // MS_CANVAS_3D=0 时整段跳过，走历史 2D 路径（下方原样保留）。
    if (view3d_) {
        std::vector<PreviewRenderer::DeliverItem> items;
        int   w3 = 0, h3 = 0;
        QRectF dom3;
        if (preview_ && scene_ &&
            preview_->deliverMode() ==
                PreviewRenderer::DeliverMode::GpuDeliver) {
            preview_->pullGpu(items, w3, h3, dom3); // 失败=无贴图（棋盘格兜底）
        }
        paintGL3D(items, w3, h3, dom3);
        return;
    }

    // ---- 1. 合成层（GL 直通；失败/无帧时由 QPainter 回退）----------------
    bool composed = false;
    std::vector<PreviewRenderer::DeliverItem> gpu_items;
    int   gpu_w = 0, gpu_h = 0;
    QRectF gpu_dom;
    if (preview_ && scene_ &&
        preview_->deliverMode() ==
            PreviewRenderer::DeliverMode::GpuDeliver) {
        if (preview_->pullGpu(gpu_items, gpu_w, gpu_h, gpu_dom)) {
            composed = composeGpuFrame(gpu_items, gpu_w, gpu_h, gpu_dom);
        }
    }
    // 【诊断 MS_CANVAS_DBG=1】paintGL 状态流（限频 30 拍一次）
    {
        static const bool s_dbg = [] {
            const bool on = std::getenv("MS_CANVAS_DBG") != nullptr;
            return on;
        }();
        if (s_dbg) {
            static int s_cnt = 0;
            if (++s_cnt <= 30) {  // 前 30 帧逐帧打印（点击选中只触发少数 repaint，30 拍限频会全吞）
                int nn = 0;
                for (const auto& it : gpu_items)
                    if (!it.frame.isNull() || it.dxt) ++nn;
                std::fprintf(stderr,
                             "canvas-dbg: paint#%d pull=%d composed=%d "
                             "items=%zu withFrame=%d comp=%dx%d dom=(%.0f,%.0f "
                             "%+.0fx%+.0f)\n",
                             s_cnt, preview_ ? 1 : 0, (int)composed,
                             gpu_items.size(), nn, gpu_w, gpu_h,
                             gpu_dom.x(), gpu_dom.y(), gpu_dom.width(),
                             gpu_dom.height());
                std::fflush(stderr);
            }
        }
    }

    // ---- 2. 标注层（兼软路径合成；GL 已合成时只画标注）-------------------
    // 【诊断 MS_CANVAS_NOMARK=1】跳过 QPainter 标注层——二分"QPainter 遗留
    // 状态污染下一帧 GL 合成"假说：跳过后若 frame#2+ 恢复正常 → 污染实锤；
    // 仍黑 → 污染在合成自身（与 QPainter 无关）。
    {
        static const bool s_nomark = std::getenv("MS_CANVAS_NOMARK") != nullptr;
        if (s_nomark && composed) {
            static int s_n = 0;
            if (++s_n <= 10)
                std::fprintf(stderr, "canvas-dbg: paint#%d 标注层跳过（诊断）\n",
                             s_n), std::fflush(stderr);
            return;
        }
    }
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 控件底：纯黑 = 合成输出底色（【去虚拟画布化】整个预览窗口即画布
    // 本体——不再有"画布外的舞台延伸区"，纵横比不符时的窄边与画布同色）。
    // GL 已合成时背景由 glClear 完成，跳过 fillRect（覆盖会把画面抹掉）。
    if (!composed) p.fillRect(rect(), Qt::black);

    if (!scene_) {
        p.setPen(QColor(0x90, 0x90, 0x90));
        p.drawText(rect(), Qt::AlignCenter,
                   "无场景（加载 .scene 文件后显示预览）");
        return;
    }

    // 画布内容：最近拉到的真实合成帧（所见即所得）——GL 直通已画时跳过；
    // 软路径（SoftCompose 模式/GL 回退/GpuDeliver 无帧）按域贴图。
    const QRectF cr = canvasRect();

    if (!composed) {
        // ---- 软路径合成 -----------------------------------------------------
        if (!gpu_items.empty()) {
            // (a) GpuDeliver 交付存在但 GL 失败：逐项 drawImage 回退
            //（QPainter source-over 混合与 GL 语义一致；仿射项经
            //  translate/rotate 复刻 display 几何——正确性兜底）
            if (scene_->canvas_w > 0 && gpu_w > 0 && gpu_h > 0 &&
                gpu_dom.width() > 0 && gpu_dom.height() > 0) {
                const double k  = cr.width() / scene_->canvas_w;
                const double sx = gpu_dom.width()  / gpu_w; // 画布/合成域
                const double sy = gpu_dom.height() / gpu_h;
                p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                for (const auto& it : gpu_items) {
                    // 【DXT 软解退役 2026-11-17】worker 只交裸块（GL 路径
                    // 直传）；QPainter 回退（GL 判死异常态）在此现场解码
                    // 兜底——dxtref 标准算法，≈1-3ms/帧@1080p 可接受
                    QImage lazy;
                    if (it.frame.isNull() && it.dxt) {
                        lazy = QImage(it.gpu_w, it.gpu_h,
                                      QImage::Format_RGBA8888);
                        if (!lazy.isNull())
                            dxtref::decodeToRgba(
                                it.gpu_w, it.gpu_h, it.dxt,
                                /*dxt5=*/it.dxt_format == 0x83F3,
                                lazy.bits(),
                                static_cast<std::size_t>(lazy.bytesPerLine()));
                    }
                    const QImage& f =
                        !it.frame.isNull() ? it.frame : lazy;
                    if (f.isNull()) continue;
                    // 裁切采样窗（ScreenMapper crop_u/v 消费——GL 判死
                    // 回退与 GL 合成同语义：窗内区域铺满目标矩形，内容
                    // 不缩放；全窗 = 未裁切逐位历史路径）
                    const QRectF src_win(
                        it.item.crop_u0 * f.width(),
                        it.item.crop_v0 * f.height(),
                        (it.item.crop_u1 - it.item.crop_u0) * f.width(),
                        (it.item.crop_v1 - it.item.crop_v0) * f.height());
                    // 绘制项合成域坐标 → 画布坐标：域原点 + 坐标×域缩放
                    const double cx = gpu_dom.x() + it.item.x * sx;
                    const double cy = gpu_dom.y() + it.item.y * sy;
                    const double cw = it.item.w * sx;
                    const double chh = it.item.h * sy;
                    p.setOpacity(std::clamp(it.item.opacity, 0.0, 1.0));
                    // 【颜色合成取舍 2026-11-20】QPainter 兜底不实现片段级
                    // RGB 增益/Gamma（it.item.rgb_gain/gamma）——本路径仅
                    // GL 判死应急显示（异常态，性能不设防），颜色合成缺失
                    // 可接受（DXT 解压兜底同取舍）；GL 主路径已生效
                    if (it.item.affine && it.item.canvas_w > 0 &&
                        it.item.canvas_h > 0) {
                        // 仿射（旋转）：帧按片段画布矩形缩放后绕中心旋转
                        //（帧→画布正变换 = R(rot)·缩放，display 同语义）
                        const QPointF c = canvasToWidget(QPointF(
                            it.item.canvas_x + it.item.canvas_w / 2.0,
                            it.item.canvas_y + it.item.canvas_h / 2.0));
                        p.save();
                        p.translate(c);
                        p.rotate(it.item.rotation);
                        p.drawImage(QRectF(-it.item.canvas_w * k / 2.0,
                                           -it.item.canvas_h * k / 2.0,
                                           it.item.canvas_w * k,
                                           it.item.canvas_h * k), f,
                                    src_win);
                        p.restore();
                    } else {
                        // 轴对齐：输出矩形 → 画布 → 控件直接映射
                        const QRectF target(cr.left() + cx * k,
                                            cr.top() + cy * k,
                                            cw * k, chh * k);
                        p.drawImage(target, f, src_win);
                    }
                }
                p.setOpacity(1.0);
                p.setRenderHint(QPainter::SmoothPixmapTransform, false);
            }
        } else if (!latest_frame_.isNull()) {
            // SoftCompose 现状贴图（历史行为逐行保留）
            QRectF target = cr;
            if (scene_ && scene_->canvas_w > 0 &&
                frame_domain_.width() > 0 && frame_domain_.height() > 0) {
                const double k = cr.width() / scene_->canvas_w;
                target = QRectF(cr.left() + frame_domain_.x() * k,
                                cr.top() + frame_domain_.y() * k,
                                frame_domain_.width() * k,
                                frame_domain_.height() * k);
            }
            p.setRenderHint(QPainter::SmoothPixmapTransform, true);
            p.drawImage(target, latest_frame_);
            p.setRenderHint(QPainter::SmoothPixmapTransform, false);
        }
    }

    // screens：输出区域标注（真实帧模式下不填屏体底色——避免遮住画面）。
    // 按 z 升序绘制（多屏重叠时大 z 在上层，与叠放语义一致）
    {
        // 绘制序 = 数组序（多屏重叠时靠后者覆盖靠前者，与命中一致）；
        // 【2026-11-25 选中屏最后画】重叠时选中框/标签不被后绘屏覆盖
        //（选中态恒可见——"选中后不知道选了哪个"修复的绘制半边）
        const bool no_frame = latest_frame_.isNull() && !composed;
        const int sel_scr = sel_.kind == 1 ? sel_.a : -1;
        for (int i = 0; i < (int)scene_->screens.size(); ++i)
            if (i != sel_scr)
                drawScreen(p, i, false, no_frame);
        if (sel_scr >= 0 && sel_scr < (int)scene_->screens.size())
            drawScreen(p, sel_scr, true, no_frame); // 选中屏压顶重绘
    }

    // clips：标注层（z 升序；真实帧模式下只画边框+标签，画面已在合成帧内）
    // 【只显当前】播放头不活跃的片段不进标注层（与 3D 视图 Prim/命中同口径）
    struct Order { int tr, cl; };
    std::vector<Order> order;
    for (int tr = 0; tr < (int)scene_->timeline.tracks.size(); ++tr)
        for (int cl = 0;
             cl < (int)scene_->timeline.tracks[tr].clips.size(); ++cl) {
            if (!clipActiveAtPlayhead(scene_->timeline.tracks[tr].clips[cl]))
                continue;
            order.push_back({tr, cl});
        }
    std::stable_sort(order.begin(), order.end(),
                     [this](const Order& x, const Order& y) {
                         return scene_->timeline.tracks[x.tr].clips[x.cl]
                                    .transform.z <
                                scene_->timeline.tracks[y.tr].clips[y.cl]
                                    .transform.z;
                     });
    for (const auto& od : order)
        drawClip(p, od.tr, od.cl,
                 sel_.kind == 4 && sel_.a == od.tr && sel_.b == od.cl,
                 latest_frame_.isNull() && !composed); // GL 帧已含画面不画棋盘格

    // ---- 吸附参考线（拖动会话中命中的对齐线；最上层品红细线贯穿画布）----
    if (!guide_x_.empty() || !guide_y_.empty()) {
        QPen gp(QColor(0xE0, 0x40, 0xC0), 1); // 品红（对象层中高对比）
        gp.setStyle(Qt::DashLine);
        p.setPen(gp);
        p.setBrush(Qt::NoBrush);
        for (double gx : guide_x_) {
            const double wx = canvasToWidget(QPointF(gx, 0)).x();
            p.drawLine(QPointF(wx, cr.top()), QPointF(wx, cr.bottom()));
        }
        for (double gy : guide_y_) {
            const double wy = canvasToWidget(QPointF(0, gy)).y();
            p.drawLine(QPointF(cr.left(), wy), QPointF(cr.right(), wy));
        }
        // 角点吸附标记：x/y 同时命中边线锚的交叉点 = 角对角贴合，品红
        // 实心小方块（区别于贯穿虚线，一眼可辨"角点吸上了"）。
        if (guide_pt_valid_) {
            const QPointF wp = canvasToWidget(guide_pt_);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0xE0, 0x40, 0xC0));
            p.drawRect(QRectF(wp.x() - 3.0, wp.y() - 3.0, 6.0, 6.0));
        }
    }
    // ---- 坐标轴（2026-11-05 用户简化：仅两条线，无刻度无数值）------------
    // x 轴沿画布顶边、y 轴沿画布左边——给自由缩放/平移的视图一个画布
    // 边界坐标参照（画布域原点 (0,0) 在左上）。白半透明细线，画在画布
    // 矩形边缘、随缩放平移联动（非固定视口标尺）。此前为"刻度短线+
    // 数值标注"的刻度尺形态，按用户要求去可读标注只留参照线。
    {
        // 十字坐标轴（2026-11-05 用户反馈："坐标轴应该是个十字线，现在
        // 只有第三象限"——顶边/左边两条边线只构成左上角 L 形（单象限
        // 方位参照），改为过画布中心的完整十字：x 轴沿水平中线横贯、
        // y 轴沿垂直中线纵贯，交点即画布中心，随缩放平移联动）。
        // 抗锯齿 + 1px 线的光栅化陷阱（2026-11-05 FBO 抓帧实证）：QPainter
        // 的 GL 引擎把 1px 线交给原生 GL 线光栅化，按像素中心采样——线心
        // 落在像素边界（整数坐标）时两侧采样点都在线外，整条线静默消失。
        // 故把两条轴线的线心强制对齐到像素中心（round+0.5），任何窗口
        // 尺寸下稳定可见（偏差 ≤0.5px 无感）。
        const QColor axis(255, 255, 255, 140); // 白半透明（与旧刻度线同色）
        p.setPen(QPen(axis, 1));
        p.setBrush(Qt::NoBrush);
        const qreal cx = std::round(cr.center().x()) + 0.5; // y 轴：像素列中心
        const qreal cy = std::round(cr.center().y()) + 0.5; // x 轴：像素行中心
        // x 轴：画布水平中线横贯（水平线光栅化只取决于 y）
        p.drawLine(QPointF(cr.left(), cy), QPointF(cr.right(), cy));
        // y 轴：画布垂直中线纵贯（垂直线光栅化只取决于 x）
        p.drawLine(QPointF(cx, cr.top()), QPointF(cx, cr.bottom()));
    }
}

// ---------------------------------------------------------------------------
// paintGL3D —— 3D 视口渲染主路径（正交投影 + 纯 QPainter；零 GL 管线改动）
//
// 渲染层序（painter's algorithm）：
//   1. 视口底（Blender 式深灰——3D 是"空间排布工作台"，与 2D 输出底色
//      纯黑区分，一眼辨识当前视图模式）；
//   2. 地面网格（自适应步长）+ 三轴参照线（红=画布x、绿=画布y、蓝=高度）；
//   3. cue 图元（仅片段）按【叠放层级 layer 升序、同层视深远→近】绘制：
//      片段 = 仿射贴图（GpuDeliver 源帧）或棋盘格 + 类型色边框 + 标签——
//      cue 间遮挡主键 = 层级 transform.z（空间高度 = pos_z 深度 Z）；
//   4. display 恒最上层（2026-11-05 用户定案：display 无 Z 设置、不参与
//      遮挡，只是取景框——无颜色填充，仅边框 + 在线圆点 + 标签）；
//   5. 吸附参考线（拖动会话；地面贯穿线，品红虚线）；
//   6. 选中标注（红虚线四边形 + 四角手柄 + 旋转拖柄——投影到对象高度平面）；
//   7. HUD（左上操作提示 + 右上相机读数）+ 轴球视角控件。
//  （画布舞台矩形已删——2026-11-05 用户定案：整个 xy 平面就是画布，
//   画布边界矩形无空间语义。）
//
// 贴图匹配：DeliverItem 不带 clip 指针（worker 快照置空），按几何元组
// (canvas_x/y/w/h, rotation, opacity, z) 最近距离匹配场景片段——拖动期间
// 场景几何先行（工作副本已写、快照滞后一拍），最近匹配保证"几何精确、
// 内容滞后一拍"（几何以场景工作副本为准，贴图只提供画面）。
// ---------------------------------------------------------------------------
// 【Display 面片贴图 2026-11-18 → 退役 2026-11-20 用户定案】曾把
// GpuDeliver 逐项交付合成"全局 Eye 总画面" QImage，供 3D 视口屏幕面片
void SceneCanvasWidget::paintGL3D(
        const std::vector<PreviewRenderer::DeliverItem>& items,
        int comp_w, int comp_h, const QRectF& dom) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 【Display 面片贴图源退役 2026-11-20 用户定案】不再合成全局 Eye 总
    // 画面——display 在预览中不显示任何画面，只是银幕位置的取景框。
    // comp_w/comp_h/dom 参数保留（接口稳定，当前无消费者）。

    // ---- 0. 每帧同步绝对比例 = fitScale × view3d_zoom_ ----------------------
    // 缩放语义与 2D 同构：view3d_zoom_ 是相对倍率（0.1~16），fitScale 随窗口
    // 自适应——窗口 resize / 布局就位后比例自动跟随，不依赖 setScene 时机
    //（首版把 fitScale 快照进 cam3d_.scale，布局前的小窗口值被冻结 → HUD 40%）
    {
        const double fs  = fitScale();
        cam3d_.scale = (fs > 0 ? fs : 1.0) * view3d_zoom_;
    }

    // ---- 1. 视口底 ----------------------------------------------------------
    p.fillRect(rect(), QColor(0x2B, 0x2B, 0x2B));
    if (!scene_) {
        p.setPen(QColor(0x90, 0x90, 0x90));
        p.drawText(rect(), Qt::AlignCenter,
                   "无场景（加载 .scene 文件后显示预览）");
        return;
    }

    // ---- 2. 地面网格 + 三轴参照线 ------------------------------------------
    // 可见地面范围：视口四角反投影到地面（h=0）取包围盒。低俯仰时远角
    // 反投影发散到极远——钳到注视点周围有限域（画布对角 ×30 + 5000）。
    const auto clampG = [this](const QPointF& q) {
        const double diag =
            std::sqrt(scene_->canvas_w * (double)scene_->canvas_w +
                      scene_->canvas_h * (double)scene_->canvas_h);
        const double lim = diag * 30.0 + 5000.0;
        return QPointF(std::clamp(q.x(), cam3d_.tx - lim, cam3d_.tx + lim),
                       std::clamp(q.y(), cam3d_.tz - lim, cam3d_.tz + lim));
    };
    const QPointF c00 = clampG(widgetToCanvasAt(QPointF(0, 0), 0.0));
    const QPointF c10 = clampG(widgetToCanvasAt(QPointF(width(), 0), 0.0));
    const QPointF c01 = clampG(widgetToCanvasAt(QPointF(0, height()), 0.0));
    const QPointF c11 = clampG(widgetToCanvasAt(QPointF(width(), height()), 0.0));
    const double gx0 = std::min({c00.x(), c10.x(), c01.x(), c11.x()});
    const double gx1 = std::max({c00.x(), c10.x(), c01.x(), c11.x()});
    const double gz0 = std::min({c00.y(), c10.y(), c01.y(), c11.y()});
    const double gz1 = std::max({c00.y(), c10.y(), c01.y(), c11.y()});

    // 自适应步长：屏上网格间距保持 24~160px（×5 进制，十进制读数友好）
    double step = 100.0;
    while (step * cam3d_.scale < 24.0) step *= 5.0;
    while (step * cam3d_.scale > 160.0 && step > 1.0) step /= 5.0;

    p.setPen(QPen(QColor(0x3A, 0x3A, 0x3A), 1)); // 网格（略亮于视口底）
    p.setBrush(Qt::NoBrush);
    const int i0 = (int)std::floor(gx0 / step), i1 = (int)std::ceil(gx1 / step);
    const int j0 = (int)std::floor(gz0 / step), j1 = (int)std::ceil(gz1 / step);
    if ((i1 - i0) + (j1 - j0) < 400) { // 极端缩放下放弃细网格（防卡顿）
        for (int i = i0; i <= i1; ++i)
            p.drawLine(project3(i * step, 0, gz0), project3(i * step, 0, gz1));
        for (int j = j0; j <= j1; ++j)
            p.drawLine(project3(gx0, 0, j * step), project3(gx1, 0, j * step));
    }
    // 三轴参照线（Blender 配色语义：红=X、绿=Y）——蓝高度轴已随 2D 化
    // 退役（2026-11-20：pos_z 恒 0，竖直轴无空间语义可示）
    p.setPen(QPen(QColor(0xA8, 0x52, 0x52), 1.4));
    p.drawLine(project3(gx0, 0, 0), project3(gx1, 0, 0)); // 画布 x 轴（世界 X）
    p.setPen(QPen(QColor(0x52, 0xA0, 0x5E), 1.4));
    p.drawLine(project3(0, 0, gz0), project3(0, 0, gz1)); // 画布 y 轴（世界 Z）

    // ---- 3.（舞台矩形已删——2026-11-05 用户定案：整个 xy 平面就是画布，
    //      画布边界矩形没有空间语义；地面网格 + 三轴即全部参照物）--------------

    // ---- 4. 图元收集 + 视深排序（远→近 painter's algorithm）-----------------
    // 贴图表：GpuDeliver 逐项交付的源帧。匹配键 = clip_id 精确相等
    //（DeliverItem.clip_id 由 pump 快照时随项携带，全程自持）。
    // 【修复 2026-11-05 贴图错配】旧实现按几何元组（x/y/w/h/rot/opa/z）
    // 最近距离匹配——items 几何是合成输出域（≈画布×fit 缩放），域差成
    // 为共同常数后，同 z/rot/opa 的相邻片段间距离只剩 dx/dy 微差：文本
    // 片段帧缺失（解码/就绪滞后一拍）时，同层邻近的视频帧被误配给文本
    // 面片（用户实证：clip-6 文本显示别人的画面）。id 匹配未命中 → 棋
    // 盘格占位（宁缺勿错：占位可接受，显示错误内容是完整性事故）。
    struct TexEntry {
        std::string clip_id; // 交付项的 clip 身份（空 = 无法匹配）
        const PreviewRenderer::DeliverItem* it; // 三形态域交付项（frame/
                                                //  dxt 二选一；native 项已滤）
    };
    std::vector<TexEntry> texs;
    texs.reserve(items.size());
    for (const auto& it : items) {
        // 三形态域防御：frame 与 dxt 均空 = native 项（CUDA 域，3D 视口
        // 不消费——硬解协商回退下载后 frame 才出现）→ 棋盘格占位
        if (it.frame.isNull() && !it.dxt) continue;
        texs.push_back({it.clip_id, &it});
    }
    auto matchTex = [&texs](const std::string& clip_id)
            -> const PreviewRenderer::DeliverItem* {
        if (clip_id.empty()) return nullptr;
        for (const auto& t : texs) {
            if (t.clip_id == clip_id) return t.it; // 同 id 多项取首个
        }
        return nullptr; // 本片段无交付帧（未就绪/出区间）→ 棋盘格
    };

    struct Prim {
        int       kind;    // 4=clip（display 已不入 prims——恒最上层单独绘制）
        int       a, b;    // 场景下标
        double    depth;   // 视深（大 = 远）
        double    h;       // 悬浮高度（clip = pos_z 深度 Z——三维空间真实
                           // z 位置；2026-11-18 语义终定）
        double    layer;   // 【遮挡主键】叠放层级 transform.z——与合成器
                           // Stacking By Layer 同语义：低层先画、高层后画；
                           // 非空间坐标，不参与高度（2026-11-18 语义终定）
        QPolygonF quad;    // 投影四边形（0=TL 1=TR 2=BR 3=BL）
        QTransform tex;    // 画布局部矩形 → 屏幕（仿射；贴图/棋盘共用）
        const QImage* img; // 匹配源帧（可空 = 棋盘格占位；dxt 项 img 为空
                           //  但 GL 贴图仍可用——QPainter 回退路径专用）
        const PreviewRenderer::DeliverItem* it; // 三形态域交付（GL 贴图源）
        bool deformed = false; // 变形片段（锚点≠中心或角点位移）——贴图
                               // 改走两三角仿射分块（quad 非平行四边形，
                               // 单仿射 tex 数学前提已破）
        QRectF vis;            // 裁切后可见矩形（画布域；未裁切 = 名义矩形）
                               // ——面片几何/贴图/棋盘全按它解算
                               //（ScreenMapper judge 同式，2026-11-17）
        double cuv[4] = {0, 0, 1, 1}; // 帧采样 UV 窗 {u0,v0,u1,v1}（归一化；
                                      // 全 0/1 = 未裁切——GL/QPainter 两
                                      // 路同消费，与 display 同式）
        Transform tf;          // tween 解算快照（收集期求值一次——绘制期
                               // 透明度/调色/吊线/选中标注直接复用，消除
                               // 每帧每片段多次全通道重解算）
    };
    // 【display/cue 关系 2026-11-05 用户定案】display 是"取景器"而非实体：
    // 没有 Z 设置、不参与 cue 遮挡排序、无颜色填充只画边框、恒绘制在
    // 所有 cue 之上（WATCHOUT Stage 同风格）。prims 只装 cue。
    std::vector<Prim> prims;
    prims.reserve(8);
    // display 投影四边形（收集后于所有 cue 绘制完再画——恒最上层）
    std::vector<std::pair<int, QPolygonF>> screen_quads;
    screen_quads.reserve(scene_->screens.size());
    // Show 全局透视仪（场景级常量——循环外求值一次）
    double quad_ef = 0.0, quad_evx = 0.0, quad_evy = 0.0;
    scene_->effectivePersp(quad_ef, quad_evx, quad_evy);
    for (int i = 0; i < (int)scene_->screens.size(); ++i) {
        const Screen& s = scene_->screens[i];
        if (s.w <= 0 || s.h <= 0) continue;
        // 屏姿态（2026-11-17 阶段2；2026-11-20 透视压平统一）：俯仰/偏航
        // 非 0 → 取景框 = quadCorners3DP 透视压平四角（与素材 posed 分支
        // 完全同模式同源：针孔压扁近大远小 + 滚转 + Show 全局透视仪
        // Scene::effectivePersp 全部烘焙，压平后画布域四角逐角贴地
        // z=0——display 取景器无高度语义）。
        // 无姿态 → 历史 projectQuad 逐位（纯滚转含锚点/位移同路径）。
        if (s.rot_x != 0.0 || s.rot_y != 0.0) {
            double wx[4], wy[4];
            cg::quadCorners3DP(
                s.x, s.y, s.w, s.h, 0.0 /*取景窗无深度*/, s.rotation,
                s.rot_x, s.rot_y, 0 /*锚=中心*/, nullptr,
                quad_ef, quad_evx, quad_evy, wx, wy, nullptr);
            QPolygonF q(4);
            for (int k = 0; k < 4; ++k)
                q[k] = project3(wx[k], 0.0, wy[k]);
            screen_quads.push_back({i, q});
        } else {
            screen_quads.push_back(
                {i, projectQuad(QRectF(s.x, s.y, s.w, s.h), s.rotation, 0.0)});
        }
    }
    for (int tr = 0; tr < (int)scene_->timeline.tracks.size(); ++tr) {
        const auto& clips = scene_->timeline.tracks[tr].clips;
        for (int cl = 0; cl < (int)clips.size(); ++cl) {
            const Clip&   c = clips[cl];
            // 【音频/Control 片段不画】无图像数据——3D 视图同样不产面片
            //（与 2D drawClip / 命中测试同口径；2026-11-15 改片段级判定，
            // 轨道去类型化后无类型轨上的隐形片段不再漏网）
            if (trackInvisibleOnCanvas(scene_->timeline.tracks[tr].kind,
                                       c.type)) continue;
            // 【只显当前】播放头不活跃的片段不画（连方框占位也无——
            // WATCHOUT Stage 语义；命中同口径过滤见 hitTest）
            if (!clipActiveAtPlayhead(c)) continue;
            QRectF r = clipCanvasRect(c);
            if (r.width() <= 0 || r.height() <= 0) continue;
            const Transform tf = clipResolvedTransform(c); // tween 求值
            // 【裁切】几何全按裁后可见矩形解算 + uv 采样窗随 Prim 下传
            //（display ScreenMapper judge 同式：内容不缩放，裁掉部分从
            // 采样窗移除，可见区仍铺满裁后矩形——GL/QPainter 两路同消费）
            double cuv[4];
            clipCropApply(tf, r, cuv);
            const double h = tf.pos_z; // 悬浮高度 = 深度 Z（3D 空间真实
                                       // z 位置；层级 transform.z 不再参
                                       // 与高度——2026-11-18 语义终定）
            // 变形片段（2026-11-09 锚点/角点；2026-11-10 姿态 3D；阶段③
            // 透视）：四角绕锚点 + 位移（非平行四边形）；3D 位姿 = 透视
            // 四边形（quadCorners3DP 预投影——含滚转/锚点/位移/深度/全场
            // 透视全部语义，与 display ScreenMapper pose3d 链同源唯一
            // 真相）；默认中心锚无姿态无位移走历史路径（projectQuad 逐位）。
            // 【画布↔display 一致性】此前 deformed 漏判 posed——俯仰/偏航
            // 只进 display（ScreenMapper pose3d 链），3D 视图画面恒为
            // 2D 矩形（用户体感"XY 旋转无效"）；deformed 含 posed 后
            // 贴图/棋盘格/边框/选中框自动走四边形本体路径（pr.quad）。
            // 【2026-11-20 透视压平统一】3D 视口素材四角 = quadCorners3DP
            // 透视压平（与 display ScreenMapper 同源）。
            const bool posed  = tf.pose3DActive();
            const bool deformed =
                posed || tf.anchor != 0 || tf.cornerActive();
            QPolygonF q(4);
            if (posed) {
                // 【透视压平 2026-11-20 用户拍板】素材三轴旋转按透视变换
                // 映射到 XY 平面：quadCorners3DP 预投影（与 display
                // ScreenMapper pose3d 链完全同源——针孔压扁近大远小 +
                // 滚转/锚点/位移/深度/Show 全局透视仪 Scene::
                // effectivePersp 全部烘焙，压平后画布域四角）。面片贴地
                // 呈梯形（近大远小）；内容透视压缩由 GL 贴图路径
                //（Canvas3DGl::drawClipFace 屏幕四点单应）精确复现，
                // QPainter 兜底为两三角仿射分块（与 SoftRenderer 同构）。
                // 【性能】effectivePersp 是场景级常量（循环外已求值一次
                // ——quad_ef/quad_evx/quad_evy），此处直接复用。
                double wx[4], wy[4];
                cg::quadCorners3DP(r.x(), r.y(), r.width(), r.height(),
                                   tf.pos_z, tf.rotation, tf.rot_x,
                                   tf.rot_y, tf.anchor,
                                   tf.cornerActive() ? tf.corner : nullptr,
                                   quad_ef, quad_evx, quad_evy, wx, wy,
                                   nullptr);
                for (int i = 0; i < 4; ++i)
                    q[i] = project3(wx[i], 0.0, wy[i]); // 压平贴地 → 舞台相机
            } else {
                q = projectQuad(r, tf.rotation, h, tf.anchor,
                                deformed ? tf.corner : nullptr);
            }
            // 仿射（画布局部 (0,0)-(w,h) → 投影平行四边形）：三对应点。
            // 列向量 = (P1−P0)/w 与 (P3−P0)/h——正交投影保平行性，此
            // 变换精确（无透视分量），QTransform 仿射足够。
            // 【条件化】变形片段贴图走两三角分块（pr.tex 不消费），恒等
            // 占位即可（调试探针的 mapRect 读数对 deformed 无意义）
            QTransform m = deformed
                ? QTransform()
                : QTransform((q[1].x() - q[0].x()) / r.width(),
                             (q[1].y() - q[0].y()) / r.width(),
                             (q[3].x() - q[0].x()) / r.height(),
                             (q[3].y() - q[0].y()) / r.height(),
                             q[0].x(), q[0].y());
            const PreviewRenderer::DeliverItem* matched = matchTex(c.id);
            // 视觉高度：透视压平后旋转诱导深度已烘焙进四角几何（近大
            // 远小由四角位置表达），面片贴地无空间高度——pos_z 恒 0，
            // 深度按画布平面位置算（遮挡排序键见下）
            const double h_vis = h;
            prims.push_back(
                {4, tr, cl,
                 cam3d_.depthOf(r.center().x(), h_vis, r.center().y()),
                 h_vis, (double)c.transform.z, q, m,
                             (matched && !matched->frame.isNull())
                                 ? &matched->frame : nullptr,
                             matched, deformed, r,
                             {cuv[0], cuv[1], cuv[2], cuv[3]}, tf});
        }
    }
    // 【遮挡排序 2026-11-18 语义终定】cue 间遮挡主键 = 叠放层级 transform.z
    //（低层先画、高层后画——与 2D 标注层/display 合成链 Stacking 同键）；
    // 同层级按视深 tie-break（远的先画——含 pos_z 空间高度的物理遮挡）。
    // display 不参与排序（取景框恒最上层，见绘制段 5b）。
    std::stable_sort(prims.begin(), prims.end(),
                     [](const Prim& a, const Prim& b) {
                         if (a.layer != b.layer)
                             return a.layer < b.layer; // 层级低先画
                         return a.depth > b.depth;     // 远的先画
                     });

    // 【诊断 MS_CANVAS_DBG=1】3D 图元统计（限首 5 拍——场景加载/片段投影
    // 排查入口：tracks 数、clip 数、投影后 quad 中心坐标）
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        static int s_n          = 0;
        if (s_dbg && ++s_n <= 5) {
            int nclip = 0;
            for (const auto& t : scene_->timeline.tracks)
                nclip += (int)t.clips.size();
            std::fprintf(stderr,
                         "canvas-3d: paint#%d tracks=%d clips=%d prims=%zu "
                         "scale=%.3f t=(%.0f,%.0f) items=%zu\n",
                         s_n, (int)scene_->timeline.tracks.size(), nclip,
                         prims.size(), cam3d_.scale, cam3d_.tx, cam3d_.tz,
                         items.size());
            for (size_t ti = 0; ti < items.size() && ti < 4; ++ti) {
                std::fprintf(stderr,
                             "canvas-3d:   item#%zu frame_null=%d "
                             "native=%d gpu=%dx%d z=%d\n",
                             ti, (int)items[ti].frame.isNull(),
                             (int)(items[ti].native != nullptr),
                             items[ti].gpu_w, items[ti].gpu_h,
                             items[ti].item.z);
                if (!items[ti].frame.isNull()) {
                    const QImage& f  = items[ti].frame;
                    const QRgb   px  = f.pixel(0, 0);
                    const QRgb   px2 = f.pixel(f.width() / 2, f.height() / 2);
                    std::fprintf(stderr,
                                 "canvas-3d:     frame %dx%d fmt=%d "
                                 "px(0,0)=(%d,%d,%d,a%d) px(mid)=(%d,%d,%d,a%d)\n",
                                 f.width(), f.height(), (int)f.format(),
                                 qRed(px), qGreen(px), qBlue(px), qAlpha(px),
                                 qRed(px2), qGreen(px2), qBlue(px2),
                                 qAlpha(px2));
                }
            }
            for (const Prim& q : prims) {
                if (q.kind != 4) continue;
                std::fprintf(stderr,
                             "canvas-3d:   clip prim depth=%.0f "
                             "center=(%.0f,%.0f)\n",
                             q.depth, q.quad[0].x(), q.quad[0].y());
            }
            std::fflush(stderr);
        }
    }

    // ---- 5. 图元绘制（仅 cue——遮挡纯靠 Z 轴排序；display 恒最上层见 5b）---
    for (const Prim& pr : prims) {
        {
            // ---- 片段（悬浮于 pos_z 深度平面）-------------------------
            const Clip&   c = scene_->timeline.tracks[pr.a].clips[pr.b];
            const QRectF r = pr.vis; // 裁切后可见矩形（构建期已解算——
                                     // 与面片几何/贴图同源）
            // tween 求值统一入口：读收集期快照（Prim.tf——本片段绘制全程
            // 共用：透明度/调色/吊线/选中标注，不再全通道重解算）
            const Transform& tf = pr.tf;
            const bool    sel = (sel_.kind == 4 && sel_.a == pr.a &&
                                 sel_.b == pr.b);
            // 类型色（与 2D 路径/时间轴同组色源——trackKindColor 单点维护）
            const QColor border = trackKindColor(
                scene_->timeline.tracks[pr.a].kind);
            // 高度吊线（类型色半透明）：地面中心 → 悬浮中心——深度 Z 的
            // 空间参照（pos_z=0 不画，贴地无位移）
            const QPointF c0 = project3(r.center().x(), 0.0, r.center().y());
            // 【2026-11-18 语义终定】悬浮高度 = 深度 Z pos_z（层级 transform.z
            // 不再参与空间高度；旋转诱导深度在中心处 ≈0——锚点局部
            // (0,0) 旋转不变）
            const double h_mid = tf.pos_z;
            const QPointF c1 = project3(r.center().x(), h_mid,
                                        r.center().y());
            if (tf.pos_z != 0) {
                p.setPen(QPen(QColor(border.red(), border.green(), border.blue(),
                                     90), 1.0));
                p.drawLine(c0, c1);
            }
            // 片段面：仿射贴图（源帧）或棋盘格占位
            p.setTransform(QTransform());
            {
                static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
                static int s_n          = 0;
                if (s_dbg && ++s_n <= 15) {
                    QRectF mb = pr.tex.mapRect(QRectF(0, 0, r.width(),
                                                      r.height()));
                    std::fprintf(stderr,
                                 "canvas-3d: draw#%d clip=(%s) img=%s(%dx%d) "
                                 "texs=%zu z=%d texmap=(%.0f,%.0f %.0fx%.0f) "
                                 "opa=%.2f\n",
                                 s_n, c.id.c_str(),
                                 pr.img ? "yes" : "null",
                                 pr.img ? pr.img->width() : 0,
                                 pr.img ? pr.img->height() : 0,
                                 texs.size(), c.transform.z,
                                 mb.x(), mb.y(), mb.width(), mb.height(),
                                 tf.opacity),
                            std::fflush(stderr);
                }
            }
            // ---- 面片：GL 贴图优先（阶段1 GL 化 2026-11-17）----------------
            // 裸 GL 画片段面片：HAP DXT 压缩纹理直传（零 CPU 解压——
            // worker 软解退役的前提）+ 单应透视校正（光栅 /W 插值逐点
            // 精确，两三角仿射的数学缺陷在 GL 路径根治）。QPainter 夹层
            // 用官方 begin/endNativePainting 混用模式；GL 失败（初始化/
            // 上传/退化四边形）或 MS_CANVAS_GL3D=0 → 下方 QPainter 原路径
            // 逐位兜底（棋盘格/仿射贴图语义不变）。
            bool gl_drawn = false;
            {
                static const bool s_gl3d_on = [] {
                    const char* e = std::getenv("MS_CANVAS_GL3D");
                    return e == nullptr || e[0] != '0';
                }();
                if (pr.it && (!pr.it->frame.isNull() || pr.it->dxt) &&
                    s_gl3d_on) {
                    if (!gl3d_) gl3d_ = std::make_unique<Canvas3DGl>();
                    gl3d_->frame(width(), height(),
                                 (float)devicePixelRatioF());
                    Canvas3DGl::TexData td;
                    if (pr.it->dxt) { // DXT 域：裸块+格式+尺寸直透
                        td.dxt        = pr.it->dxt;
                        td.dxt_format = pr.it->dxt_format;
                        td.width      = (int)pr.it->gpu_w;
                        td.height     = (int)pr.it->gpu_h;
                    } else { // CPU RGBA 域（软解/硬解下载回退）
                        td.rgba   = pr.it->frame.constBits();
                        td.stride = (std::uint32_t)pr.it->frame.bytesPerLine();
                        td.width  = pr.it->frame.width();
                        td.height = pr.it->frame.height();
                    }
                    const double qx[4] = {pr.quad[0].x(), pr.quad[1].x(),
                                          pr.quad[2].x(), pr.quad[3].x()};
                    const double qy[4] = {pr.quad[0].y(), pr.quad[1].y(),
                                          pr.quad[2].y(), pr.quad[3].y()};
                    p.beginNativePainting();
                    // 【贴图语义 2026-11-23 用户定案】corner 拖角 = 双线性
                    // 细分（无透视，与 display 同构同密度）；纯旋转/非中心
                    // 锚（平行四边形）= submitFace 单应退 W≡1 精确仿射
                    // （6 顶点，免 2112 细分）；3D 姿态压平 = 单应透视
                    // w 顶点（冻结路径，角位移已烘焙进 pr.quad）。
                    gl_drawn = gl3d_->drawClipFace(
                        c.id, td, qx, qy,
                        (float)std::clamp(tf.opacity, 0.0, 1.0),
                        (float)pr.cuv[0], (float)pr.cuv[1],
                        (float)pr.cuv[2], (float)pr.cuv[3],
                        tf.rgb_gain, tf.gamma,
                        tf.cornerActive() && !tf.pose3DActive());
                    p.endNativePainting();
                }
            }
            // ---- QPainter 回退贴图源：DXT 项现场解码（软解退役兜底）----
            // worker 只交裸块；GL 判死（S3TC 不支持/初始化失败）时在此
            // 解码出 RGBA——回退异常态每帧每片段 1-3ms 可接受
            QImage face_lazy;
            if (!gl_drawn && pr.it && pr.it->dxt && !pr.img) {
                face_lazy = QImage((int)pr.it->gpu_w, (int)pr.it->gpu_h,
                                   QImage::Format_RGBA8888);
                if (!face_lazy.isNull())
                    dxtref::decodeToRgba(
                        pr.it->gpu_w, pr.it->gpu_h, pr.it->dxt,
                        /*dxt5=*/pr.it->dxt_format == 0x83F3,
                        face_lazy.bits(),
                        static_cast<std::size_t>(face_lazy.bytesPerLine()));
            }
            const QImage* face_img =
                pr.img ? pr.img
                       : (!face_lazy.isNull() ? &face_lazy : nullptr);
            // ---- 片段级调色（QPainter 回退路径；GL 主路径 shader 已消费）--
            // 与 display 同式（增益→clamp→pow(1/γ)）；全默认零开销跳过，
            // 快照 QImage 共享不改写（副本进 face_tinted 持有）。
            QImage face_tinted;
            if (!gl_drawn && face_img &&
                (tf.rgb_gain[0] != 1.0 || tf.rgb_gain[1] != 1.0 ||
                 tf.rgb_gain[2] != 1.0 || tf.gamma != 1.0)) {
                face_tinted =
                    applyFaceColor(*face_img, tf.rgb_gain, tf.gamma);
                face_img = &face_tinted;
            }
            if (!gl_drawn &&
                face_img && !face_img->isNull() && r.width() > 0 && r.height() > 0) {
                const double sx = r.width()  / face_img->width();
                const double sy = r.height() / face_img->height();
                if (pr.deformed) {
                    // ---- 变形四边形：两三角仿射分块贴图 ----------------
                    // 对角线 LB→RT 分解（三角 A=RT,RB,LB / B=RT,LB,LT）
                    // 与 display 端 SoftRenderer::drawQuadSampling 同构
                    //（同分解 → 非平行四边形时对角线两侧的细微采样差异
                    // 两端一致，预览与输出视觉统一）。每三角独立解仿射 +
                    // clip 后 drawImage（光栅管线不支持投影变换贴图）。
                    const QPointF* q = pr.quad.data();
                    struct Tri { int i0, i1, i2; };
                    const Tri tris[2] = {{1, 2, 3}, {1, 3, 0}}; // RT RB LB / RT LB LT
                    const QPointF src[2][3] = {
                        {QPointF(r.width(), 0), QPointF(r.width(), r.height()),
                         QPointF(0, r.height())},                     // A 源
                        {QPointF(r.width(), 0), QPointF(0, r.height()),
                         QPointF(0, 0)},                              // B 源
                    };
                    for (int t = 0; t < 2; ++t) {
                        const QPointF& s0 = src[t][0];
                        const QPointF& s1 = src[t][1];
                        const QPointF& s2 = src[t][2];
                        const QPointF& d0 = q[tris[t].i0];
                        const QPointF& d1 = q[tris[t].i1];
                        const QPointF& d2 = q[tris[t].i2];
                        // 3 点仿射解：x' = a·x + c·y + e；y' = b·x + d·y + f
                        const double det = (s1.x() - s0.x()) * (s2.y() - s0.y()) -
                                           (s2.x() - s0.x()) * (s1.y() - s0.y());
                        if (std::fabs(det) < 1e-12) continue; // 零面积防御
                        const double dx1 = d1.x() - d0.x(), dy1 = d1.y() - d0.y();
                        const double dx2 = d2.x() - d0.x(), dy2 = d2.y() - d0.y();
                        const double ax1 = s1.x() - s0.x(), ay1 = s1.y() - s0.y();
                        const double ax2 = s2.x() - s0.x(), ay2 = s2.y() - s0.y();
                        const double ta = (dx1 * ay2 - dx2 * ay1) / det;
                        const double tc = (dx2 * ax1 - dx1 * ax2) / det;
                        const double tb = (dy1 * ay2 - dy2 * ay1) / det;
                        const double td = (dy2 * ax1 - dy1 * ax2) / det;
                        QTransform tri(ta, tb, tc, td,
                                       d0.x() - ta * s0.x() - tc * s0.y(),
                                       d0.y() - tb * s0.x() - td * s0.y());
                        QPainterPath clip_path;
                        clip_path.addPolygon(
                            QPolygonF({d0, d1, d2}));
                        clip_path.closeSubpath();
                        p.save();
                        p.setClipPath(clip_path);
                        // 变换复合同主路径：像素 →(源窗平移+缩放)→ 画布局
                        // 部 →(tri)→ 控件（Qt 行向量 A*B = 先 A 后 B）。
                        // 源域 = 裁切采样窗：窗内像素平移到原点后铺满局部
                        //（未裁切 = 恒等平移 + 原缩放，历史路径逐位不变）
                        const QTransform srcmap =
                            QTransform::fromTranslate(
                                -pr.cuv[0] * face_img->width(),
                                -pr.cuv[1] * face_img->height()) *
                            QTransform::fromScale(
                                sx / std::max(pr.cuv[2] - pr.cuv[0], 1e-12),
                                sy / std::max(pr.cuv[3] - pr.cuv[1], 1e-12));
                        p.setTransform(srcmap * tri);
                        p.setOpacity(std::clamp(tf.opacity, 0.0, 1.0));
                        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                        p.drawImage(QPointF(0, 0), *face_img);
                        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
                        p.restore();
                    }
                    p.setOpacity(1.0);
                } else {
                // 变换复合：【Qt 行向量约定 A*B = 先 A 后 B】图像像素 (u,v)
                // 先按 fromScale 放大到画布局部尺寸，再经 pr.tex 投影到屏幕
                //（首版写成 pr.tex * fromScale——实际先 tex 后放大，图像被
                // 推到视口外数千像素处，表现为"片段永远黑"）。
                // 【裁切】源域 = 采样窗：窗内像素平移到原点后铺满局部
                //（ScreenMapper uv 窗同式；未裁切 = 恒等，逐位历史路径）
                const QTransform srcmap =
                    QTransform::fromTranslate(-pr.cuv[0] * face_img->width(),
                                              -pr.cuv[1] * face_img->height()) *
                    QTransform::fromScale(
                        sx / std::max(pr.cuv[2] - pr.cuv[0], 1e-12),
                        sy / std::max(pr.cuv[3] - pr.cuv[1], 1e-12));
                p.setTransform(srcmap * pr.tex);
                p.setOpacity(std::clamp(tf.opacity, 0.0, 1.0));
                p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                p.drawImage(QPointF(0, 0), *face_img);
                p.setRenderHint(QPainter::SmoothPixmapTransform, false);
                p.setOpacity(1.0);
                }
            } else if (!gl_drawn && pr.deformed) {
                // 变形占位：棋盘格裁进位移后四边形（AABB 铺底 + clip）
                QPainterPath clip_path;
                clip_path.addPolygon(pr.quad);
                clip_path.closeSubpath();
                p.save();
                p.setClipPath(clip_path);
                drawChecker(p, pr.quad.boundingRect());
                p.restore();
            } else if (!gl_drawn) {
                p.setTransform(pr.tex);
                drawChecker(p, QRectF(0, 0, r.width(), r.height()));
            }
            // 边框（屏幕对齐重画——投影四边形描边）
            p.setTransform(QTransform());
            p.setPen(QPen(border, sel ? 2.2 : 1.2));
            p.setBrush(Qt::NoBrush);
            p.drawPolygon(pr.quad);
            // 标签（投影 TL 角旁，水平可读）
            {
                QFont f = p.font();
                f.setPointSize(8);
                p.setFont(f);
                p.setPen(QColor(0xE8, 0xE8, 0xE8));
                p.drawText(QRectF(pr.quad[0].x() + 5, pr.quad[0].y() + 3,
                                  160, 14),
                           Qt::AlignLeft | Qt::AlignTop,
                           QString::fromStdString(c.id));
            }
            if (sel) {
                // 选中标注：素材手柄（2026-11-20 统一交互优化，与 2D 标
                // 注层同式同源）——视觉四角/边线（四角 = 角点位移拖拽、
                // 四边 = 尺寸缩放）；角点经本面片同高度平面投影
                QPolygonF quad_c = clipVisualQuadCanvas(c, tf);
                QPolygonF quad_w;
                quad_w.reserve(4);
                for (int i = 0; i < 4; ++i)
                    quad_w << project3(quad_c[i].x(), tf.pos_z,
                                       quad_c[i].y());
                drawClipHandles(p, quad_w);
                if (tf.anchor == 0 && !tf.cornerActive() &&
                    !tf.pose3DActive()) { // 旋转柄仅限未变形矩形
                    drawRotateHandle(p, clipCanvasRect(c), tf.rotation,
                                     tf.pos_z);
                }
            }
        }
    }

    // ---- 5b. display 绘制（恒最上层 · 取景框语义，2026-11-05 用户定案）------
    // display 不参与 cue 遮挡排序：画在所有 cue 之后 = 恒覆盖其上。
    // 【面片贴图退役 2026-11-20 用户定案】display 在预览中不显示任何画面，
    // 只是银幕位置的取景框——画面贴图三层路径（阶段2 FBO 精确路径 /
    // 全局总画面 UV 兜底 / QPainter 两三角）全部退役；Display 校色只作用
    // 于正式输出（ScreenMapper 合成端），预览画布零体现。
    for (const auto& sq : screen_quads) {
        const Screen& s = scene_->screens[sq.first];
        const bool sel  = (sel_.kind == 1 && sel_.a == sq.first);
        p.setTransform(QTransform());

        p.setPen(QPen(sel ? QColor(0xE8, 0xE8, 0xE8) : QColor(0x8A, 0x8A, 0x8A),
                      sel ? 1.8 : 1.0));
        p.setBrush(Qt::NoBrush); // 取景框不填充（2026-11-20 定案：不显示画面）
        p.drawPolygon(sq.second);
        // 在线圆点 + 标签（投影 TL 角旁，屏幕对齐可读）
        double lx = 6;
        bool online = false; // 中线名随连接状态变色（16:44 用户要求）
        if (!s.player_id.empty()) {
            online = display_online_.count(s.player_id) != 0 &&
                     display_online_.at(s.player_id);
            p.setPen(Qt::NoPen);
            p.setBrush(online ? QBrush(QColor(0x33, 0xB2, 0x6E))
                              : QBrush(QColor(0xE0, 0x55, 0x55)));
            p.drawEllipse(sq.second[0] + QPointF(9.0, 9.0), 3.0, 3.0);
            p.setBrush(Qt::NoBrush);
            p.setPen(QColor(0xC8, 0xC8, 0xC8));
            lx = 17;
        }
        QFont f = p.font();
        f.setPointSize(9);
        p.setFont(f);
        const QString label =
            QString("%1 → %2")
                .arg(QString::fromStdString(s.id),
                     s.player_id.empty() ? QString("(未绑定)")
                                         : QString::fromStdString(s.player_id));
        p.drawText(QRectF(sq.second[0].x() + lx, sq.second[0].y() + 3,
                          220, 16),
                   Qt::AlignLeft | Qt::AlignTop, label);
        if (sel) {
            QRectF r;
            double rot = 0.0;
            if (selectionCanvasRect(&r, &rot)) {
                // 选中虚线框（2026-11-20 透视压平统一）：姿态屏 = 取景框
                // 同款 quadCorners3DP 透视四边形（膨胀 2.5px——名义矩形
                // 膨胀后过同一条投影链，与素材 posed 选中框同模式：只显
                // 旋转后在 XY 平面的映射）；手柄/旋转柄同隐（透视四边形
                // 无矩形柄语义，与 hitResizeHandle 命中同隐——素材先例）。
                // 非姿态屏 = 历史 projectQuad 滚转框 + 手柄逐位。
                p.setPen(QPen(QColor(0xFF, 0x33, 0x33), 1.6, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                if (s.rot_x != 0.0 || s.rot_y != 0.0) {
                    // effectivePersp 场景级常量复用（循环外 quad_* 已求值）
                    double wx[4], wy[4];
                    cg::quadCorners3DP(
                        s.x - 2.5, s.y - 2.5, s.w + 5.0, s.h + 5.0,
                        0.0, s.rotation, s.rot_x, s.rot_y, 0, nullptr,
                        quad_ef, quad_evx, quad_evy, wx, wy, nullptr);
                    QPolygonF box(4);
                    for (int k = 0; k < 4; ++k)
                        box[k] = project3(wx[k], 0.0, wy[k]);
                    p.drawPolygon(box);
                } else {
                    p.drawPolygon(projectQuad(
                        r.adjusted(-2.5, -2.5, 2.5, 2.5), rot, 0.0));
                    drawResizeHandles(p, r, rot, 0.0);
                    drawRotateHandle(p, r, rot, 0.0);
                }
            }

            // 【2026-11-25 选中中线名】投影取景框中央加粗 display 名——
            // 与 drawScreen 选中分支同款（双路径统一入口 drawCenterName）
            drawCenterName(p, sq.second.boundingRect(), s.player_id, online);
        }
    }
    p.setTransform(QTransform());

    // ---- 6. 吸附参考线（拖动会话；地面贯穿线，品红虚线）---------------------
    if (!guide_x_.empty() || !guide_y_.empty()) {
        p.setPen(QPen(QColor(0xE0, 0x40, 0xC0), 1, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        for (double gx : guide_x_)
            p.drawLine(project3(gx, 0, gz0), project3(gx, 0, gz1));
        for (double gy : guide_y_)
            p.drawLine(project3(gx0, 0, gy), project3(gx1, 0, gy));
        // 角点吸附标记：交叉点画布坐标 (x, y) 落在地面 y=0（与横线
        // guide_y 的投影 (·,0,·) 同口径），品红实心小方块。
        if (guide_pt_valid_) {
            const QPointF wp = project3(guide_pt_.x(), 0, guide_pt_.y());
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0xE0, 0x40, 0xC0));
            p.drawRect(QRectF(wp.x() - 3.0, wp.y() - 3.0, 6.0, 6.0));
        }
    }

    // ---- 6b. 全局消隐点 VP + 观察眼点 Eye 标记（仅手动 Eye 已配置）--------
    // 橙色十字圆标 = 透视投影的唯一消隐点，可直接拖动（对齐 WATCHOUT
    // Stage Perspective 的 Vanishing Point 语义）；其正上方 F 高度处的
    // 圆环 = 针孔模型观察眼点 Eye（s=F/(F−Z) 的物理出处——素材升向
    // Eye 放大、远离缩小）。未配置时不画——派生默认 VP 无编辑意义
    //（避免误导）。改 F 的可视反馈 = Eye 环随焦距升降。
    if (scene_->persp_focal > 0.0) {
        // 【2026-11-18 用户定案】VP Gizmo 选中制：点 VP/Eye 选中（与对象
        // 选中互斥，applySelection 统一收口）——Gizmo 三轴仅选中时显示；
        // 选中态 VP/Eye 标记提亮（与选中框同语义）
        const bool vp_hot = vp_selected_;
        const QColor vpc  = vp_hot ? QColor(0xFF, 0xC2, 0x5C)
                                   : QColor(0xE8, 0x9C, 0x28);
        const QPointF vp =
            project3(scene_->persp_vp_x, 0.0, scene_->persp_vp_y);
        // Eye 点 = VP 正上方 F 高度（光轴竖直向下穿过 VP）
        const QPointF eye = project3(scene_->persp_vp_x,
                                     scene_->persp_focal,
                                     scene_->persp_vp_y);
        p.setPen(QPen(vpc, 1, Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        p.drawLine(vp, eye); // 光轴（VP→Eye 竖直连线）
        p.setPen(QPen(vpc, vp_hot ? 2.0 : 1.4));
        p.setBrush(QColor(0xE8, 0x9C, 0x28, 40));
        p.drawEllipse(vp, 6.0, 6.0);
        p.setBrush(Qt::NoBrush);
        p.drawLine(vp + QPointF(-11, 0), vp + QPointF(-4, 0));
        p.drawLine(vp + QPointF(4, 0), vp + QPointF(11, 0));
        p.drawLine(vp + QPointF(0, -11), vp + QPointF(0, -4));
        p.drawLine(vp + QPointF(0, 4), vp + QPointF(0, 11));
        p.drawText(vp + QPointF(10, -8), QStringLiteral("VP"));
        // Eye 环（区别于 VP 十字；半径随 F 轻微呼吸——高度感）
        const double er = 6.0 + std::min(6.0, scene_->persp_focal / 20000.0);
        p.setPen(QPen(vpc, vp_selected_ ? 2.0 : 1.4));
        p.drawEllipse(eye, er, er);
        p.setBrush(vpc);
        p.drawEllipse(eye, 1.8, 1.8); // 中心实点
        p.setBrush(Qt::NoBrush);
        p.drawText(eye + QPointF(10, -8), QStringLiteral("Eye"));
    }

    // ---- 6c. 移动 Gizmo（Blender 式三轴箭头；选中对象锚点，恒最上层）----
    if (sel_.kind != 0) drawMoveGizmo(p);
    drawVpGizmo(p); // VP 三轴（选中 VP 且 F>0 时；蓝 Z = Eye 高度 F）

    // ---- 7. HUD（左上操作提示；右上相机读数与轴球已屏蔽——
    //      2026-11-20 2D 化：视角恒正俯视，无旋转语义可读）--------------------
    p.setPen(QColor(0xB8, 0xB8, 0xB8));
    {
        QFont f = p.font();
        f.setPointSize(8);
        p.setFont(f);
    }
    p.drawText(QRectF(8, 6, width() - 16, 16), Qt::AlignLeft | Qt::AlignTop,
               "滚轮缩放 · 左键空白/中键平移 · 双击空白复位");
}

// ---------------------------------------------------------------------------
// composeGpuFrame —— GL 直通合成（GpuDeliver 交付帧 → 本 widget FBO）
//
// 坐标链（与 QPainter 软路径同一映射基准，所见即所得不漂移）：
//   绘制项坐标（合成域 0..comp_w/comp_h）
//     → 画布坐标（域原点 + 合成坐标×域缩放 sx/sy）
//     → 控件坐标（canvasRect fit×zoom+pan）
//     → FBO 设备像素（×dpr；GL 原点在左下 → y 翻转）。
// 实现上不逐项换算：GlRenderer(ExternalCtx) 消费合成域坐标的 DrawItem，
// 只需把视口定位到"内容域映射矩形"的 FBO 设备像素位置（左下原点换算），
// 投影 u_outSize=(comp_w,comp_h) 保持解算坐标域——数学与 display 端
// GL 后端逐位一致（同一 buildVerts/drawQuad 路径）。
// ---------------------------------------------------------------------------
bool SceneCanvasWidget::composeGpuFrame(
        const std::vector<PreviewRenderer::DeliverItem>& items,
        int comp_w, int comp_h, const QRectF& domain) {
    if (items.empty() || comp_w <= 0 || comp_h <= 0 ||
        domain.width() <= 0 || domain.height() <= 0)
        return false;

    // ---- 惰性建 GL 合成器（必须上下文 current——paintGL 内成立）---------
    if (!gl_) {
        // 【第三步 2026-11-05】传入 CUDA 0 拷贝协商旗标（与解码源共享同一
        // shared_ptr——PreviewRenderer::gpuDeliveryFlag；解码端发布
        // GpuCudaNv12 帧、渲染端 interop 探测失败翻 false 自动回退下载）
        gl_ = std::make_unique<GlRenderer>(
            GlRenderer::ExternalCtxTag{},
            preview_ ? preview_->gpuDeliveryFlag() : nullptr);
        std::fprintf(stderr, "canvas: 预览 GL 合成器创建（ExternalCtx）\n");
        std::fflush(stderr);
    }
    if (gl_->failed()) return false; // 初始化失败：调用方走 QPainter 回退

    // ---- 内容域映射矩形（控件逻辑坐标 → FBO 设备像素，左下原点）---------
    const QRectF cr  = canvasRect();
    if (scene_->canvas_w <= 0 || scene_->canvas_h <= 0) return false;
    const double k   = cr.width() / scene_->canvas_w; // 控件像素/画布单位
    const double dpr = devicePixelRatioF();
    // 目标矩形（控件坐标，top-down）：与软路径贴图同一公式
    const double tx = cr.left() + domain.x() * k;
    const double ty = cr.top()  + domain.y() * k;
    const int fbo_w = static_cast<int>(std::lround(width()  * dpr));
    const int fbo_h = static_cast<int>(std::lround(height() * dpr));
    const int vx    = static_cast<int>(std::lround(tx * dpr));
    const int vy    = fbo_h - static_cast<int>(std::lround(ty * dpr)) -
                      static_cast<int>(std::lround(domain.height() * k * dpr));
    const int vw    = static_cast<int>(std::lround(domain.width()  * k * dpr));
    const int vh    = static_cast<int>(std::lround(domain.height() * k * dpr));
    if (vw <= 0 || vh <= 0) return false;

    // 帧视口（完整矩形）：合成内容贴进 FBO 的内容域位置，其余区域保持
    // 清屏黑（【修复 2026-11-05】尺寸必须显式传——旧版 beginFrame 内部
    // 恒用 comp 尺寸当视口，dom 映射矩形 ≠ comp 尺寸时内容被压缩偏移）
    gl_->setExternalViewport(vx, vy, vw, vh);
    gl_->beginFrame(comp_w, comp_h); // 清屏 + 帧状态（视口=子矩形）
    for (const auto& it : items) {
        if (it.frame.isNull() && !it.native && !it.dxt) continue; // 三形态域防御
        FrameView fv;
        if (it.native) {
            // ---- GPU 域（③阶段 cuda 硬解 0 拷贝；2026-11-05 第三步）----
            // AVFrame 保活引用直透 draw()——interop D2D 进纹理，零下载。
            // 尺寸标量来自交付（buildVerts 的 su/sv 比例需要）；stride/
            // rgba 不参与 GPU 路径（GlRenderer 内部取 AVFrame linesize）。
            fv.domain = FrameDomain::GpuCudaNv12;
            fv.native = it.native;
            fv.width  = it.gpu_w;
            fv.height = it.gpu_h;
        } else if (deliverFrameView(it, fv)) {
            // DXT/CPU 双域（deliverFrameView 统一填充——屏面片路径同源）
        } else {
            continue; // 帧空且无压缩块（防御，理论不可达）
        }
        gl_->draw(it.item, fv);
    }
    {
        // 【诊断 MS_CANVAS_DBG=1】compose 状态流（限频；GL 错误位由
        // GlRenderer::endFrame 内部检查——本 TU 不碰 GL 头）
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg) {
            static int s_cnt = 0;
            if (++s_cnt % 30 == 1)
                std::fprintf(stderr,
                             "canvas-dbg: compose#%d items=%zu "
                             "vp=(%d,%d %dx%d) comp=%dx%d fbo=%dx%d "
                             "dpr=%.2f dom=(%.0f,%.0f %.0fx%.0f)\n",
                             s_cnt, items.size(), vx, vy, vw, vh,
                             comp_w, comp_h, fbo_w, fbo_h, dpr,
                             domain.x(), domain.y(),
                             domain.width(), domain.height());
            std::fflush(stderr);
        }
    }
    gl_->endFrame(); // ExternalCtx：仅 glFlush（呈现归 Qt 合成器）
    return true;
}

void SceneCanvasWidget::drawScreen(QPainter& p, int i, bool selected,
                                   bool show_body) const {
    const QRectF r = screenRect(i);
    if (!r.isValid()) return;
    const Screen& s = scene_->screens[i];

    // 3D 姿态屏（俯仰/偏航非 0）：透视四边形取景框（唯一真相
    // CanvasGeometry::quadCorners3DP + Scene::effectivePersp 透视压平
    // ——2026-11-20 与素材旋转模式统一：旋转后压到 XY 平面的映射，
    // 2D 视口/3D 视口/ScreenMapper 屏姿态单应全部同源同参序）
    const bool posed = (s.rot_x != 0.0 || s.rot_y != 0.0);
    QPolygonF poly; // 控件域四角（姿态分支；序 LT RT RB LB）
    if (posed) {
        double ef = 0.0, evx = 0.0, evy = 0.0;
        scene_->effectivePersp(ef, evx, evy);
        double wx[4], wy[4];
        cg::quadCorners3DP(s.x, s.y, s.w, s.h, 0.0, s.rotation, s.rot_x,
                           s.rot_y, 0, nullptr, ef, evx, evy, wx, wy,
                           nullptr);
        for (int k = 0; k < 4; ++k)
            poly << canvasToWidget(QPointF(wx[k], wy[k]));
    }

    // ---- 屏体：绕中心顺时针旋转绘制（screen.rotation 语义；Qt y 向下
    //      坐标系中正角度即顺时针，与画布语义一致）；姿态屏画透视四边形 ----
    p.save();
    p.setBrush(show_body ? QBrush(QColor(0x14, 0x14, 0x14)) : Qt::NoBrush);
    p.setPen(QPen(selected ? QColor(0xE8, 0xE8, 0xE8) : QColor(0x8A, 0x8A, 0x8A),
                  selected ? 1.8 : 1.0));
    if (posed) {
        p.drawPolygon(poly);
    } else if (s.rotation != 0.0) {
        p.translate(r.center());
        p.rotate(s.rotation);
        p.drawRect(QRectF(-r.width() / 2.0, -r.height() / 2.0,
                          r.width(), r.height()));
    } else {
        p.drawRect(r);
    }
    p.restore();

    // 标签："id → player"（AABB 左上角，restore 后画保持水平可读——
    // 标签是编辑器标注而非屏内容，不随屏旋转；姿态屏取四角 AABB）。
    // 【连接测试联动】已绑定的屏在标签行画在线圆点：●绿 = TCP 在线、
    // ●红 = 离线（状态来自 display_online_，EditorWindow 周期推送）——
    // 未绑定显示端的屏不画圆点（无从谈起在线与否）。
    const QRectF label_r = posed ? poly.boundingRect() : r;
    QFont f = p.font();
    f.setPointSize(9);
    p.setFont(f);
    int label_left = 6; // 标签左内边距（有圆点时右移让位）
    bool online = false; // 中线名随连接状态变色（16:44 用户要求）
    if (!s.player_id.empty()) {
        online = display_online_.count(s.player_id) != 0 &&
                 display_online_.at(s.player_id);
        p.setPen(Qt::NoPen);
        p.setBrush(online ? QBrush(QColor(0x33, 0xB2, 0x6E))
                          : QBrush(QColor(0xE0, 0x55, 0x55)));
        p.drawEllipse(QPointF(label_r.left() + 9.0, label_r.top() + 9.0),
                      3.0, 3.0);
        p.setPen(QColor(0xC8, 0xC8, 0xC8)); // 恢复标签文字色
        p.setBrush(Qt::NoBrush);
        label_left = 17; // 让位圆点（9px 圆心 + 3px 半径 + 余量）
    }
    const QString label =
        QString("%1 → %2")
            .arg(QString::fromStdString(s.id),
                 s.player_id.empty() ? QString("(未绑定)")
                                     : QString::fromStdString(s.player_id));
    // 【2026-11-25 选中指示】选中屏标签红底白字——多屏重叠时与选中框
    // 加粗、选中屏压顶重绘三重确认"选中了谁"
    if (selected) {
        const QRectF text_r =
            QRectF(label_r.left() + label_left - 2.0, label_r.top() + 3.0,
                   QFontMetrics(f).horizontalAdvance(label) + 10.0, 16.0);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xFF, 0x33, 0x33));
        p.drawRoundedRect(text_r, 3, 3);
        p.setPen(Qt::white);
        p.setBrush(Qt::NoBrush);
        p.drawText(text_r, Qt::AlignCenter, label);

        // 【2026-11-25 选中中线名】屏中央加粗名字——左上角小标签在多屏/
        // 远距时不够醒目，中线大字一眼确认选中了谁（统一入口 drawCenterName）
        drawCenterName(p, label_r, s.player_id, online);
    } else {
        p.drawText(label_r.adjusted(label_left, 4, -6, 0),
                   Qt::AlignLeft | Qt::AlignTop, label);
    }

    if (selected) { // 选中标注：随旋转的虚线框 + 四角手柄 + 旋转拖柄
        if (posed) {
            // 姿态屏：红虚线沿透视四边形；缩放/旋转柄隐藏（透视四边形
            // 无法用矩形柄编辑——同 drawClip 姿态分支口径；拖动仍走
            // geo_backup 会话平移 s.x/s.y 不受影响）。2.4px 同
            // drawSelectionBox（2026-11-25 重叠可见性加粗）。
            p.setPen(QPen(QColor(0xFF, 0x33, 0x33), 2.4, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawPolygon(poly);
        } else {
            drawSelectionBox(p, r, s.rotation);
            // 【画布域入参】手柄/拖柄内部对角点做 canvasToWidget 换算（与
            // 命中测试 selectionCanvasRect 同一坐标系）——此前误传控件域 r
            // 导致双重换算：画布非 1:1 缩放时柄飞离对象（红框在对象上、柄
            // 漂浮在别处）；1:1 时映射恒等故长期未暴露。
            drawResizeHandles(p, QRectF(s.x, s.y, s.w, s.h), s.rotation);
            drawRotateHandle(p, QRectF(s.x, s.y, s.w, s.h), s.rotation);
        }
    }
}

void SceneCanvasWidget::drawClip(QPainter& p, int tr, int cl,
                                 bool selected, bool show_checker) const {
    const QRectF r = clipRect(tr, cl);
    if (!r.isValid()) return;
    const Clip& c = scene_->timeline.tracks[tr].clips[cl];
    // 【音频/Control 片段不画】无图像数据（WATCHOUT Stage 语义：声音对象
    // /指令片段不占视觉位）——连"方框+棋盘格"占位、标签与选中标注一并
    // 跳过，画布上完全隐形（命中测试同口径跳过，防止隐形框挡住别的对象）。
    // 【2026-11-15 片段类型感知】判定跟片段类型串走（轨道 kind 仅兜底）；
    // Artnet/Mapping 同口径隐形。
    if (trackInvisibleOnCanvas(scene_->timeline.tracks[tr].kind, c.type))
        return;
    const Transform tf = clipResolvedTransform(c); // tween 求值统一入口

    // 【诊断 MS_CANVAS_DBG=1】标注框控件坐标 dump（与 GL quad 对照——
    // 闭合 comp→FBO→控件换算链；四角 = r 绕中心转 rotation）
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg) {
            static int s_cnt = 0;
            if (++s_cnt <= 12) {
                const double rad = tf.rotation * 3.14159265358979323846 / 180.0;
                const double ct = std::cos(rad), stn = std::sin(rad);
                const QPointF ctr = r.center();
                const double hw = r.width() / 2.0, hh = r.height() / 2.0;
                std::fprintf(stderr,
                             "clip-dbg: drawClip#%d id=%s r=(%.1f,%.1f %.1fx%.1f) "
                             "rot=%.1f corners=", s_cnt, c.id.c_str(),
                             r.x(), r.y(), r.width(), r.height(),
                             tf.rotation);
                for (int i = 0; i < 4; ++i) {
                    const double lx = (i & 1) ? hw : -hw;
                    const double ly = (i & 2) ? hh : -hh;
                    std::fprintf(stderr, "(%.1f,%.1f)",
                                 ctr.x() + ct * lx - stn * ly,
                                 ctr.y() + stn * lx + ct * ly);
                }
                const QRectF cr = canvasRect();
                std::fprintf(stderr, " canvasRect=(%.1f,%.1f %.1fx%.1f)\n",
                             cr.x(), cr.y(), cr.width(), cr.height());
                std::fflush(stderr);
            }
        }
    }

    // 边框色 = 轨道类型色（与时间轴插件模型的类型色同一组，视觉分组统一：
    // 色源在媒体插件，画布内经 trackKindColor 单点维护——改动时与插件同步）
    const QColor border = trackKindColor(scene_->timeline.tracks[tr].kind);

    // 变形片段（2026-11-09 锚点/角点；2026-11-10 姿态 3D；2026-11-17 阶段③
    // 透视；2026-11-20 修订恢复透视——正交扁平化曾短暂替代，用户目验旋转
    // 观感后拍板透视）：四角 = 绕锚点旋转 + 位移（2D）或 quadCorners3DP
    // 透视预投影（3D 位姿——含滚转/锚点/角位移/深度/全场透视，与
    // ScreenMapper 预投影同式同参序）——画布域计算 → 控件域换算，本体
    // 绘制走多边形；默认中心锚无姿态无位移 = 历史路径逐位不变。
    const bool posed = tf.pose3DActive();
    const bool deformed = posed || tf.anchor != 0 || tf.cornerActive();
    QPolygonF poly; // 控件域四角（变形分支；序 LT RT RB LB）
    if (deformed) {
        const QRectF cr = clipCanvasRect(c); // 画布域（角点数学所在域）
        double wx[4], wy[4];
        if (posed) {
            // 3D 位姿：quadCorners3DP（唯一真相 CanvasGeometry
            // ——含深度/全场透视全部语义）
            double ef = 0.0, evx = 0.0, evy = 0.0;
            scene_->effectivePersp(ef, evx, evy);
            cg::quadCorners3DP(cr.x(), cr.y(), cr.width(), cr.height(),
                               tf.pos_z, tf.rotation, tf.rot_x,
                               tf.rot_y, tf.anchor,
                               tf.cornerActive() ? tf.corner : nullptr,
                               ef, evx, evy, wx, wy, nullptr);
        } else {
            QPointF cs[4];
            clipQuadCorners(cr, tf.rotation, tf.anchor,
                            tf.cornerActive() ? tf.corner : nullptr, cs);
            for (int i = 0; i < 4; ++i) {
                wx[i] = cs[i].x();
                wy[i] = cs[i].y();
            }
        }
        for (int i = 0; i < 4; ++i) poly << canvasToWidget(QPointF(wx[i], wy[i]));
    }

    p.save();
    // 旋转：绕中心顺时针（transform.rotation 语义；Qt 正角度即顺时针）。
    // 棋盘格与边框随片段旋转；标签在 restore 后画（水平可读）
    if (deformed) {
        // 变形：棋盘格裁进四边形 + 多边形边框
        if (show_checker) {
            QPainterPath clip_path;
            clip_path.addPolygon(poly);
            clip_path.closeSubpath();
            p.setClipPath(clip_path);
            drawChecker(p, poly.boundingRect());
        }
        p.setPen(QPen(border, selected ? 2.2 : 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(poly);
    } else if (tf.rotation != 0.0) {
        p.translate(r.center());
        p.rotate(tf.rotation);
        const QRectF local(-r.width() / 2.0, -r.height() / 2.0,
                           r.width(), r.height());
        // 棋盘格仅在占位模式画（真实帧模式下片段画面已含在合成帧内，
        // 这里只画边框 + 标签标注片段边界）
        if (show_checker) drawChecker(p, local);
        p.setPen(QPen(border, selected ? 2.2 : 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(local);
    } else {
        if (show_checker) drawChecker(p, r);
        p.setPen(QPen(border, selected ? 2.2 : 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);
    }
    p.restore();

    // 标签：片段 id（AABB 左上角，水平可读；变形片段取位移后四角 AABB）
    p.setPen(QColor(0xE8, 0xE8, 0xE8));
    QFont f = p.font();
    f.setPointSize(8);
    p.setFont(f);
    const QRectF label_r = deformed ? poly.boundingRect() : r;
    p.drawText(label_r.adjusted(5, 3, -4, 0), Qt::AlignLeft | Qt::AlignTop,
               QString::fromStdString(c.id));

    if (selected) { // 选中标注：素材手柄（2026-11-20 统一交互优化）
        // 视觉四角/边线 = clipVisualQuadCanvas 透视投影（2026-11-20 定案；
        // 与画面同源不分离——变形片段手柄跟随位移四角）：四角 = 角点位移
        // 拖拽、四边 = 尺寸缩放（实线可抓）。原「红虚线+隐藏手柄」与
        // 「标称虚线框+矩形缩放柄」两条分叉在此收敛为一条。
        QPolygonF quad_c = clipVisualQuadCanvas(c, tf);
        QPolygonF quad_w;
        quad_w.reserve(4);
        for (int i = 0; i < 4; ++i) quad_w << canvasToWidget(quad_c[i]);
        drawClipHandles(p, quad_w);
        if (!deformed) { // 旋转柄仅限未变形矩形（变形四边形上柄位未定义
                         // ——姿态编辑走属性面板）
            drawRotateHandle(p, clipCanvasRect(c), tf.rotation);
        }
    }
}

void SceneCanvasWidget::drawChecker(QPainter& p, const QRectF& r) const {
    // 棋盘格（透明纹理的行业画法）：底色格 + 深色格交错，裁剪进矩形
    const double cell = 12.0;
    p.save();
    p.setClipRect(r);
    p.fillRect(r, QColor(0x21, 0x21, 0x21));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x2E, 0x2E, 0x2E));
    const int cols = (int)std::ceil(r.width() / cell);
    const int rows = (int)std::ceil(r.height() / cell);
    for (int i = 0; i < cols; ++i)
        for (int j = 0; j < rows; ++j) {
            if (((i + j) & 1) == 0) continue; // 只画深色格（浅格为底色）
            p.drawRect(QRectF(r.left() + i * cell, r.top() + j * cell,
                              cell, cell));
        }
    p.restore();
}

void SceneCanvasWidget::drawSelectionBox(QPainter& p, const QRectF& r,
                                         double rotation_deg) const {
    p.save();
    // 红虚线 2.4px（2026-11-25 加粗自 1.6——多屏重叠时弱线混入普通框）
    p.setPen(QPen(QColor(0xFF, 0x33, 0x33), 2.4, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    // 外扩避免与内容边框重叠；旋转对象绕中心同步旋转（框随姿态一致，
    // 与对象旋转绘制 drawScreen/drawClip 同一变换）
    const QRectF box = r.adjusted(-2.5, -2.5, 2.5, 2.5);
    if (rotation_deg != 0.0) {
        p.translate(box.center());
        p.rotate(rotation_deg);
        p.drawRect(QRectF(-box.width() / 2.0, -box.height() / 2.0,
                          box.width(), box.height()));
    } else {
        p.drawRect(box);
    }
    p.restore();
}

void SceneCanvasWidget::drawResizeHandles(QPainter& p, const QRectF& r,
                                          double rotation_deg,
                                          double plane_h) const {
    // 四角实心方块（白底黑描边，WATCHOUT 风格）：角点取旋转后画布位置
    // （CanvasGeometry::rotatedCorner），绘制在控件域、不随对象旋转倾斜。
    // plane_h：3D 模式下手柄在对象高度平面上投影（2D 恒 0 不参与换算）
    const double hs = 3.0; // 半边长（6px 方块）
    for (int corner = 0; corner < 4; ++corner) {
        double cx = 0, cy = 0;
        cg::rotatedCorner(r.x(), r.y(), r.width(), r.height(), rotation_deg,
                          corner, &cx, &cy);
        const QPointF wp = canvasToWidgetAt(QPointF(cx, cy), plane_h);
        const QRectF h(wp.x() - hs, wp.y() - hs, hs * 2, hs * 2);
        p.fillRect(h, QColor(0xFF, 0xFF, 0xFF));
        p.setPen(QPen(QColor(0x00, 0x00, 0x00), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(h);
    }
}

void SceneCanvasWidget::drawRotateHandle(QPainter& p, const QRectF& r,
                                         double rotation_deg,
                                         double plane_h) const {
    // 旋转拖柄（WATCHOUT 风格）：顶边中点 → 外延圆点连线 + 4px 白圆。
    // 外延量控件像素恒定（22px）→ 换算画布单位；柄位置随对象旋转。
    // 3D：柄位置在对象高度平面上投影（plane_h）
    if (r.width() <= 0 || r.height() <= 0) return;
    const double k = view3d_ ? cam3d_.scale
                             : (canvasRect().width() /
                                (scene_ && scene_->canvas_w > 0
                                     ? scene_->canvas_w : 1.0));
    const double offset_canvas = (k > 0 ? 22.0 / k : 22.0);
    double hx = 0, hy = 0, tx = 0, ty = 0;
    cg::rotateHandlePos(r.x(), r.y(), r.width(), r.height(), rotation_deg,
                        offset_canvas, &hx, &hy);
    cg::rotateHandlePos(r.x(), r.y(), r.width(), r.height(), rotation_deg, 0.0,
                        &tx, &ty); // 连线起点 = 顶边中点
    const QPointF tw = canvasToWidgetAt(QPointF(tx, ty), plane_h);
    const QPointF hw = canvasToWidgetAt(QPointF(hx, hy), plane_h);
    p.save();
    p.setPen(QPen(QColor(0x00, 0x00, 0x00), 1.2)); // 连线（黑，醒目）
    p.drawLine(tw, hw);
    p.setPen(QPen(QColor(0x00, 0x00, 0x00), 1));
    p.setBrush(QBrush(QColor(0xFF, 0xFF, 0xFF)));
    p.drawEllipse(hw, 4.0, 4.0); // 圆柄（半径 4px）
    p.restore();
}

// ---------------------------------------------------------------------------
// 选中中线名（2026-11-25，双绘制路径统一入口）：取景框中央 11pt 加粗
// display 名。只显 player_id（"scr-N →" 取景窗内部 id 对识别 display 无
// 意义，用户定案）；未绑定显 "(未绑定)"。上移 16px 错开中央移动 Gizmo
//（gizmo 在 6c 段绘制恒压名上，只能挪名字）；柔和黑影保对比度（取景框
// 无填充、下方是画面内容）；在线=绿 / 离线·未绑定=红（与在线圆点同
// 事实源 display_online_）。容不下时 ElideMiddle 中部省略。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::drawCenterName(QPainter& p, const QRectF& box,
                                       const std::string& player_id,
                                       bool online) const {
    if (box.width() <= 0 || box.height() <= 0) return;
    QFont cf = p.font();
    cf.setPointSize(11);
    cf.setBold(true);
    const QFontMetrics cfm(cf);
    const QString name = cfm.elidedText(
        player_id.empty() ? QString("(未绑定)")
                          : QString::fromStdString(player_id),
        Qt::ElideMiddle,
        qMax(0, static_cast<int>(box.width()) - 12));
    if (name.isEmpty()) return;
    p.setFont(cf);
    const QRectF name_r = box.translated(0, -16.0); // 错开中央 Gizmo
    p.setPen(QPen(QColor(0, 0, 0, 150)));           // 柔和黑影
    p.drawText(name_r.translated(1.2, 1.2), Qt::AlignCenter, name);
    p.setPen(QPen(online ? Qt::green : Qt::red));
    p.drawText(name_r, Qt::AlignCenter, name);
}

// ---------------------------------------------------------------------------
// 鼠标交互
// 3D 模式：右键 = orbit 旋转会话（真位移旋转视角；无位移松开 = 菜单，
// contextMenuEvent 由 suppress_menu_ 旗标协调——Windows 右键松开即发菜单
// 事件）；中键/左键空白 = 平移（光标下世界点跟随光标，锚定闭式解）。
// 2D 模式交互骨架逐字节保留（仅坐标换算分发到 canvasFromScreen）。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::mousePressEvent(QMouseEvent* ev) {
    // ---- 右键：轨道旋转已屏蔽（2026-11-20 2D 化——视角恒正俯视锁定）；
    //      右键仅弹菜单（contextMenuEvent），按下不启动任何视图会话 ----
    // ---- 中键平移会话（视图浏览；优先于一切对象交互——平移不改场景）----
    if (ev->button() == Qt::MiddleButton && scene_) {
        panning_          = true;
        pan_start_widget_ = ev->position();
        if (view3d_)
            pan_anchor_world_ = widgetToCanvasAt(ev->position(), 0.0);
        else
            pan_start_view_ = view_pan_;
        setCursor(Qt::ClosedHandCursor);
        ev->accept();
        return;
    }
    if (ev->button() != Qt::LeftButton || !scene_) {
        QWidget::mousePressEvent(ev);
        return;
    }

    // ---- 撤销会话开启（2026-11-26）：左键按下即冻结 before 基线——覆盖
    //      全部左键会话（对象拖动/角点/边线/旋转/Gizmo/VP/空白平移）。
    //      松手 endEdit 统一收口；无位移会话（单击选中/平移）由
    //      SceneHistory 去重丢弃。平移会话也开（含中键分支之外的左键空白
    //      平移）——空会话零成本丢弃，换取插桩单点全覆盖。----
    if (hist_) hist_->beginEdit("画布编辑", *scene_);
    const QPointF wp = ev->position();

    // ---- 3D 轴球视角控件：已屏蔽（2026-11-20 2D 化——右上角角标退役，
    //      端点跳视角/球内拖拽 orbit 一并停用；drawAxisGizmo/hitAxisGizmo
    //      函数保留供回退）--------------------------------------------------

    QRectF sel_rect;
    double sel_rot = 0.0;

    // ---- 全局消隐点 VP（3D，优先于对象交互——地面标记易被覆盖）：
    //      点 VP 十字/Eye 环 = 选中 VP（清对象选中）+ 即进自由拖会话；
    //      已选中时压 Gizmo 轴（含蓝 Z=Eye 高度 F）= 单轴约束会话。
    //      实时改 persp_vp_x/y（蓝轴改 persp_focal）--------------------
    if (view3d_ && scene_ && scene_->persp_focal > 0.0) {
        const int vaxis = hitVpGizmo(wp); // 前提=vp_selected_（内部判）
        const bool vp_center = vpHandleHit(wp);
        const bool vp_eye    = eyeHandleHit(wp);
        if (vaxis >= 0) {
            vp_drag_axis_    = vaxis;
            vp_orig_x_       = scene_->persp_vp_x;
            vp_orig_y_       = scene_->persp_vp_y;
            vp_orig_f_       = scene_->persp_focal;
            gizmo_press_widget_ = wp; // 复用会话按下点（对象 Gizmo 互斥）
            gizmo_axis_scr_  = moveGizmoAxisVec(
                vaxis, scene_->persp_vp_x, 0.0, scene_->persp_vp_y);
            dragging_vp_ = true;
            moved_       = false;
            setCursor(Qt::ClosedHandCursor);
            statusTip(vaxis == 2 ? "沿蓝 Z 轴拖动 Eye 高度（焦距 F）"
                     : vaxis == 0 ? "沿红 X 轴拖动全局消隐点（VP）"
                                  : "沿绿 Y 轴拖动全局消隐点（VP）");
            update();
            ev->accept();
            return;
        }
        if (vp_center || vp_eye) {
            // 单击 VP/Eye = 选中（Gizmo 显示）+ 自由拖会话一步到位
            if (!vp_selected_) {
                sel_ = Selection{}; // VP 选中让位对象选中（互斥单选）
                vp_selected_ = true;
                update(); // Gizmo/VP 标记高亮立即呈现
            }
            dragging_vp_ = true;
            vp_drag_axis_ = -1; // 中心圈/Eye 自由拖
            moved_       = false;
            setCursor(Qt::ClosedHandCursor);
            statusTip("已选中 VP（拖动自由移动；Shift 吸附 10px）");
            ev->accept();
            return;
        }
    }

    // ---- 移动 Gizmo 三轴手柄（3D，Blender 式；优先于对象整体拖动）：
    //      命中轴 = 单轴约束会话——屏幕 delta 在轴方向上的投影 → 世界
    //      位移（轴向可视化位移的主入口；拖中心区域/对象本体 = 自由拖
    //      走既有路径）------------------------------------------------
    if (view3d_ && sel_.kind != 0) {
        const int axis = hitMoveGizmo(wp);
        if (axis >= 0) {
            double cx = 0, gh = 0, cy = 0;
            moveGizmoAnchor(cx, gh, cy); // 命中前提=锚点有效
            gizmo_drag_axis_    = axis;
            gizmo_press_widget_ = wp;
            gizmo_axis_scr_     = moveGizmoAxisVec(axis, cx, gh, cy);
            if (sel_.kind == 1) {
                const Screen& s = scene_->screens[sel_.a];
                drag_orig_x_ = s.x;
                drag_orig_y_ = s.y;
            } else {
                const Transform rt = clipResolvedTransform(
                    scene_->timeline.tracks[sel_.a].clips[sel_.b]);
                drag_orig_x_ = rt.x;
                drag_orig_y_ = rt.y;
                gizmo_orig_z_ = rt.pos_z; // 高度轴起始（tween 基准求值）
            }
            drag_plane_h_  = selectionPlaneH();
            dragging_      = true;
            rotating_      = false;
            resizing_      = false;
            resize_corner_ = -1;
            moved_         = false;
            guide_x_.clear();
            guide_y_.clear();
            guide_pt_valid_ = false;
            setCursor(Qt::ClosedHandCursor);
            statusTip(axis == 2 ? "沿高度轴（蓝 Z）拖动 pos_z"
                     : axis == 0 ? "沿红 X 轴拖动画布 X"
                                 : "沿绿 Y 轴拖动画布 Y");
            update();
            ev->accept();
            return;
        }
    }

    // ---- 旋转拖柄最优先：柄在对象"头顶"外延处，压柄按下必进旋转会话
    //      （先于角点手柄与 hitTest——三者互斥，柄离角点更近易误触）----
    if (hitRotateHandle(wp) && selectionCanvasRect(&sel_rect, &sel_rot)) {
        drag_plane_h_ = selectionPlaneH(); // 3D：会话冻结在对象高度平面
        dragging_ = true;
        rotating_ = true;
        resize_corner_  = -1;
        resizing_       = false;
        resize_rotation_ = sel_rot;
        rotate_orig_rot_ = sel_rot; // 起始 rotation（增量基准）
        {
            const QPointF cp = canvasFromScreen(wp);
            rotate_start_ang_ = cg::angleFromCenter(sel_rect.x(), sel_rect.y(),
                                                    sel_rect.width(),
                                                    sel_rect.height(),
                                                    cp.x(), cp.y());
        }
        drag_start_canvas_ = canvasFromScreen(wp);
        moved_ = false;
        update();
        statusTip("拖动旋转（Shift 吸附 15°）");
        return;
    }

    // ---- 素材角点/边线手柄（2026-11-20 交互优化）：片段四角 = 角点位
    //      移拖拽（corner pin）、四边 = 尺寸缩放（对边视觉固定）——先于
    //      hitTest，压柄按下必进专用会话（素材矩形缩放柄已退役：
    //      hitResizeHandle 对 clip 恒 -1）--------------------------------
    if (sel_.kind == 4) {
        const int chit = hitClipCornerHandle(wp);
        const int ehit = chit < 0 ? hitClipEdgeHandle(wp) : -1;
        if (chit >= 0 || ehit >= 0) {
            auto& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
            if (clipActiveAtPlayhead(c)) {
                const Transform tf = clipResolvedTransform(c);
                const QRectF cr = clipCanvasRect(c);
                if (chit >= 0) {
                    corner_drag_ = chit;
                    edge_drag_   = -1;
                } else {
                    corner_drag_ = -1;
                    edge_drag_   = ehit;
                    // 会话冻结：标称矩形（画布域，含 auto-size 兜底宽高）
                    // + 姿态 + 锚点——tick 全程以冻结参数解算
                    edge_orig_x_ = cr.x();
                    edge_orig_y_ = cr.y();
                    edge_orig_w_ = cr.width();
                    edge_orig_h_ = cr.height();
                    edge_orig_rot_    = tf.rotation;
                    edge_orig_rx_     = tf.rot_x;
                    edge_orig_ry_     = tf.rot_y;
                    edge_orig_anchor_ = tf.anchor;
                    // 局部轴单位向量（取自当前视觉四角——投影方向与可
                    // 见边一致）与对边中点（回贴锚）
                    double ef = 0.0, evx = 0.0, evy = 0.0;
                    scene_->effectivePersp(ef, evx, evy);
                    double vx[4], vy[4];
                    cg::quadCorners3DP(
                        cr.x(), cr.y(), cr.width(), cr.height(), tf.pos_z,
                        tf.rotation, tf.rot_x, tf.rot_y, tf.anchor,
                        tf.cornerActive() ? tf.corner : nullptr,
                        ef, evx, evy, vx, vy, nullptr);
                    const QPointF c0(vx[0], vy[0]), c1(vx[1], vy[1]),
                        c2(vx[2], vy[2]), c3(vx[3], vy[3]);
                    edge_dir_x_ = c1 - c0; // 顶边方向 = 局部 x 轴
                    edge_dir_y_ = c3 - c0; // 左边方向 = 局部 y 轴（向下）
                    const double lenx = std::hypot(edge_dir_x_.x(),
                                                   edge_dir_x_.y());
                    const double leny = std::hypot(edge_dir_y_.x(),
                                                   edge_dir_y_.y());
                    if (lenx > 1e-9) edge_dir_x_ /= lenx;
                    else edge_dir_x_ = QPointF(1.0, 0.0);
                    if (leny > 1e-9) edge_dir_y_ /= leny;
                    else edge_dir_y_ = QPointF(0.0, 1.0);
                    switch (ehit) { // 对边中点（画布域，冻结）
                        case 0: edge_opp_mid_ = (c3 + c2) / 2.0; break;
                        case 1: edge_opp_mid_ = (c0 + c3) / 2.0; break;
                        case 2: edge_opp_mid_ = (c0 + c1) / 2.0; break;
                        default: edge_opp_mid_ = (c1 + c2) / 2.0; break;
                    }
                }
                drag_plane_h_    = selectionPlaneH();
                dragging_        = true;
                rotating_        = false;
                resizing_        = false;
                resize_corner_   = -1;
                gizmo_drag_axis_ = -1;
                drag_start_canvas_ = canvasFromScreen(wp);
                moved_ = false;
                collectSnapReferences(); // 吸附参考集合（角点/边线会话同用）
                guide_x_.clear();
                guide_y_.clear();
                guide_pt_valid_ = false;
                update();
                statusTip(chit >= 0
                              ? "拖动角点位移（与属性面板「形变」联动）"
                              : "拖动边线调整大小（对边固定，与属性面板尺寸/位置联动）");
                ev->accept();
                return;
            }
        }
    }

    // ---- 角点缩放手柄次之：对象已选中时手柄浮于其上，压在手柄按下必须
    //      进入缩放会话（先于 hitTest，避免被判定为"重新选中/拖动"）----
    const int corner = hitResizeHandle(wp);
    if (corner >= 0 && selectionCanvasRect(&sel_rect, &sel_rot) &&
        sel_rect.width() > 0 && sel_rect.height() > 0) {
        drag_plane_h_ = selectionPlaneH(); // 3D：会话冻结在对象高度平面
        dragging_      = true;
        resizing_      = true;
        resize_corner_ = corner;
        resize_rotation_ = sel_rot;          // 会话内冻结旋转角
        resize_orig_x_ = sel_rect.x();       // 会话起始外接框（画布域）
        resize_orig_y_ = sel_rect.y();
        resize_orig_w_ = sel_rect.width();
        resize_orig_h_ = sel_rect.height();
        drag_start_canvas_ = canvasFromScreen(wp);
        moved_ = false; // 新会话：位移标志复位
        update();
        statusTip("拖动角点调整大小（对角固定）");
        return;
    }

    // ---- 常规命中【2026-11-15 选中分工】---------------------------------
    // 画布只选 screen（hitTest screen-only）；时间轴选中的 clip 例外——
    // 点击落在其变形矩形内 = 保持选中直接进拖动会话（"时间轴选谁，画布
    // 摆哪"：换选归时间轴，编辑归画布）。其余情况按 screen 命中处理
    //（选中最上层 screen / 空白清选进平移）。
    // 点击画布对象/空白 = VP Gizmo 一律退位（选中制——VP 自身的选中
    // 在上方 VP 分支处理；不在此处清则 sel_ 已空时 applySelection 提前
    // 返回会残留 VP 选中，点空白 Gizmo 不消失）
    vp_selected_ = false;
    if (!hitSelectedClip(wp))
        applySelection(hitTest(wp, HitPick::kCycle)); // 左键连点=重叠换选
    resizing_ = false; // 未进入缩放会话（命中对象走移动）
    rotating_ = false;
    resize_corner_ = -1;

    dragging_ = false;
    guide_x_.clear(); // 新会话：上一轮吸附参考线作废
    guide_y_.clear();
    guide_pt_valid_ = false;
    if (sel_.kind == 0) {
        // ---- 空白处左键 = 平移会话（中键平移保留）----
        // 空白按下即进入平移会话（光标变抓手），拖动 = 移动视图（不改场景
        // 数据、不算编辑）；单击未动 = 只清空选择（applySelection 已做）。
        panning_          = true;
        pan_start_widget_ = wp;
        if (view3d_)
            pan_anchor_world_ = widgetToCanvasAt(wp, 0.0); // 3D：世界锚点
        else
            pan_start_view_ = view_pan_;
        setCursor(Qt::ClosedHandCursor);
        update();
        statusTip("拖动平移视图（左键空白/中键；双击空白复位）");
        return;
    }
    // 记录拖动基准（画布坐标 + 对象原始 x/y）。3D：反投影平面 = 被拖
    // 对象所在高度平面（片段 pos_z 深度 Z / 屏幕地面）——拖动全程对象
    // 贴着光标所在平面移动，增量语义与 2D 逐位一致
    drag_plane_h_ = selectionPlaneH();
    dragging_ = true;
    moved_    = false; // 新一轮按下：位移标志复位（单击不产生 sceneEdited）
    drag_start_canvas_ = canvasFromScreen(wp);
    collectSnapReferences(); // 吸附参考线集合（拖动期间不变）
    if (sel_.kind == 1) {
        const Screen& s = scene_->screens[sel_.a];
        drag_orig_x_ = s.x;
        drag_orig_y_ = s.y;
        statusTip(QString("已选中屏幕 %1（拖动改位置）")
                               .arg(QString::fromStdString(s.id)));
    } else {
        const auto& c =
            scene_->timeline.tracks[sel_.a].clips[sel_.b];
        // 拖动基值 = 播放头处求值结果（clipResolvedTransform——tween 接管
        // 属性的静态 transform 与画面可能分叉，用静态值起拖会在按下瞬间
        // 跳变到基准位置；2026-11-06 方案 A auto-key 批次修正）
        const Transform rt = clipResolvedTransform(c);
        drag_orig_x_ = rt.x;
        drag_orig_y_ = rt.y;
        // 2D 化（2026-11-20）：位置提示不再带 Z（深度 Z 已屏蔽显示）
        statusTip(QString("已选中片段 %1（拖动改画布位置）")
                      .arg(QString::fromStdString(c.id)));
    }
}

void SceneCanvasWidget::mouseMoveEvent(QMouseEvent* ev) {
    // ---- 3D 轨道会话（右键拖拽）：方位角/俯仰角增量（视图操作）----
    // 俯仰钳制 8°~89°（8° 防地面反投影发散；89° 不越顶防 yaw 翻转跳变）
    if (orbiting_) {
        const QPointF d = ev->position() - orbit_last_pos_;
        orbit_last_pos_ = ev->position();
        if (std::fabs(d.x()) + std::fabs(d.y()) >= 1.0) orbit_moved_ = true;
        cam3d_.yaw_deg =
            std::fmod(cam3d_.yaw_deg + d.x() * 0.3 + 360.0, 360.0);
        cam3d_.pitch_deg =
            std::clamp(cam3d_.pitch_deg - d.y() * 0.3, Camera3D::kPitchMin,
                       Camera3D::kPitchMax);
        update();
        ev->accept();
        return;
    }

    // ---- 平移会话：2D 移 view_pan_；3D 移注视点（世界锚点跟随光标）----
    if (panning_) {
        if (view3d_) {
            panAnchorTo(ev->position(), pan_anchor_world_);
        } else {
            view_pan_ = pan_start_view_ + (ev->position() - pan_start_widget_);
        }
        update();
        ev->accept();
        return;
    }

    // ---- 悬停光标反馈（非拖动时）：旋转柄 → 十字；角点手柄 → 对角缩放。
    //      Qt 无任意角度光标，旋转对象的视觉方向差异在此简化为同一组
    //      对角光标（局部 0/2 ↔ 主对角线、1/3 ↔ 副对角线）----
    if (!dragging_ && !dragging_vp_ && scene_) {
        // 移动 Gizmo/VP Gizmo 悬停（3D；与 press 优先级同序——轴球之后
        // VP 之前；VP 会话中不重复判定免高亮抖动）
        int hg = -1;
        if (view3d_ && sel_.kind != 0) hg = hitMoveGizmo(ev->position());
        int vg = -1;
        if (hg < 0 && view3d_) vg = hitVpGizmo(ev->position());
        if (gizmo_hover_axis_ != hg || vp_hover_axis_ != vg) {
            gizmo_hover_axis_ = hg;
            vp_hover_axis_    = vg;
            update(); // 高亮态重绘（粗杆亮色）
        }
        if (hg >= 0 || vg >= 0) {
            setCursor(Qt::SizeAllCursor); // Gizmo 轴 → 移动光标
        } else if (view3d_ && (vpHandleHit(ev->position()) ||
                                eyeHandleHit(ev->position()))) {
            setCursor(Qt::SizeAllCursor); // VP 手柄/Eye 环 → 移动光标（点击=选中 VP）
        } else if (sel_.kind == 4) {
            // 素材手柄悬停（2026-11-20）：角点 → 对角缩放光标、边线 →
            // 横/纵光标（上/下边竖拖、左/右边横拖）；高亮态变化即重绘
            const int chov = hitClipCornerHandle(ev->position());
            const int ehov =
                chov < 0 ? hitClipEdgeHandle(ev->position()) : -1;
            if (chov != corner_hover_ || ehov != edge_hover_) {
                corner_hover_ = chov;
                edge_hover_   = ehov;
                update(); // 高亮态重绘（蓝亮色）
            }
            if (chov >= 0) {
                setCursor(chov == 0 || chov == 2 ? Qt::SizeFDiagCursor
                                                 : Qt::SizeBDiagCursor);
            } else if (ehov >= 0) {
                setCursor(ehov == 0 || ehov == 2 ? Qt::SizeVerCursor
                                                 : Qt::SizeHorCursor);
            } else if (cursor().shape() != Qt::ArrowCursor) {
                unsetCursor();
            }
        } else if (hitRotateHandle(ev->position())) {
            setCursor(Qt::CrossCursor); // 旋转：十字光标（无原生旋转光标）
        } else {
            const int hc = hitResizeHandle(ev->position());
            if (hc >= 0) {
                setCursor(hc == cg::kCornerTL || hc == cg::kCornerBR
                              ? Qt::SizeFDiagCursor
                              : Qt::SizeBDiagCursor);
            } else if (cursor().shape() != Qt::ArrowCursor) {
                unsetCursor(); // 离开手柄恢复箭头（只在状态变化时调用，免抖动）
            }
        }
    }

    // ---- VP 拖动会话：轴约束（红 X/绿 Y 改 vp_x/y；蓝 Z 改 Eye 高度
    //      F——钳制 ≥1 防 Eye 沉入地面）或自由拖（光标在地面平面的交点
    //      → persp_vp_x/y）；Shift 吸附 10px（仅自由拖）----
    if (dragging_vp_) {
        if (scene_) {
            if (vp_drag_axis_ >= 0) {
                // 单轴约束：与对象 Gizmo 同公式 t=(Δmouse·v)/|v|²
                const QPointF dw = ev->position() - gizmo_press_widget_;
                const double len2 = gizmo_axis_scr_.x() * gizmo_axis_scr_.x() +
                                    gizmo_axis_scr_.y() * gizmo_axis_scr_.y();
                if (len2 > 1e-6) {
                    const double t =
                        QPointF::dotProduct(dw, gizmo_axis_scr_) / len2;
                    if (vp_drag_axis_ == 0)
                        scene_->persp_vp_x = vp_orig_x_ + t;
                    else if (vp_drag_axis_ == 1)
                        scene_->persp_vp_y = vp_orig_y_ + t;
                    else
                        scene_->persp_focal = std::max(1.0, vp_orig_f_ + t);
                    moved_ = true;
                    if (preview_) preview_->invalidate();
                    emit sceneEditedLive();
                }
            } else {
                const QPointF cp = widgetToCanvasAt(ev->position(), 0.0);
                double vx = cp.x(), vy = cp.y();
                if (ev->modifiers() & Qt::ShiftModifier) {
                    vx = std::round(vx / 10.0) * 10.0;
                    vy = std::round(vy / 10.0) * 10.0;
                }
                scene_->persp_vp_x = vx;
                scene_->persp_vp_y = vy;
                moved_ = true;
                if (preview_) preview_->invalidate(); // 透视参数变更重合成
                emit sceneEditedLive();               // display 节流下发
            }
        }
        update();
        ev->accept();
        return;
    }

    // ---- 移动 Gizmo 轴约束会话：屏幕 delta 投影到被拖轴方向 → 世界位移
    //      （t = (Δmouse·v)/|v|²，v = 按下冻结的"世界 1 单位"屏幕向量）。
    //      注：不套自由拖的合成域 s_a 修正——Gizmo 交互以 3D 视口内精确
    //      跟手为目标（正交轨道相机仿射映射天然 1:1）；高度轴直写 pos_z
    //      （静态属性，无 tween 通道）---------------------------------
    if (dragging_ && gizmo_drag_axis_ >= 0 && sel_.kind != 0 && scene_) {
        const QPointF dw = ev->position() - gizmo_press_widget_;
        const double len2 = gizmo_axis_scr_.x() * gizmo_axis_scr_.x() +
                            gizmo_axis_scr_.y() * gizmo_axis_scr_.y();
        if (len2 > 1e-6) {
            const double t =
                QPointF::dotProduct(dw, gizmo_axis_scr_) / len2;
            if (gizmo_drag_axis_ == 2) {
                // 高度轴（仅片段）：直写 pos_z——静态属性，无 tween 通道
                if (sel_.kind == 4 && sel_.a >= 0 &&
                    sel_.a < (int)scene_->timeline.tracks.size() &&
                    sel_.b >= 0 && sel_.b <
                        (int)scene_->timeline.tracks[sel_.a].clips.size()) {
                    scene_->timeline.tracks[sel_.a].clips[sel_.b]
                        .transform.pos_z = gizmo_orig_z_ + t;
                    moved_ = true;
                    if (preview_) preview_->invalidate();
                    emit sceneEditedLive();
                }
            } else {
                // X/Y 轴：与自由拖同写回路径（tween auto-key + 静态兑底；
                // 屏幕无透视缩放直写）
                double nx = drag_orig_x_;
                double ny = drag_orig_y_;
                if (gizmo_drag_axis_ == 0)
                    nx += t;
                else
                    ny += t;
                if (sel_.kind == 1 && sel_.a >= 0 &&
                    sel_.a < (int)scene_->screens.size()) {
                    scene_->screens[sel_.a].x = nx;
                    scene_->screens[sel_.a].y = ny;
                    syncPropDialog(scene_->screens[sel_.a].id); // 画布→弹窗
                    moved_ = true;
                    emit sceneEditedLive();
                } else if (sel_.kind == 4 && sel_.a >= 0 &&
                           sel_.a < (int)scene_->timeline.tracks.size() &&
                           sel_.b >= 0 &&
                           sel_.b <
                               (int)scene_->timeline.tracks[sel_.a]
                                   .clips.size()) {
                    auto& c =
                        scene_->timeline.tracks[sel_.a].clips[sel_.b];
                    const double cur_z = clipResolvedTransform(c).z;
                    if (!tweenAutoKeyWrite(c, TweenProp::Position, nx, ny,
                                           cur_z)) {
                        c.transform.x = nx;
                        c.transform.y = ny;
                    }
                    moved_ = true;
                    if (preview_) preview_->invalidate();
                    emit sceneEditedLive();
                }
            }
        }
        update();
        ev->accept();
        return;
    }

    if (!dragging_ || sel_.kind == 0 || !scene_) {
        QWidget::mouseMoveEvent(ev);
        return;
    }

    // ---- 素材角点/边线会话（2026-11-20）：分发专用 tick（画布域反投
    //      影 = 冻结高度平面，与旋转/缩放会话同口径）----
    if (corner_drag_ >= 0) {
        tickClipCornerDrag(canvasFromScreen(ev->position()));
        ev->accept();
        return;
    }
    if (edge_drag_ >= 0) {
        tickClipEdgeDrag(canvasFromScreen(ev->position()));
        ev->accept();
        return;
    }

    // ---- 旋转会话：方向角增量 → rotation（Shift 吸附 15°）。
    //      增量语义与手柄初始位置无关：new = orig + (now − start)，
    //      "角=中心到鼠标的画布方向角"（angleFromCenter 同约定）----
    if (rotating_) {
        const QPointF now = canvasFromScreen(ev->position()); // 冻结平面反投影
        QRectF r;
        double rot0 = 0.0;
        if (!selectionCanvasRect(&r, &rot0)) return;
        const double ang = cg::angleFromCenter(r.x(), r.y(), r.width(),
                                               r.height(), now.x(), now.y());
        double rot = rotate_orig_rot_ + (ang - rotate_start_ang_);
        if (ev->modifiers() & Qt::ShiftModifier)
            rot = std::round(rot / 15.0) * 15.0; // Shift：15° 吸附
        if (sel_.kind == 1 && sel_.a >= 0 &&
            sel_.a < (int)scene_->screens.size()) {
            scene_->screens[sel_.a].rotation = rot;
            syncPropDialog(scene_->screens[sel_.a].id); // 画布→弹窗
        } else if (sel_.kind == 4 && sel_.a >= 0 &&
                   sel_.a < (int)scene_->timeline.tracks.size() &&
                   sel_.b >= 0 &&
                   sel_.b < (int)scene_->timeline.tracks[sel_.a].clips.size()) {
            auto& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
            // 旋转通道接管 → auto-key（拖旋转=播放头处打键）；否则静态
            if (!tweenAutoKeyWrite(c, TweenProp::Rotation, rot))
                c.transform.rotation = rot;
        }
        moved_ = true;
        // 【暂停态拖动跟手】写工作副本后递增预览内容版本——拉帧泵据此在
        // 下一拍提交新合成请求（播放态靠播放头推进自然触发，暂停态唯一
        // 触发源就是这里；缺它则合成帧冻结在拖动前位置，标注层先行）
        if (preview_) preview_->invalidate();
        emit sceneEditedLive(); // display 节流下发（编辑中 display 持续跟手）
        update();
        return;
    }

    // ---- 角点缩放会话：锚定对角解算新外接框（旋转对象在初始局部坐标
    //      系解算，锚点画布位置全程不动——数学见 CanvasGeometry.hpp）；
    //      Shift = 等比约束（新框与原框相似，比例取包住鼠标的大轴）----
    if (resizing_) {
        const QPointF now = canvasFromScreen(ev->position()); // 冻结平面反投影
        // 最小尺寸 1（画布单位）：screen 满足 R4 校验（w/h>0）；clip 的
        // transform.w/h ≤ 0 会触发 auto-size 回退语义，显式 resize 恒写正值
        const bool keep_ratio = ev->modifiers() & Qt::ShiftModifier;
        const cg::ResizeResult rr =
            keep_ratio
                ? cg::resizeRectAnchoredRatio(
                      resize_orig_x_, resize_orig_y_, resize_orig_w_,
                      resize_orig_h_, resize_rotation_, resize_corner_,
                      now.x(), now.y(), 1.0, 1.0)
                : cg::resizeRectAnchored(
                      resize_orig_x_, resize_orig_y_, resize_orig_w_,
                      resize_orig_h_, resize_rotation_, resize_corner_,
                      now.x(), now.y(), 1.0, 1.0);
        if (sel_.kind == 1 && sel_.a >= 0 &&
            sel_.a < (int)scene_->screens.size()) {
            Screen& s = scene_->screens[sel_.a];
            s.x = rr.x;
            s.y = rr.y;
            s.w = rr.w;
            s.h = rr.h;
            syncPropDialog(s.id); // 画布→弹窗（缩放会话跟手刷数值）
        } else if (sel_.kind == 4 && sel_.a >= 0 &&
                   sel_.a < (int)scene_->timeline.tracks.size() &&
                   sel_.b >= 0 &&
                   sel_.b < (int)scene_->timeline.tracks[sel_.a].clips.size()) {
            auto& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
            auto& t = c.transform;
            // 方案 A auto-key（v3 三维打包双模）：位置/尺寸各按整条属性判定
            //——通道接管即打包打键（位置一键携带 x/y/z 三分量——z 取播放头
            // 当前值防拖拽归零；缩放一键携带 w/h）；无通道写静态基准
            const double cur_z = clipResolvedTransform(c).z;
            if (!tweenAutoKeyWrite(c, TweenProp::Size, rr.w, rr.h)) {
                t.w = rr.w;
                t.h = rr.h;
            }
            if (!tweenAutoKeyWrite(c, TweenProp::Position, rr.x, rr.y,
                                   cur_z)) {
                t.x = rr.x;
                t.y = rr.y;
            }
        }
        moved_ = true; // 缩放即编辑（与拖动同策略：真实变更才上报）
        if (preview_) preview_->invalidate(); // 暂停态缩放跟手（同位置拖动）
        emit sceneEditedLive(); // display 节流下发（同旋转会话）
        update();
        return;
    }

    // ---- 位置拖动（原有路径）：画布坐标增量 → 对象新 x/y（实时写工作
    //      副本；补丁在下发时统一 diff）。3D：增量取自"视线∩冻结高度
    //      平面"的反投影——拖动增量语义与 2D 逐位一致 ----
    const QPointF now   = canvasFromScreen(ev->position());
    double nx = drag_orig_x_ + (now.x() - drag_start_canvas_.x());
    double ny = drag_orig_y_ + (now.y() - drag_start_canvas_.y());
    // 真实位移才标记（0 位移的 move 事件不算拖动——单击防假脏）
    if (nx != drag_orig_x_ || ny != drag_orig_y_) moved_ = true;
    // ---- 自动吸附：移动后外接框的边缘/中心贴齐参考线（写 nx/ny 并记录
    //      命中线供绘制；关闭开关时仅清除上一帧的参考线显示）----
    guide_x_.clear();
    guide_y_.clear();
    guide_pt_valid_ = false; // snapping_ 关闭时 applySnap 不跑，手动失效
    if (snapping_) applySnap(&nx, &ny);
    if (sel_.kind == 1 && sel_.a >= 0 &&
        sel_.a < (int)scene_->screens.size()) {
        scene_->screens[sel_.a].x = nx;
        scene_->screens[sel_.a].y = ny;
        syncPropDialog(scene_->screens[sel_.a].id); // 画布→弹窗（拖动跟手）
    } else if (sel_.kind == 4 && sel_.a >= 0 &&
               sel_.a < (int)scene_->timeline.tracks.size() &&
               sel_.b >= 0 &&
               sel_.b < (int)scene_->timeline.tracks[sel_.a].clips.size()) {
        auto& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
        // 【2026-11-20 2D 化】正交扁平化后显示移动 = 画布 delta（透视
        // s=F/(F−pos_z) 缩放退役）——拖拽 delta 天然精确跟手，原"阶段③
        // 拖拽透视修正"补偿块删除。吸附仍按 clip 域历史语义。
        // 方案 A auto-key（2026-11-06 用户报告"打过 K 帧后画布拖不动"；
        // 2026-11-09 v3 三维打包：位置一键携带 x/y/z 三分量——z 取播放头
        // 当前值防拖拽把悬浮高度键成 0；无通道写静态基准
        const double cur_z = clipResolvedTransform(c).z;
        if (!tweenAutoKeyWrite(c, TweenProp::Position, nx, ny, cur_z)) {
            c.transform.x = nx;
            c.transform.y = ny;
        }
    }
    // 【暂停态拖动跟手】工作副本已写 → 内容版本失效，拉帧泵下一拍按新
    // 几何重新合成（此前只在松开时经 sceneEdited 链路间接刷新——暂停态
    // 播放头不动，拖动全程合成帧停在按下前，仅标注层移动）
    if (preview_) preview_->invalidate();
    // live 编辑→display 节流下发（适配器 requestAutoPublish 侧 200ms 一拍——
    // live 中 display 持续跟手，不再"松手才动"）
    emit sceneEditedLive();
    update();
}

void SceneCanvasWidget::mouseReleaseEvent(QMouseEvent* ev) {
    // ---- 3D 轨道收尾（右键拖拽 或 轴球球体拖拽）：真位移=旋转结束。
    // 右键真位移吞掉随后的菜单事件（视图旋转≠菜单意图）；左键球体拖拽
    // 无菜单语义，纯收尾 ----
    if (view3d_ && orbiting_ &&
        (ev->button() == Qt::RightButton ||
         ev->button() == Qt::LeftButton)) {
        orbiting_ = false;
        if (ev->button() == Qt::RightButton && orbit_moved_)
            suppress_menu_ = true; // 仅右键：真位移=旋转视图（吞菜单）
        unsetCursor();
        ev->accept();
        return;
    }
    // ---- 平移收尾（中键/左键空白会话；松开即结束，光标恢复箭头）----
    // 【2026-11-05】左键空白拖拽平移加入——两键共用会话标志（按下时
    // 已互斥：对象手柄/命中优先，未命中才进平移）
    if ((ev->button() == Qt::MiddleButton ||
         ev->button() == Qt::LeftButton) &&
        panning_) {
        panning_ = false;
        unsetCursor();
        // 撤销会话收口（左键平移有会话；中键无——endEdit 幂等无害）。
        // 视图平移不改场景 → 去重丢弃，不入历史。
        if (ev->button() == Qt::LeftButton && hist_)
            hist_->endEdit(*scene_);
        ev->accept();
        return;
    }
    if (ev->button() == Qt::LeftButton && dragging_vp_) {
        dragging_vp_  = false;
        vp_drag_axis_ = -1; // VP Gizmo 轴约束会话复位
        unsetCursor();
        if (moved_) emit sceneEdited(); // VP 落点已写入工作副本
        moved_ = false;
        if (hist_) hist_->endEdit(*scene_); // 撤销会话收口（VP 位移一条）
        ev->accept();
        return;
    }
    if (ev->button() == Qt::LeftButton && dragging_) {
        dragging_ = false;
        resizing_ = false;      // 缩放/旋转/移动会话统一在此收尾
        rotating_ = false;
        resize_corner_ = -1;
        gizmo_drag_axis_ = -1;  // 移动 Gizmo 轴约束会话复位
        corner_drag_ = -1;      // 素材角点/边线会话复位（2026-11-20）
        edge_drag_   = -1;
        corner_hover_ = -1;
        edge_hover_   = -1;
        guide_x_.clear();       // 吸附参考线显示随会话结束清除
        guide_y_.clear();
        guide_pt_valid_ = false;
        // 【仅真实拖动才上报】单击选中（无位移）不算编辑——假脏会让
        // 用户关窗时被"未保存"询问、下发补丁产生无意义 diff
        if (moved_) emit sceneEdited(); // 拖动落点已写入工作副本
        moved_ = false;
        if (hist_) hist_->endEdit(*scene_); // 撤销会话收口（拖动一条历史）
        update();
    }
    QWidget::mouseReleaseEvent(ev);
}

void SceneCanvasWidget::mouseDoubleClickEvent(QMouseEvent* ev) {
    if (!scene_) {
        QWidget::mouseDoubleClickEvent(ev);
        return;
    }
    // 双击屏=精确选中最上层（kTop：轮转属左键连点，双击不走）
    const Selection hit = hitTest(ev->position(), HitPick::kTop);
    // 【选中分工 2026-11-15】双击落在已选中片段（时间轴选中）身上 = 无
    // 动作：画布对 clip 无双击语义（属性面板唯一入口 = timeline 双击，
    // 2026-11-05 定案；hitTest 已不返回 clip）——特判防落到下方"空白
    // 双击复位视图"分支误触发。
    if (hitSelectedClip(ev->position())) return;
    if (hit.kind == 4) { // 理论不可达（hitTest screen-only）；防御保留
        applySelection(hit);
        return;
    }
    if (hit.kind == 1) {
        // 【2026-11-25 用户定案】双击屏 = 仅选中——属性面板唯一入口 =
        // 右键菜单「屏幕属性…」（双击弹属性与选中交互冲突，取消双击入口）
        applySelection(hit);
        return;
    }
    // ---- 空白双击 = 视图复位（滚轮/右键旋转/中键浏览后的快捷"回家"；
    //      原空白双击走 QWidget 默认（无副作用），行为叠加安全）----
    if (!viewIsHome()) {
        resetView();
        statusTip(view3d_ ? "视图已复位（缺省视角，画布居中）"
                                   : "视图已复位（100%，适配窗口）");
        return;
    }
    QWidget::mouseDoubleClickEvent(ev);
}

void SceneCanvasWidget::contextMenuEvent(QContextMenuEvent* ev) {
    // 【3D 轨道旋转协调】Windows 右键【松开】即发 contextMenuEvent——
    // 刚结束的拖拽旋转会话置 suppress_menu_ 吞掉本次菜单；无位移的右键
    // 单击不置旗标，菜单照常弹出
    if (suppress_menu_) {
        suppress_menu_ = false;
        return;
    }
    if (!scene_) return;
    // 【2026-11-15 选中分工+菜单收敛】画布右键菜单只服务银幕——片段无
    // 菜单（删除/属性等片段操作唯一入口 = timeline；此前「删除片段」项
    // 信号无消费者属死菜单）。已选片段的右键落空即无菜单，不误弹银幕菜单。
    // 【kSticky 2026-11-25 两轮实锤定稿】右键菜单禁轮转但延续选中——
    // 当前选中在命中集合内=用户有意编辑它（含连点轮转选中的下层屏），
    // 菜单动作必须服务它；选中不在集合/未选中屏=切视觉最上层。
    // 演化：无差别轮转 → 右键 player-1 翻 player-2；恒最上层 → 连点选中
    // 的 player-2 右键又翻回 player-1。kSticky=两轮回归的合取。
    const Selection hit = hitTest(ev->pos(), HitPick::kSticky);
    QMenu menu(this);

    if (hit.kind == 1) {
        // 【2026-11-15 三次定稿】菜单序=场景动作置顶（高频优先）→ 银幕操作
        // → 删除屏幕殿后（危险操作惯例）。
        QAction* act_pub  = menu.addAction("下发场景");
        QAction* act_sync = menu.addAction("同步媒体");
        menu.addSeparator();
        QAction* act_prop = menu.addAction("屏幕属性…");
        // 断开/连接显示端：画布内直接执行（setDisplayConnect）——停用立即
        // 断开，启用交看门狗重连。状态口径 = display_online_（宿主 200ms
        // 拍推送 connected&&enabled）。
        bool online = false;
        bool has_display = false;
        if (hit.a >= 0 && hit.a < (int)scene_->screens.size()) {
            const auto& s = scene_->screens[hit.a];
            has_display = !s.player_id.empty();
            if (has_display) {
                auto it = display_online_.find(s.player_id);
                online = it != display_online_.end() && it->second;
            }
        }
        QAction* act_conn = menu.addAction(online ? "断开显示端" : "连接显示端");
        act_conn->setEnabled(has_display); // 未绑定显示端无动作可做
        // 黑屏切换（2026-11-25）：单 display 正常显示/黑屏——MsgDisplayBlack
        // 单播直达该端（运行时指令不入场景、不随下发广播）；态镜像在
        // runtime（display 注册路径补发，重连/重启自愈）。离线端收不到
        // 指令 → 置灰（未绑定同）。
        bool black = false;
        if (has_display && runtime_)
            black = runtime_->displayBlack(QString::fromStdString(
                scene_->screens[hit.a].player_id));
        QAction* act_black = menu.addAction(black ? "取消黑屏" : "黑屏");
        act_black->setEnabled(has_display && online);
        menu.addSeparator();
        QAction* act_del = menu.addAction("删除屏幕");
        QAction* chosen = menu.exec(ev->globalPos());
        if (!chosen) return;
        // 右键菜单动作直接同步分发（kSticky hit=延续当前选中或视觉最上
        // 层——错层翻转的真凶是 hitTest 轮转复用，已在 hitTest 源头修复
        // 2026-11-25；历史上的"防泄漏守卫/singleShot 纠正"补丁随之撤除）
        if (chosen == act_prop) {
            applySelection(hit);
            openScreenProperty(hit.a);
        } else if (chosen == act_conn) {
            setDisplayConnect(hit.a, !online);
        } else if (chosen == act_black) {
            const std::string pid = scene_->screens[hit.a].player_id;
            runtime_->setDisplayBlack(QString::fromStdString(pid), !black);
            statusTip(QString("%1 %2")
                          .arg(QString::fromStdString(pid),
                               !black ? "已切黑屏" : "已恢复显示"));
        } else if (chosen == act_del) {
            deleteScreen(hit.a);
        } else if (chosen == act_pub) {
            publishScene();
        } else if (chosen == act_sync) {
            syncMedia();
        }
        return;
    }
    // 空白：添加屏幕（唯一入口——自动连带登记本机显示端，用户约定"只要一个"：
    // 屏幕与显示端一一对应。登记在 addScreen 内完成：
    // display_id = screen.player_id，本机默认端口）
    // 【2026-11-18 统一】2D/3D 从"类型"降格为"创建预设"——底层只有一种
    // Screen（画布取景窗 + 可选 3D 姿态 rot_x/rot_y）；新建默认零姿态 =
    // 旧 2D 行为，需要斜装/立面直接在属性对话框调俯仰/偏航（±85°）。
    // 不另立 3D 类型：真投影仪映射（Eye/Target/Lens Shift）作为 Screen
    // 字段扩展在统一模型上做，维护两套类型是历史包袱无谓继承。
    QAction* act_add = menu.addAction("添加屏幕");
    // 吸附对齐开关（拖动时贴齐其他图元的边缘/中心；默认开启）
    QAction* act_snap = menu.addAction("吸附对齐（边缘/中心）");
    act_snap->setCheckable(true);
    act_snap->setChecked(snapping_);
    // 【全局透视（Eye 点）入口已退役 2026-11-20 —— 2D 化】视角锁定正俯视、
    // 素材 pos_z 恒 0，全局透视失去作用对象；openEyeDialog() 函数本体
    // 保留供回退（对标 drawAxisGizmo 的退役模式）。
    // 【正交视图开关已退役 2026-11-18】功能及其函数全删——当初为预览
    // 画面而做，现 display 面片已在 3D 视口常驻贴图，预览合成恒与
    // display 同源（透视）。
    // 视图已缩放/平移/旋转时提供复位入口（与双击空白同效；可发现性）
    QAction* act_reset = nullptr;
    if (!viewIsHome())
        act_reset = menu.addAction(view3d_ ? "重置视图（缺省视角）"
                                           : "重置视图（100%）");
    QAction* chosen = menu.exec(ev->globalPos());
    if (chosen == act_add) addScreen();
    else if (chosen == act_snap) {
        snapping_ = act_snap->isChecked();
        statusTip(snapping_ ? "吸附对齐已开启" : "吸附对齐已关闭");
    }
    else if (chosen == act_reset) {
        resetView();
        statusTip(view3d_ ? "视图已复位（缺省视角，画布居中）"
                                   : "视图已复位（100%，适配窗口）");
    }
}

void SceneCanvasWidget::resizeEvent(QResizeEvent* ev) {
    update(); // fit 缩放依赖控件尺寸
    QOpenGLWidget::resizeEvent(ev); // 基类负责 FBO 重分配（GL 尺寸跟随）
}

// ---------------------------------------------------------------------------
// 滚轮缩放（裸滚轮；以光标为锚点）：光标下的画布点缩放前后保持不动——
//   光标 = fit中心 + view_pan + 画布点 × k  →  新 pan = 光标 − 画布点×新k − fit中心
// 缩放只改 canvasRect()（一处生效全链路）；合成帧尺寸随画布矩形放大，
// 放大浏览时预览清晰度同步提升（引擎侧 8192 上限兜底）。
// 【修饰键演进】2026-11-04 曾加 Ctrl 要求（防触摸板误触缩成一小块）；
// 2026-11-05 按用户最新要求去除 Ctrl——滚轮直接缩放（PS/Figma 主流网页
// 惯例），保留 10%~1600% 范围钳制 + 触摸板纯横滑不缩放两道防呆；
// 误缩放后双击空白/右键「适应窗口」一键回家。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::wheelEvent(QWheelEvent* ev) {
    // 【诊断 MS_CANVAS_DBG=1】wheel 到达性检查（PostMessage 注入联测：
    // 事件是否到达本 widget、修饰符是否被 Qt 识别）
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg) {
            static int s_cnt = 0;
            if (++s_cnt <= 20)
                std::fprintf(stderr,
                             "canvas-dbg: wheel#%d mods=0x%x "
                             "dy=%d pos=(%.0f,%.0f)\n",
                             s_cnt, (unsigned)ev->modifiers(),
                             ev->angleDelta().y(), ev->position().x(),
                             ev->position().y()), std::fflush(stderr);
        }
    }
    if (!scene_ || scene_->canvas_w <= 0 || scene_->canvas_h <= 0) {
        QWidget::wheelEvent(ev);
        return;
    }
    // ---- 3D dolly（正交"缩放"= 改视口比例；光标锚定）----
    // 触摸板横滑防护与 2D 同款；倍率范围 10%~1600%（与 2D view_zoom_ 同域）。
    // 【缩放语义】改相对倍率 view3d_zoom_，绝对比例 = fitScale × zoom 在
    // paintGL3D 开头每帧同步——此处手动同步一次供锚点换算使用
    if (view3d_) {
        if (ev->angleDelta().y() == 0) {
            QWidget::wheelEvent(ev);
            return;
        }
        const double factor = ev->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        const double nz     = std::clamp(view3d_zoom_ * factor, 0.1, 16.0);
        ev->accept();
        if (nz == view3d_zoom_) return; // 已到边界（吞掉即可）
        const QPointF anchor = widgetToCanvasAt(ev->position(), 0.0); // 旧视图域
        view3d_zoom_ = nz;
        const double fs  = fitScale();
        cam3d_.scale = (fs > 0 ? fs : 1.0) * view3d_zoom_; // 同步后才能反投影
        panAnchorTo(ev->position(), anchor); // 光标下地面点保持不动
        update();
        statusTip(
            QString("视图 %1%（滚轮缩放 · 左键空白/中键平移 · 双击空白复位）")
                .arg((int)std::lround(view3d_zoom_ * 100.0)));
        return;
    }
    // 步进：滚轮一格 ±15%；范围 10%~1600%（整画布鸟瞰 ↔ 单屏细节）。
    // 【触摸板横滑防护】angleDelta().y()==0（纯水平滚动）不缩放——否则
    // 触摸板横滑手势会被误判为缩小
    if (ev->angleDelta().y() == 0) {
        QWidget::wheelEvent(ev);
        return;
    }
    const double factor   = ev->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    const double new_zoom = std::clamp(view_zoom_ * factor, 0.1, 16.0);
    ev->accept();
    if (new_zoom == view_zoom_) return; // 已到边界（仍有滚轮事件，吞掉即可）
    const QPointF cp = widgetToCanvas(ev->position()); // 锚点（旧视图域）
    view_zoom_ = new_zoom;
    const double k = fitScale() * view_zoom_;
    view_pan_.setX(ev->position().x() - cp.x() * k -
                   (width() - scene_->canvas_w * k) / 2.0);
    view_pan_.setY(ev->position().y() - cp.y() * k -
                   (height() - scene_->canvas_h * k) / 2.0);
    update();
    statusTip(QString("视图 %1%（滚轮缩放 · 左键空白/中键拖拽平移 · "
                               "双击空白/「适应窗口」复位）")
                           .arg((int)std::lround(view_zoom_ * 100.0)));
}

// ---------------------------------------------------------------------------
// 键盘微调（方向键族）：选中对象后
//   方向键 = 调整大小（→/← 改宽、↓/↑ 改高；锚定左上角）
//   Alt+方向键 = 移动位置（x/y）
//   Shift = 大步进（10 画布单位；普通 1 画布单位）
// 写工作副本 + sceneEdited（置脏 + 防抖自动下发链路与鼠标拖动完全一致）
// ---------------------------------------------------------------------------
void SceneCanvasWidget::keyPressEvent(QKeyEvent* ev) {
    if (!scene_ || sel_.kind == 0) {
        QWidget::keyPressEvent(ev); // 无选中：交还父类（不提示不占用）
        return;
    }
    int dx = 0, dy = 0;
    switch (ev->key()) {
        case Qt::Key_Left:  dx = -1; break;
        case Qt::Key_Right: dx = +1; break;
        case Qt::Key_Up:    dy = -1; break;
        case Qt::Key_Down:  dy = +1; break;
        default: QWidget::keyPressEvent(ev); return; // 其他键不拦截
    }
    const double step =
        (ev->modifiers() & Qt::ShiftModifier) ? 10.0 : 1.0; // 画布单位
    const bool move_mode = ev->modifiers() & Qt::AltModifier; // Alt=移动

    if (sel_.kind == 1 && sel_.a >= 0 &&
        sel_.a < (int)scene_->screens.size()) {
        Screen& s = scene_->screens[sel_.a];
        if (move_mode) { s.x += dx * step; s.y += dy * step; }
        else {           s.w = std::max(1.0, s.w + dx * step);
                         s.h = std::max(1.0, s.h + dy * step); }
        syncPropDialog(s.id); // 画布→弹窗（键盘微调同联动）
    } else if (sel_.kind == 4 && sel_.a >= 0 &&
               sel_.a < (int)scene_->timeline.tracks.size() && sel_.b >= 0 &&
               sel_.b < (int)scene_->timeline.tracks[sel_.a].clips.size()) {
        // 【方向键 auto-key 同权（2026-11-09 #89）】与鼠标拖拽同语义：
        // 基准取播放头求值（clipResolvedTransform——通道接管时静态
        // transform 是过期值，直接在其上累加会被求值覆盖，画面不动且
        // 静态值漂移）；通道存在 → 播放头处打键（z 取当前求值防把悬浮
        // 高度键成 0；尺寸钳 1 下限与静态分支同口径），无通道写静态。
        auto& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
        const Transform base = clipResolvedTransform(c);
        if (move_mode) {
            if (!tweenAutoKeyWrite(c, TweenProp::Position,
                                   base.x + dx * step, base.y + dy * step,
                                   base.z)) {
                c.transform.x += dx * step;
                c.transform.y += dy * step;
            }
        } else {
            if (!tweenAutoKeyWrite(c, TweenProp::Size,
                                   (std::max)(1.0, base.w + dx * step),
                                   (std::max)(1.0, base.h + dy * step))) {
                c.transform.w = std::max(1.0, c.transform.w + dx * step);
                c.transform.h = std::max(1.0, c.transform.h + dy * step);
            }
        }
    } else {
        QWidget::keyPressEvent(ev); // 选中下标失效（场景刚变）：交还父类
        return;
    }
    ev->accept();
    if (preview_) preview_->invalidate(); // 暂停态键盘微调跟手（同拖动路径）
    update();
    emit sceneEdited(); // 与鼠标拖动同链路（置脏 + 50ms 防抖自动下发）
    // 撤销收编（离散一步式；自动重复经合并窗口折叠——长按方向键为一条）
    if (hist_) hist_->checkpoint("方向键微调", *scene_);
    statusTip(QString("%1：%2 %3%4（Shift 大步进 · Alt 切移动）")
                           .arg(sel_.kind == 1 ? "屏幕" : "片段",
                                move_mode ? "移动" : "大小")
                           .arg(dx != 0 ? (dx > 0 ? "→" : "←")
                                        : (dy > 0 ? "↓" : "↑"))
                           .arg(QString::number(step, 'f', 0)));
}

// ---------------------------------------------------------------------------
// 自动吸附：参考线收集与命中
// ---------------------------------------------------------------------------
void SceneCanvasWidget::collectSnapReferences() {
    snap_ref_x_.clear();
    snap_ref_y_.clear();
    if (!scene_) return;
    auto addX = [&](double v) { snap_ref_x_.push_back(v); };
    auto addY = [&](double v) { snap_ref_y_.push_back(v); };
    // screens（排除被拖的 screen 自身——自己对自己吸附无意义）
    for (int i = 0; i < (int)scene_->screens.size(); ++i) {
        if (sel_.kind == 1 && sel_.a == i) continue;
        const Screen& s = scene_->screens[i];
        addX(s.x); addX(s.x + s.w / 2.0); addX(s.x + s.w);
        addY(s.y); addY(s.y + s.h / 2.0); addY(s.y + s.h);
    }
    // clips（排除被拖的 clip 自身；宽高取绘制矩形——与视觉一致）
    // 【只显当前】不活跃片段不上画布，也不作吸附参考（对不可见对象
    // 吸附会凭空"吸住"——与绘制/命中同口径）
    for (int tr = 0; tr < (int)scene_->timeline.tracks.size(); ++tr) {
        const auto& clips = scene_->timeline.tracks[tr].clips;
        for (int cl = 0; cl < (int)clips.size(); ++cl) {
            if (sel_.kind == 4 && sel_.a == tr && sel_.b == cl) continue;
            if (!clipActiveAtPlayhead(clips[cl])) continue;
            const QRectF r = clipCanvasRect(clips[cl]);
            addX(r.left()); addX(r.center().x()); addX(r.right());
            addY(r.top());  addY(r.center().y()); addY(r.bottom());
        }
    }
    // 升序排序（命中扫描用二分窗口；去重非必需——重复线命中同值无害）
    std::sort(snap_ref_x_.begin(), snap_ref_x_.end());
    std::sort(snap_ref_y_.begin(), snap_ref_y_.end());
}

void SceneCanvasWidget::applySnap(double* nx, double* ny) {
    if (!scene_ || scene_->canvas_w <= 0) return;
    // 阈值 = 控件 7px 换算画布单位（视觉手感恒定，不随缩放变化）
    const double k = canvasRect().width() / scene_->canvas_w;
    if (k <= 0) return;
    const double thr = 7.0 / k;

    // 被拖对象当前宽高（吸附锚点用；位置拖动不改尺寸，直接取现值）
    double w = 0, h = 0;
    if (sel_.kind == 1 && sel_.a >= 0 &&
        sel_.a < (int)scene_->screens.size()) {
        w = scene_->screens[sel_.a].w;
        h = scene_->screens[sel_.a].h;
    } else if (sel_.kind == 4 && sel_.a >= 0 &&
               sel_.a < (int)scene_->timeline.tracks.size() && sel_.b >= 0 &&
               sel_.b < (int)scene_->timeline.tracks[sel_.a].clips.size()) {
        const QRectF r = clipCanvasRect(
            scene_->timeline.tracks[sel_.a].clips[sel_.b]);
        w = r.width();
        h = r.height();
    } else {
        return;
    }

    // 单轴吸附：对 moving 的 3 锚（left/center/right）找阈值内最近参考线。
    // 命中 → 位置贴齐 + 记录参考线（绘制）。x/y 独立处理（可同时吸附）。
    // off_out 输出命中的锚偏移（0=左/上边、size/2=中线、size=右/下边），
    // 供角点交叉判定（x/y 同时命中边线级锚 = 角对角贴合）。
    auto snapAxis = [&](const std::vector<double>& refs, double pos,
                        double size, std::vector<double>* hit_out,
                        double* off_out) -> double {
        double best = pos;
        double best_diff = thr;   // 命中上限即阈值（初值保证只收阈值内的）
        double best_ref  = 0.0;   // 命中的参考线值（绘制用）
        double best_off  = 0.0;   // 命中的锚偏移（角点判定用）
        bool   found = false;
        for (const double ref : refs) {
            for (const double off : {0.0, size / 2.0, size}) { // left/center/right
                const double diff = std::fabs(ref - (pos + off));
                if (diff <= best_diff) {
                    best_diff = diff;
                    best  = ref - off; // 贴齐后的位置（锚点落在参考线上）
                    best_ref = ref;
                    best_off = off;
                    found = true;
                }
            }
        }
        if (found) {
            hit_out->push_back(best_ref); // 记录命中的参考线（去重）
            *off_out = best_off;
        }
        return best;
    };
    guide_x_.clear();
    guide_y_.clear();
    double x_off = 0.0, y_off = 0.0;
    *nx = snapAxis(snap_ref_x_, *nx, w, &guide_x_, &x_off);
    *ny = snapAxis(snap_ref_y_, *ny, h, &guide_y_, &y_off);

    // 角点吸附标记（2026-11-26）：x/y 两轴同时命中**边线级**锚（left/right
    // × top/bottom，即锚偏移 ≠ 中线 size/2）→ 交叉点 = 被拖对象角点与参考
    // 对象边线/角点重合。角点贴合本就由单轴吸附隐式生效（角点坐标=边线坐
    // 标），这里只是记下交叉点供绘制实心小方块——"角对角"可感知。
    guide_pt_valid_ = false;
    if (!guide_x_.empty() && !guide_y_.empty() &&
        x_off != w / 2.0 && y_off != h / 2.0) {
        guide_pt_ = QPointF(*nx + x_off, *ny + y_off);
        guide_pt_valid_ = true;
    }
}

void SceneCanvasWidget::snapPoint(double* px, double* py) {
    guide_x_.clear();
    guide_y_.clear();
    guide_pt_valid_ = false;
    if (!scene_ || scene_->canvas_w <= 0) return;
    // 阈值 = 控件 7px 换算画布单位（与 applySnap 同手感）
    const double k = canvasRect().width() / scene_->canvas_w;
    if (k <= 0) return;
    const double thr = 7.0 / k;
    // 单点语义：无外接框三锚，点本身即锚——各轴找阈值内最近参考线
    auto nearest = [&](const std::vector<double>& refs, double v,
                       bool* hit) -> double {
        double best = v;
        double best_diff = thr;
        bool   found = false;
        for (const double ref : refs) {
            const double diff = std::fabs(ref - v);
            if (diff <= best_diff) {
                best_diff = diff;
                best  = ref;
                found = true;
            }
        }
        *hit = found;
        return best;
    };
    bool hit_x = false, hit_y = false;
    const double sx = nearest(snap_ref_x_, *px, &hit_x);
    const double sy = nearest(snap_ref_y_, *py, &hit_y);
    if (hit_x) { *px = sx; guide_x_.push_back(sx); }
    if (hit_y) { *py = sy; guide_y_.push_back(sy); }
    // 两轴同中 → 交叉点即被拖角，角点标记与移动会话同款视觉
    if (hit_x && hit_y) {
        guide_pt_ = QPointF(*px, *py);
        guide_pt_valid_ = true;
    }
}

} // namespace ms
