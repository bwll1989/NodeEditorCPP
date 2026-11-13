// ============================================================================
//  SceneCanvasWidget.cpp —— Scene 预览画布实现（第五迭代：3D 视口）
//
//  【3D 视口（2026-11-05 用户定案"整个画布预览控件改为三维的"）】
//  Blender 式正交 3D 视口：画布矩形躺在地面（h=0），片段按 transform.z
//  悬浮其上（Z 位置即空间高度），地面网格 + 三轴参照线。
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
//    * 松开：sceneEdited 信号（EditorWindow 置脏，补丁统一下发）；
//    * 拖动/缩放/旋转会话中：clipTransformLiveChanged 高频广播（片段）——
//      属性面板数字跟手（2026-11-05 画布→面板实时同步）；
//    * 双击：片段 = 仅选中（弹属性面板已移除，2026-11-05 用户定案——
//      属性面板唯一入口 = timeline 双击）/ 屏幕 → screenDoubleClicked；
//    * 右键：3D 模式 = 拖拽旋转视角（真位移）/ 无位移弹菜单；2D = 菜单；
//      菜单内容：空白 = 添加屏幕；屏幕 = 属性/删除；片段 = 删除（单动作）。
// ============================================================================

#include "SceneCanvasWidget.hpp"

#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "PreviewRenderer.hpp" // 预览渲染（render 成功 → 真实合成帧上屏）
#include "model/CanvasGeometry.hpp" // 角点缩放/旋转命中纯函数（core 共享）
#include "engine/TweenEngine.hpp" // tween 通道求值纯函数（画布跟随曲线）
#include "GlRenderer.hpp"   // GL 合成后端（ExternalCtx 模式；GL 直通路径）
// 注：本 TU 不 include GlLoader.hpp——Qt GL 头（QOpenGLWidget→qopengl.h）
// 已定义 GL_* 宏，与 GlLoader 的常量枚举冲突；GL 诊断在 GlRenderer 内部做。

namespace ms {

namespace cg = canvas_geometry;

namespace {
// ---------------------------------------------------------------------------
// 轨道类型色（3D 标注层与 2D drawClip 两处同源共用——2026-11-06 收敛原
// 两份相同 switch；色值与媒体插件 *ClipPlugin.hpp 的 setColor 同组，视觉
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
inline bool trackInvisibleOnCanvas(AssetKind kind) {
    return kind == AssetKind::Audio || kind == AssetKind::Control;
}
/// z 排序哨兵下限（命中测试"尚无候选"初值；原两处独立硬编码收敛）
constexpr int kZOrderMin = -1'000'000'00;
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
                std::fprintf(stderr, "[canvas-dbg] pumpTick cost=%lldus\n",
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
                    std::fprintf(stderr, "[canvas-dbg] loopLag=%lldms\n",
                                 (long long)lag_ms);
                    std::fflush(stderr);
                });
            }
        }
    });
    pull_timer_->start(33);

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
                std::fprintf(stderr, "[canvas-shot] 已保存 %s (%dx%d)\n",
                             path.toLocal8Bit().constData(), img.width(),
                             img.height());
            else
                std::fprintf(stderr, "[canvas-shot] 保存失败: %s\n",
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
                         "[canvas-dbg] paintEvent#%d cost=%lldus wall=%lld\n",
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
SceneCanvasWidget::~SceneCanvasWidget() = default;

void SceneCanvasWidget::setScene(Scene* s) {
    scene_ = s;
    applySelection(Selection{}); // 场景指针切换，选中态作废（通知场景树清高亮）
    dragging_ = false;
    resizing_ = false;    // 进行中的缩放/旋转会话同样作废（对象可能已不存在）
    rotating_ = false;
    resize_corner_ = -1;
    // 【打开即铺满】场景加载/切换 → 视图复位（zoom=fit、pan=0）：预览视图
    // 恒定呈现"整个 scene 铺满预览区"（WATCHOUT 舞台语义）。没有这条，
    // 上次会话的滚轮缩放/平移会延续到新场景，打开即"画布缩在角落"。
    resetView();
    update();
}

void SceneCanvasWidget::setPreviewRenderer(PreviewRenderer* r) {
    preview_ = r; // 借用指针（EditorWindow 拥有；nullptr = 回退占位画法）
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
    applySelection(Selection{kind, a, b}); // 统一入口（含场景树通知）
}

// ---------------------------------------------------------------------------
// 选中统一入口：赋值 + 重绘 + selectionChanged（场景树同步高亮）
// 值未变化时静默返回——press 高频路径/外部 highlight 重复调用不刷树
// ---------------------------------------------------------------------------
void SceneCanvasWidget::applySelection(const Selection& s) {
    if (s == sel_) return;
    sel_ = s;
    update();
    emit selectionChanged(sel_.kind, sel_.a, sel_.b);
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
        emit statusMessage("右视图（轴球 +X）· 双击空白复位");
        break;
    case 1:  // −X 左视
        cam3d_.yaw_deg = 270.0; cam3d_.pitch_deg = Camera3D::kPitchMin;
        emit statusMessage("左视图（轴球 −X）· 双击空白复位");
        break;
    case 2:  // +Z 顶视（沿高度轴正俯视画布——与 2D 回退视图同向无镜像；
             // 高度轴即属性面板"Z 位置"）
        cam3d_.yaw_deg = 0.0;   cam3d_.pitch_deg = Camera3D::kPitchMax;
        emit statusMessage("顶视图（轴球 +Z）· 双击空白复位");
        break;
    case 3:  // +Y 正视（贴地平视画布正面——画布 y 轴方向）
        cam3d_.yaw_deg = 0.0;   cam3d_.pitch_deg = Camera3D::kPitchMin;
        emit statusMessage("正视图（轴球 +Y）· 双击空白复位");
        break;
    case 4:  // −Y 背视
        cam3d_.yaw_deg = 180.0; cam3d_.pitch_deg = Camera3D::kPitchMin;
        emit statusMessage("背视图（轴球 −Y）· 双击空白复位");
        break;
    default:
        return;
    }
    update();
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
                   .clips[sel_.b].transform.z; // 片段悬浮高度 = transform.z
    return 0.0; // 屏幕/无选中：地面（屏幕是物理输出平面，恒贴地）
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
    // 当前 z 防拖拽把高度键成 0）
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
// 命中测试
//   * 2D：片段取 z 最大者（顶层优先），其次 screen——旋转对象把点相对
//     中心逆旋转后判未旋转矩形；
//   * 3D：鼠标视线与每个对象所在高度平面求交后做同样的画布域包含测试
//     （片段平面 h=transform.z，屏幕恒地面 h=0）；多个平面都命中时取
//     视深最近者——正交投影下"离相机近 = 视觉遮挡在上"，与 2D 的
//     "z 大者在上的叠放语义"统一为同一视觉直觉
// ---------------------------------------------------------------------------
SceneCanvasWidget::Selection SceneCanvasWidget::hitTest(
        const QPointF& widget_pos) const {
    if (!scene_ || scene_->canvas_w <= 0 || scene_->canvas_h <= 0) return {};

    // 统一换算到画布域（一次 fit 换算；旋转发生在对象局部，画布域判定
    // 与控件缩放无关）。旋转判定经 CanvasGeometry 共享实现（与 core_tests
    // 验证的数学逐位一致）
    const auto rectHit = [](const QRectF& r, double rot_deg,
                            const QPointF& p) -> bool {
        return cg::rotatedRectContains(r.x(), r.y(), r.width(), r.height(),
                                       rot_deg, p.x(), p.y());
    };
    // 片段命中（变形感知，2026-11-09 锚点/角点改造）：
    //   * 默认（中心锚无位移）→ 原 rotatedRectContains（逐位不变）；
    //   * 仅锚点变化 → 点绕锚点逆旋转回未旋转矩形局部系再判包含
    //     （数学 = rotatedRectContains 平移锚点版）；
    //   * 角点位移 → 位移后四角 AABB（四边形可能非凸，AABB 简单可靠；
    //     编辑器命中精度足够——绘制边框仍是四边形本体）
    const auto clipHit = [](const QRectF& r, const Transform& tf,
                            const QPointF& p) -> bool {
        if (r.width() <= 0 || r.height() <= 0) return false;
        if (tf.poseActive()) {
            // 3D 姿态（2026-11-10）：quadCorners3D 预投影四角 AABB——
            // 透视四边形可能非凸，AABB 简单可靠（与角点位移分支同策略）；
            // quadCorners3D 内含滚转/锚点/位移全部语义，姿态存在时优先
            double wx[4], wy[4];
            cg::quadCorners3D(r.x(), r.y(), r.width(), r.height(),
                              tf.rotation, tf.rot_x, tf.rot_y, tf.anchor,
                              tf.corner, wx, wy);
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
    };

    if (view3d_) {
        // ---- 3D 拾取：逐对象高度平面反投影 → 画布域包含 → 视深最近 ----
        // 视深 = dot(对象中心−注视点, F)（Camera3D::depthOf）——同一鼠标
        // 射线命中的两个对象，平面离相机近者深度小（视觉在上）。
        Selection best{};
        double best_depth = 0.0;
        for (int tr = 0; tr < (int)scene_->timeline.tracks.size(); ++tr) {
            const auto& clips = scene_->timeline.tracks[tr].clips;
            for (int cl = 0; cl < (int)clips.size(); ++cl) {
                const Clip& c = clips[cl];
                // 【只显当前】不活跃片段不可命中——所见即所得（画布上
                // 看不见的隐形框若可命中会"挡住"别的对象）
                if (!clipActiveAtPlayhead(c)) continue;
                // 【音频/Control 片段不命中】无图像（画布不画）——命中同
                // 口径跳过，防止隐形框拦截点击
                if (trackInvisibleOnCanvas(scene_->timeline.tracks[tr].kind))
                    continue;
                const QRectF r = clipCanvasRect(c);
                if (r.width() <= 0 || r.height() <= 0) continue;
                const double h = (double)c.transform.z;
                const QPointF cp =
                    widgetToCanvasAt(widget_pos, h); // 该片段高度平面
                // rotation/anchor/corner 走统一求值入口（tween 通道接管
                // 语义 + 2026-11-09 变形感知命中）
                if (!clipHit(r, clipResolvedTransform(c), cp))
                    continue;
                const double d =
                    cam3d_.depthOf(r.center().x(), h, r.center().y());
                // 同深度（同平面）取数组序靠后者——与 2D 叠放次序一致
                if (best.kind != 4 || d <= best_depth) {
                    best = Selection{4, tr, cl};
                    best_depth = d;
                }
            }
        }
        if (best.kind == 4) return best;

        int best_scr = -1;
        double best_scr_depth = 0.0;
        for (int i = 0; i < (int)scene_->screens.size(); ++i) {
            const Screen& s = scene_->screens[i];
            if (s.w <= 0 || s.h <= 0) continue;
            const QPointF cp = widgetToCanvasAt(widget_pos, 0.0); // 屏幕贴地
            if (!rectHit(QRectF(s.x, s.y, s.w, s.h), s.rotation, cp)) continue;
            const double d =
                cam3d_.depthOf(s.x + s.w / 2.0, 0.0, s.y + s.h / 2.0);
            if (best_scr < 0 || d <= best_scr_depth) {
                best_scr = i;
                best_scr_depth = d;
            }
        }
        if (best_scr >= 0) return Selection{1, best_scr, -1};
        return {};
    }

    const QPointF cp = widgetToCanvas(widget_pos);

    // ---- 片段：z 最大者优先（同 z 取数组序靠后者——绘制次序中在上层，
    //      与 z 升序 stable_sort 的绘制层级一致）----
    Selection best{};
    int best_z = kZOrderMin;
    for (int tr = 0; tr < (int)scene_->timeline.tracks.size(); ++tr) {
        const auto& clips = scene_->timeline.tracks[tr].clips;
        for (int cl = 0; cl < (int)clips.size(); ++cl) {
            // 【只显当前】不活跃片段不可命中（与 3D 视图/绘制同口径）
            if (!clipActiveAtPlayhead(clips[cl])) continue;
            // 【音频/Control 片段不命中】无图像（画布不画）——同口径跳过
            if (trackInvisibleOnCanvas(scene_->timeline.tracks[tr].kind))
                continue;
            if (!clipHit(clipCanvasRect(clips[cl]),
                         clipResolvedTransform(clips[cl]), cp))
                continue;
            const int z = clips[cl].transform.z;
            if (best.kind != 4 || z >= best_z) {
                best = Selection{4, tr, cl};
                best_z = z;
            }
        }
    }
    if (best.kind == 4) return best;

    // ---- 屏幕：z 大者优先（多屏重叠取上层；同 z 取数组序靠后）----
    int best_scr = -1;
    int best_scr_z = kZOrderMin;
    for (int i = 0; i < (int)scene_->screens.size(); ++i) {
        const Screen& s = scene_->screens[i];
        if (!rectHit(QRectF(s.x, s.y, s.w, s.h), s.rotation, cp)) continue;
        if (best_scr < 0 || s.z >= best_scr_z) {
            best_scr = i;
            best_scr_z = s.z;
        }
    }
    if (best_scr >= 0) return Selection{1, best_scr, -1};

    return {};
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
    QRectF r;
    double rot = 0.0;
    if (!selectionCanvasRect(&r, &rot)) return -1;
    if (r.width() <= 0 || r.height() <= 0) return -1;
    // 变形片段（锚点/角点）手柄隐藏：角点缩放语义在变形四边形上未定义
    //（手柄画在未位移角会与边框分离——宁缺勿错，角点拖拽后续版本实现）
    if (sel_.kind == 4) {
        const Clip& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
        const Transform tf = clipResolvedTransform(c);
        if (tf.anchor != 0 || tf.cornerActive()) return -1;
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
    // 变形片段：旋转柄同缩放柄一起隐藏（见 hitResizeHandle）
    if (sel_.kind == 4) {
        const Clip& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
        const Transform tf = clipResolvedTransform(c);
        if (tf.anchor != 0 || tf.cornerActive()) return false;
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
                std::fprintf(stderr, "[canvas-dbg] paintGL cost=%lldus\n",
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
        paintGL3D(items);
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
                    if (!it.frame.isNull()) ++nn;
                std::fprintf(stderr,
                             "[canvas-dbg] paint#%d pull=%d composed=%d "
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
                std::fprintf(stderr, "[canvas-dbg] paint#%d 标注层跳过（诊断）\n",
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
                    if (it.frame.isNull()) continue;
                    // 绘制项合成域坐标 → 画布坐标：域原点 + 坐标×域缩放
                    const double cx = gpu_dom.x() + it.item.x * sx;
                    const double cy = gpu_dom.y() + it.item.y * sy;
                    const double cw = it.item.w * sx;
                    const double chh = it.item.h * sy;
                    p.setOpacity(std::clamp(it.item.opacity, 0.0, 1.0));
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
                                           it.item.canvas_h * k), it.frame);
                        p.restore();
                    } else {
                        // 轴对齐：输出矩形 → 画布 → 控件直接映射
                        const QRectF target(cr.left() + cx * k,
                                            cr.top() + cy * k,
                                            cw * k, chh * k);
                        p.drawImage(target, it.frame);
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
        std::vector<int> scr_order;
        scr_order.reserve(scene_->screens.size());
        for (int i = 0; i < (int)scene_->screens.size(); ++i)
            scr_order.push_back(i);
        std::stable_sort(scr_order.begin(), scr_order.end(),
                         [this](int a, int b) {
                             return scene_->screens[a].z <
                                    scene_->screens[b].z;
                         });
        const bool no_frame = latest_frame_.isNull() && !composed;
        for (int i : scr_order)
            drawScreen(p, i, sel_.kind == 1 && sel_.a == i, no_frame);
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
//   3. cue 图元（仅片段）按【离地高度 h 升序、同高视深远→近】绘制：片段 =
//      仿射贴图（GpuDeliver 源帧）或棋盘格 + 类型色边框 + 标签——cue 间
//      遮挡关系纯靠 Z 轴（transform.z）；
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
void SceneCanvasWidget::paintGL3D(
        const std::vector<PreviewRenderer::DeliverItem>& items) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

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
    // 三轴参照线（Blender 配色语义：红=X、绿=Y、蓝=Z/高度）
    p.setPen(QPen(QColor(0xA8, 0x52, 0x52), 1.4));
    p.drawLine(project3(gx0, 0, 0), project3(gx1, 0, 0)); // 画布 x 轴（世界 X）
    p.setPen(QPen(QColor(0x52, 0xA0, 0x5E), 1.4));
    p.drawLine(project3(0, 0, gz0), project3(0, 0, gz1)); // 画布 y 轴（世界 Z）
    const double haxis = std::max(200.0, scene_->canvas_h * 0.35);
    p.setPen(QPen(QColor(0x55, 0x78, 0xC0), 1.4));
    p.drawLine(project3(0, 0, 0), project3(0, haxis, 0)); // 高度轴（原点竖直）

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
        std::string   clip_id; // 交付项的 clip 身份（空 = 无法匹配）
        const QImage* img;
    };
    std::vector<TexEntry> texs;
    texs.reserve(items.size());
    for (const auto& it : items) {
        if (it.frame.isNull()) continue; // native 项（未协商成功）→ 棋盘格
        texs.push_back({it.clip_id, &it.frame});
    }
    auto matchTex = [&texs](const std::string& clip_id) -> const QImage* {
        if (clip_id.empty()) return nullptr;
        for (const auto& t : texs) {
            if (t.clip_id == clip_id) return t.img; // 同 id 多项取首个
        }
        return nullptr; // 本片段无交付帧（未就绪/出区间）→ 棋盘格
    };

    struct Prim {
        int       kind;    // 4=clip（display 已不入 prims——恒最上层单独绘制）
        int       a, b;    // 场景下标
        double    depth;   // 视深（大 = 远）
        double    h;       // 【遮挡主键】离地高度（clip = transform.z）——
                           // cue 间遮挡纯靠 Z 轴，与合成器 Stacking By
                           // Layer 同语义：低层先画、高层后画
        QPolygonF quad;    // 投影四边形（0=TL 1=TR 2=BR 3=BL）
        QTransform tex;    // 画布局部矩形 → 屏幕（仿射；贴图/棋盘共用）
        const QImage* img; // 匹配源帧（可空 = 棋盘格占位）
        bool deformed = false; // 变形片段（锚点≠中心或角点位移）——贴图
                               // 改走两三角仿射分块（quad 非平行四边形，
                               // 单仿射 tex 数学前提已破）
    };
    // 【display/cue 关系 2026-11-05 用户定案】display 是"取景器"而非实体：
    // 没有 Z 设置、不参与 cue 遮挡排序、无颜色填充只画边框、恒绘制在
    // 所有 cue 之上（WATCHOUT Stage 同风格）。prims 只装 cue。
    std::vector<Prim> prims;
    prims.reserve(8);
    // display 投影四边形（收集后于所有 cue 绘制完再画——恒最上层）
    std::vector<std::pair<int, QPolygonF>> screen_quads;
    screen_quads.reserve(scene_->screens.size());
    for (int i = 0; i < (int)scene_->screens.size(); ++i) {
        const Screen& s = scene_->screens[i];
        if (s.w <= 0 || s.h <= 0) continue;
        screen_quads.push_back(
            {i, projectQuad(QRectF(s.x, s.y, s.w, s.h), s.rotation, 0.0)});
    }
    for (int tr = 0; tr < (int)scene_->timeline.tracks.size(); ++tr) {
        const auto& clips = scene_->timeline.tracks[tr].clips;
        // 【音频/Control 片段不画】无图像数据——3D 视图同样不产面片（与
        // 2D drawClip / 命中测试同口径）
        if (trackInvisibleOnCanvas(scene_->timeline.tracks[tr].kind)) continue;
        for (int cl = 0; cl < (int)clips.size(); ++cl) {
            const Clip&   c = clips[cl];
            // 【只显当前】播放头不活跃的片段不画（连方框占位也无——
            // WATCHOUT Stage 语义；命中同口径过滤见 hitTest）
            if (!clipActiveAtPlayhead(c)) continue;
            const QRectF  r = clipCanvasRect(c);
            if (r.width() <= 0 || r.height() <= 0) continue;
            const Transform tf = clipResolvedTransform(c); // tween 求值
            const double h = (double)c.transform.z; // Z 无 tween 通道（静态）
            // 变形片段（2026-11-09 锚点/角点；2026-11-10 姿态 3D）：四角
            // 绕锚点 + 位移（非平行四边形）；俯仰/偏航非 0 = 透视四边形
            //（quadCorners3D 预投影——含滚转/锚点/位移/透视缩放全部语义，
            // 与 display ScreenMapper pose3d 链同源唯一真相）；默认中心锚
            // 无姿态无位移走历史路径（projectQuad 逐位）。
            // 【画布↔display 一致性】此前 deformed 漏判 posed——俯仰/偏航
            // 只进 display（ScreenMapper pose3d 链），3D 视图画面恒为
            // 2D 矩形（用户体感"XY 旋转无效"）；deformed 含 posed 后
            // 贴图/棋盘格/边框/选中框自动走四边形本体路径（pr.quad）。
            const bool posed = tf.poseActive();
            const bool deformed =
                posed || tf.anchor != 0 || tf.cornerActive();
            QPolygonF q(4);
            if (posed) {
                double wx[4], wy[4];
                cg::quadCorners3D(r.x(), r.y(), r.width(), r.height(),
                                  tf.rotation, tf.rot_x, tf.rot_y,
                                  tf.anchor,
                                  tf.cornerActive() ? tf.corner : nullptr,
                                  wx, wy);
                // 画布域四角（z=0 域透视）→ 逐角提升到片段所在高度平面
                //（cam3d 正交投影——场景层相机与片段姿态两层语义正交）
                for (int i = 0; i < 4; ++i)
                    q[i] = project3(wx[i], h, wy[i]);
            } else {
                q = projectQuad(r, tf.rotation, h, tf.anchor,
                                deformed ? tf.corner : nullptr);
            }
            // 仿射（画布局部 (0,0)-(w,h) → 投影平行四边形）：三对应点。
            // 列向量 = (P1−P0)/w 与 (P3−P0)/h——正交投影保平行性，此
            // 变换精确（无透视分量），QTransform 仿射足够
            QTransform m((q[1].x() - q[0].x()) / r.width(),
                         (q[1].y() - q[0].y()) / r.width(),
                         (q[3].x() - q[0].x()) / r.height(),
                         (q[3].y() - q[0].y()) / r.height(),
                         q[0].x(), q[0].y());
            prims.push_back({4, tr, cl,
                             cam3d_.depthOf(r.center().x(), h, r.center().y()),
                             h, q, m,
                             matchTex(c.id), deformed});
        }
    }
    // 【遮挡排序 2026-11-05 定案】cue 间遮挡纯靠【离地高度 h】（低→高后
    // 画在上）：正交相机下单键"中心视深"会被画布平面偏移淹没——z=1 悬浮
    // clip 的视深贡献远小于水平偏移（文本联测实证），故以 h 为主键。
    // display 不参与排序（取景框恒最上层，见绘制段 5b）。
    std::stable_sort(prims.begin(), prims.end(),
                     [](const Prim& a, const Prim& b) {
                         if (a.h != b.h) return a.h < b.h; // 低层先画
                         return a.depth > b.depth;         // 远的先画
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
                         "[canvas-3d] paint#%d tracks=%d clips=%d prims=%zu "
                         "scale=%.3f t=(%.0f,%.0f) items=%zu\n",
                         s_n, (int)scene_->timeline.tracks.size(), nclip,
                         prims.size(), cam3d_.scale, cam3d_.tx, cam3d_.tz,
                         items.size());
            for (size_t ti = 0; ti < items.size() && ti < 4; ++ti) {
                std::fprintf(stderr,
                             "[canvas-3d]   item#%zu frame_null=%d "
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
                                 "[canvas-3d]     frame %dx%d fmt=%d "
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
                             "[canvas-3d]   clip prim depth=%.0f "
                             "center=(%.0f,%.0f)\n",
                             q.depth, q.quad[0].x(), q.quad[0].y());
            }
            std::fflush(stderr);
        }
    }

    // ---- 5. 图元绘制（仅 cue——遮挡纯靠 Z 轴排序；display 恒最上层见 5b）---
    for (const Prim& pr : prims) {
        {
            // ---- 片段（悬浮于 transform.z 高度平面）-------------------------
            const Clip&   c = scene_->timeline.tracks[pr.a].clips[pr.b];
            const QRectF  r = clipCanvasRect(c);
            // tween 求值统一入口（本片段绘制全程共用：透明度/选中标注）
            const Transform tf = clipResolvedTransform(c);
            const bool    sel = (sel_.kind == 4 && sel_.a == pr.a &&
                                 sel_.b == pr.b);
            // 类型色（与 2D 路径/时间轴同组色源——trackKindColor 单点维护）
            const QColor border = trackKindColor(
                scene_->timeline.tracks[pr.a].kind);
            // 高度吊线（类型色半透明）：地面中心 → 悬浮中心——Z 高度的
            // 空间参照（z=0 不画，贴地无位移）
            const QPointF c0 = project3(r.center().x(), 0.0, r.center().y());
            const QPointF c1 = project3(r.center().x(), (double)c.transform.z,
                                        r.center().y());
            if (c.transform.z != 0) {
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
                                 "[canvas-3d] draw#%d clip=(%s) img=%s(%dx%d) "
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
            if (pr.img && !pr.img->isNull() && r.width() > 0 && r.height() > 0) {
                const double sx = r.width()  / pr.img->width();
                const double sy = r.height() / pr.img->height();
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
                        // 变换复合同主路径：像素 →(fromScale)→ 画布局部
                        // →(tri)→ 控件（Qt 行向量 A*B = 先 A 后 B）
                        p.setTransform(QTransform::fromScale(sx, sy) * tri);
                        p.setOpacity(std::clamp(tf.opacity, 0.0, 1.0));
                        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                        p.drawImage(QPointF(0, 0), *pr.img);
                        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
                        p.restore();
                    }
                    p.setOpacity(1.0);
                } else {
                // 变换复合：【Qt 行向量约定 A*B = 先 A 后 B】图像像素 (u,v)
                // 先按 fromScale 放大到画布局部尺寸，再经 pr.tex 投影到屏幕
                //（首版写成 pr.tex * fromScale——实际先 tex 后放大，图像被
                // 推到视口外数千像素处，表现为"片段永远黑"）
                p.setTransform(QTransform::fromScale(sx, sy) * pr.tex);
                p.setOpacity(std::clamp(tf.opacity, 0.0, 1.0));
                p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                p.drawImage(QPointF(0, 0), *pr.img);
                p.setRenderHint(QPainter::SmoothPixmapTransform, false);
                p.setOpacity(1.0);
                }
            } else if (pr.deformed) {
                // 变形占位：棋盘格裁进位移后四边形（AABB 铺底 + clip）
                QPainterPath clip_path;
                clip_path.addPolygon(pr.quad);
                clip_path.closeSubpath();
                p.save();
                p.setClipPath(clip_path);
                drawChecker(p, pr.quad.boundingRect());
                p.restore();
            } else {
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
                // 选中标注：红虚线框（悬浮平面）+ 手柄（同平面投影）。
                // 变形片段（2026-11-09）：框 = 位移后四边形本体，缩放/
                // 旋转手柄隐藏（角点拖拽语义后续版本；画在未位移角会与
                // 边框分离——宁缺勿错）
                if (pr.deformed) {
                    p.setPen(QPen(QColor(0xFF, 0x33, 0x33), 1.6,
                                  Qt::DashLine));
                    p.setBrush(Qt::NoBrush);
                    p.drawPolygon(pr.quad);
                } else {
                const QPolygonF box = projectQuad(
                    r.adjusted(-2.5, -2.5, 2.5, 2.5), tf.rotation,
                    (double)c.transform.z);
                p.setPen(QPen(QColor(0xFF, 0x33, 0x33), 1.6, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                p.drawPolygon(box);
                drawResizeHandles(p, r, tf.rotation,
                                  (double)c.transform.z);
                drawRotateHandle(p, r, tf.rotation,
                                 (double)c.transform.z);
                }
            }
        }
    }

    // ---- 5b. display 绘制（恒最上层 · 取景框语义，2026-11-05 用户定案）------
    // display 没有 Z 设置、不参与 cue 遮挡排序：画在所有 cue 之后 = 恒
    // 覆盖其上；无颜色填充只画边框——屏幕范围内内容保持可见（WATCHOUT
    // Stage 取景框同风格）。在线圆点/标签/选中框为编辑器标注，随框置顶。
    for (const auto& sq : screen_quads) {
        const Screen& s = scene_->screens[sq.first];
        const bool sel  = (sel_.kind == 1 && sel_.a == sq.first);
        p.setTransform(QTransform());
        p.setPen(QPen(sel ? QColor(0xE8, 0xE8, 0xE8) : QColor(0x8A, 0x8A, 0x8A),
                      sel ? 1.8 : 1.0));
        p.setBrush(Qt::NoBrush); // 只是边框，无颜色填充——不遮挡 cue
        p.drawPolygon(sq.second);
        // 在线圆点 + 标签（投影 TL 角旁，屏幕对齐可读）
        double lx = 6;
        if (!s.player_id.empty()) {
            const bool online = display_online_.count(s.player_id) != 0 &&
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
        p.drawText(QRectF(sq.second[0].x() + lx, sq.second[0].y() + 3,
                          220, 16),
                   Qt::AlignLeft | Qt::AlignTop,
                   QString("%1 → %2")
                       .arg(QString::fromStdString(s.id),
                            s.player_id.empty()
                                ? QString("(未绑定)")
                                : QString::fromStdString(s.player_id)));
        if (sel) {
            QRectF r;
            double rot = 0.0;
            if (selectionCanvasRect(&r, &rot)) {
                // 选中虚线框（地面平面）+ 手柄
                const QPolygonF box = projectQuad(
                    r.adjusted(-2.5, -2.5, 2.5, 2.5), rot, 0.0);
                p.setPen(QPen(QColor(0xFF, 0x33, 0x33), 1.6, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                p.drawPolygon(box);
                drawResizeHandles(p, r, rot, 0.0);
                drawRotateHandle(p, r, rot, 0.0);
            }
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
    }

    // ---- 7. HUD（左上操作提示 + 右上相机读数）------------------------------
    p.setPen(QColor(0xB8, 0xB8, 0xB8));
    {
        QFont f = p.font();
        f.setPointSize(8);
        p.setFont(f);
    }
    p.drawText(QRectF(8, 6, width() - 16, 16), Qt::AlignLeft | Qt::AlignTop,
               "3D 视图 · 右键拖拽旋转 · 滚轮缩放 · 左键空白/中键平移 · "
               "双击空白复位");
    p.drawText(QRectF(8, 6, width() - 16, 16), Qt::AlignRight | Qt::AlignTop,
               QString("yaw %1°  pitch %2°  %3%")
                   .arg(cam3d_.yaw_deg, 0, 'f', 0)
                   .arg(cam3d_.pitch_deg, 0, 'f', 0)
                   .arg((int)std::lround(view3d_zoom_ * 100.0)));

    // ---- 8. 轴球视角控件（右上角；点击端点跳标准视角/球内拖拽 orbit）--------
    drawAxisGizmo(p);
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
        std::fprintf(stderr, "[canvas] 预览 GL 合成器创建（ExternalCtx）\n");
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
        if (it.frame.isNull() && !it.native) continue; // 双形态域防御
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
        } else {
            // ---- CPU 域（软解 / 硬解下载回退）----------------------------
            fv.width  = it.frame.width();
            fv.height = it.frame.height();
            fv.stride = static_cast<std::uint32_t>(it.frame.bytesPerLine());
            fv.rgba   = it.frame.constBits();
            fv.domain = FrameDomain::CpuRgba; // RGBA8888 内存序 = GL_RGBA 直传
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
                             "[canvas-dbg] compose#%d items=%zu "
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

    // ---- 屏体：绕中心顺时针旋转绘制（screen.rotation 语义；Qt y 向下
    //      坐标系中正角度即顺时针，与画布语义一致）----
    p.save();
    p.setBrush(show_body ? QBrush(QColor(0x14, 0x14, 0x14)) : Qt::NoBrush);
    p.setPen(QPen(selected ? QColor(0xE8, 0xE8, 0xE8) : QColor(0x8A, 0x8A, 0x8A),
                  selected ? 1.8 : 1.0));
    if (s.rotation != 0.0) {
        p.translate(r.center());
        p.rotate(s.rotation);
        p.drawRect(QRectF(-r.width() / 2.0, -r.height() / 2.0,
                          r.width(), r.height()));
    } else {
        p.drawRect(r);
    }
    p.restore();

    // 标签："id → player"（AABB 左上角，restore 后画保持水平可读——
    // 标签是编辑器标注而非屏内容，不随屏旋转）。
    // 【连接测试联动】已绑定的屏在标签行画在线圆点：●绿 = TCP 在线、
    // ●红 = 离线（状态来自 display_online_，EditorWindow 周期推送）——
    // 未绑定显示端的屏不画圆点（无从谈起在线与否）。
    p.setPen(QColor(0xC8, 0xC8, 0xC8));
    QFont f = p.font();
    f.setPointSize(9);
    p.setFont(f);
    int label_left = 6; // 标签左内边距（有圆点时右移让位）
    if (!s.player_id.empty()) {
        const bool online = display_online_.count(s.player_id) != 0 &&
                            display_online_.at(s.player_id);
        p.setPen(Qt::NoPen);
        p.setBrush(online ? QBrush(QColor(0x33, 0xB2, 0x6E))
                          : QBrush(QColor(0xE0, 0x55, 0x55)));
        p.drawEllipse(QPointF(r.left() + 9.0, r.top() + 9.0), 3.0, 3.0);
        p.setPen(QColor(0xC8, 0xC8, 0xC8)); // 恢复标签文字色
        p.setBrush(Qt::NoBrush);
        label_left = 17; // 让位圆点（9px 圆心 + 3px 半径 + 余量）
    }
    const QString label =
        QString("%1 → %2")
            .arg(QString::fromStdString(s.id),
                 s.player_id.empty() ? QString("(未绑定)")
                                     : QString::fromStdString(s.player_id));
    p.drawText(r.adjusted(label_left, 4, -6, 0), Qt::AlignLeft | Qt::AlignTop,
               label);

    if (selected) { // 选中标注：随旋转的虚线框 + 四角手柄 + 旋转拖柄
        drawSelectionBox(p, r, s.rotation);
        // 【画布域入参】手柄/拖柄内部对角点做 canvasToWidget 换算（与
        // 命中测试 selectionCanvasRect 同一坐标系）——此前误传控件域 r
        // 导致双重换算：画布非 1:1 缩放时柄飞离对象（红框在对象上、柄
        // 漂浮在别处）；1:1 时映射恒等故长期未暴露。
        drawResizeHandles(p, QRectF(s.x, s.y, s.w, s.h), s.rotation);
        drawRotateHandle(p, QRectF(s.x, s.y, s.w, s.h), s.rotation);
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
    if (trackInvisibleOnCanvas(scene_->timeline.tracks[tr].kind)) return;
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
                             "[clip-dbg] drawClip#%d id=%s r=(%.1f,%.1f %.1fx%.1f) "
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

    // 变形片段（2026-11-09 锚点/角点；2026-11-10 姿态 3D）：四角 = 绕锚点
    // 旋转 + 位移（2D）或 quadCorners3D 透视预投影（俯仰/偏航非 0）——
    // 画布域计算 → 控件域换算，本体绘制走多边形；默认中心锚无姿态无位移
    // = 历史路径逐位不变。【画布↔合成一致性】预览合成帧已按透视绘制
    //（ScreenMapper pose3d 链），标注层若只画 2D 矩形则改 X/Y 画面纹丝
    // 不动（用户体感"俯仰/偏航无效"）——此处与 ScreenMapper 同源唯一真相
    const bool posed    = tf.poseActive(); // 俯仰/偏航非 0（3D 透视）
    const bool deformed = posed || tf.anchor != 0 || tf.cornerActive();
    QPolygonF poly; // 控件域四角（变形分支；序 LT RT RB LB）
    if (deformed) {
        const QRectF cr = clipCanvasRect(c); // 画布域（角点数学所在域）
        double wx[4], wy[4];
        if (posed) {
            // 3D 姿态：quadCorners3D（唯一真相 CanvasGeometry——含滚转/
            // 锚点/角位移/透视缩放，与 ScreenMapper 预投影同式同参序）
            cg::quadCorners3D(cr.x(), cr.y(), cr.width(), cr.height(),
                              tf.rotation, tf.rot_x, tf.rot_y, tf.anchor,
                              tf.cornerActive() ? tf.corner : nullptr, wx,
                              wy);
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

    if (selected) { // 选中标注：随旋转的虚线框 + 四角手柄 + 旋转拖柄
        if (deformed) {
            // 变形：红虚线沿位移后四边形；手柄隐藏（同 3D 分支——角点
            // 拖拽语义后续版本，画在未位移角会与边框分离）
            p.setPen(QPen(QColor(0xFF, 0x33, 0x33), 1.6, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawPolygon(poly);
        } else {
        drawSelectionBox(p, r, tf.rotation);
        // 【画布域入参】同 drawScreen 选中分支：柄函数内部换算
        // canvasToWidget，须传画布域矩形（clipCanvasRect）——传控件域
        // r 会双重换算错位（缩放非 1:1 时柄与对象分离）
        drawResizeHandles(p, clipCanvasRect(c), tf.rotation);
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
    p.setPen(QPen(QColor(0xFF, 0x33, 0x33), 1.6, Qt::DashLine)); // 红虚线
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
// 鼠标交互
// 3D 模式：右键 = orbit 旋转会话（真位移旋转视角；无位移松开 = 菜单，
// contextMenuEvent 由 suppress_menu_ 旗标协调——Windows 右键松开即发菜单
// 事件）；中键/左键空白 = 平移（光标下世界点跟随光标，锚定闭式解）。
// 2D 模式交互骨架逐字节保留（仅坐标换算分发到 canvasFromScreen）。
// ---------------------------------------------------------------------------
void SceneCanvasWidget::mousePressEvent(QMouseEvent* ev) {
    // ---- 右键（仅 3D）：轨道旋转会话（视图操作，不改场景数据）----
    if (view3d_ && ev->button() == Qt::RightButton) {
        orbiting_       = true;
        orbit_moved_    = false;
        orbit_last_pos_ = ev->position();
        setCursor(Qt::ClosedHandCursor);
        ev->accept();
        return;
    }
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
    const QPointF wp = ev->position();

    // ---- 3D 轴球视角控件（Blender navigation gizmo）：命中优先于场景交互----
    // 端点球点击 = 跳标准视角；球体其它区域拖拽 = orbit（复用右键会话通道）
    if (view3d_) {
        const int giz = hitAxisGizmo(wp);
        if (giz >= 0) {
            applyStandardView(giz);
            ev->accept();
            return;
        }
        if (giz == -2) {
            orbiting_       = true;
            orbit_moved_    = false;
            orbit_last_pos_ = ev->position();
            setCursor(Qt::ClosedHandCursor);
            ev->accept();
            return;
        }
    }

    QRectF sel_rect;
    double sel_rot = 0.0;

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
        emit statusMessage("拖动旋转（Shift 吸附 15°）");
        return;
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
        emit statusMessage("拖动角点调整大小（对角固定）");
        return;
    }

    // ---- 常规命中：选中 + 进入位置拖动 ----
    applySelection(hitTest(wp));
    resizing_ = false; // 未进入缩放会话（命中对象走移动）
    rotating_ = false;
    resize_corner_ = -1;

    dragging_ = false;
    guide_x_.clear(); // 新会话：上一轮吸附参考线作废
    guide_y_.clear();
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
        emit statusMessage("拖动平移视图（左键空白/中键；双击空白复位）");
        return;
    }
    // 记录拖动基准（画布坐标 + 对象原始 x/y）。3D：反投影平面 = 被拖
    // 对象所在高度平面（片段 transform.z / 屏幕地面）——拖动全程对象
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
        emit statusMessage(QString("已选中屏幕 %1（拖动改位置）")
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
        emit statusMessage(
            QString("已选中片段 %1（拖动改画布位置 · Z=%2）")
                .arg(QString::fromStdString(c.id))
                .arg(rt.z));
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
    if (!dragging_ && scene_) {
        if (view3d_ && hitAxisGizmo(ev->position()) != -1) {
            // 3D 轴球（端点/球体）→ 张开手（可点跳视角/可拖拽 orbit）
            if (cursor().shape() != Qt::OpenHandCursor)
                setCursor(Qt::OpenHandCursor);
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

    if (!dragging_ || sel_.kind == 0 || !scene_) {
        QWidget::mouseMoveEvent(ev);
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
        if (sel_.kind == 4) emit clipTransformLiveChanged(); // 面板实时跟手
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
        if (sel_.kind == 4) emit clipTransformLiveChanged(); // 面板实时跟手
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
    if (snapping_) applySnap(&nx, &ny);
    if (sel_.kind == 1 && sel_.a >= 0 &&
        sel_.a < (int)scene_->screens.size()) {
        scene_->screens[sel_.a].x = nx;
        scene_->screens[sel_.a].y = ny;
    } else if (sel_.kind == 4 && sel_.a >= 0 &&
               sel_.a < (int)scene_->timeline.tracks.size() &&
               sel_.b >= 0 &&
               sel_.b < (int)scene_->timeline.tracks[sel_.a].clips.size()) {
        auto& c = scene_->timeline.tracks[sel_.a].clips[sel_.b];
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
    // 【画布→面板实时同步】拖动中工作副本已写（t.x/y 在上方落定）——
    // mouse-move 频率广播，属性面板数字跟手；同时进 display 节流下发链
    //（requestMsAutoPublish 侧 200ms 一拍——live 中 display 持续跟手，
    // 不再"松手才动"）
    if (sel_.kind == 4) emit clipTransformLiveChanged();
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
        ev->accept();
        return;
    }
    if (ev->button() == Qt::LeftButton && dragging_) {
        dragging_ = false;
        resizing_ = false;      // 缩放/旋转/移动会话统一在此收尾
        rotating_ = false;
        resize_corner_ = -1;
        guide_x_.clear();       // 吸附参考线显示随会话结束清除
        guide_y_.clear();
        // 【仅真实拖动才上报】单击选中（无位移）不算编辑——假脏会让
        // 用户关窗时被"未保存"询问、下发补丁产生无意义 diff
        if (moved_) emit sceneEdited(); // 拖动落点已写入工作副本
        moved_ = false;
        update();
    }
    QWidget::mouseReleaseEvent(ev);
}

void SceneCanvasWidget::mouseDoubleClickEvent(QMouseEvent* ev) {
    if (!scene_) {
        QWidget::mouseDoubleClickEvent(ev);
        return;
    }
    const Selection hit = hitTest(ev->position());
    if (hit.kind == 4) {
        // 片段双击 = 仅选中（applySelection）；不再弹属性面板——2026-11-05
        // 用户定案：属性面板唯一入口 = timeline 双击（画布/场景树双击弹
        // 面板与 timeline 组合面板并存易歧义，全部收敛）。
        applySelection(hit);
        return;
    }
    if (hit.kind == 1) {
        applySelection(hit);
        emit screenDoubleClicked(hit.a);
        return;
    }
    // ---- 空白双击 = 视图复位（滚轮/右键旋转/中键浏览后的快捷"回家"；
    //      原空白双击走 QWidget 默认（无副作用），行为叠加安全）----
    if (!viewIsHome()) {
        resetView();
        emit statusMessage(view3d_ ? "视图已复位（缺省视角，画布居中）"
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
    const Selection hit = hitTest(ev->pos());
    QMenu menu(this);

    if (hit.kind == 4) {
        // 片段菜单：只留删除（2026-11-05 用户定案：右键"片段属性…"与画布
        // 双击弹面板一并移除——属性面板唯一入口 = timeline 双击；删除动作
        // 在 EditorWindow——需同步时间轴）
        QAction* act_del = menu.addAction("删除片段");
        if (QAction* chosen = menu.exec(ev->globalPos())) {
            if (chosen == act_del) {
                emit clipDeleteRequested(hit.a, hit.b);
            }
        }
        return;
    }
    if (hit.kind == 1) {
        QAction* act_prop = menu.addAction("屏幕属性…");
        menu.addSeparator();
        QAction* act_del = menu.addAction("删除屏幕");
        if (QAction* chosen = menu.exec(ev->globalPos())) {
            if (chosen == act_prop) {
                applySelection(hit);
                emit screenDoubleClicked(hit.a);
            } else if (chosen == act_del) {
                emit screenDeleteRequested(hit.a);
            }
        }
        return;
    }
    // 空白：添加屏幕（单入口——自动连带登记本机显示端，用户约定"只要一个"：
    // 屏幕与显示端一一对应，分开两个入口徒增困惑。显示端登记由 EditorWindow
    // 在 onAddScreen 内完成：display_id = screen.player_id，本机默认端口）
    QAction* act_add = menu.addAction("添加屏幕");
    // 吸附对齐开关（拖动时贴齐其他图元的边缘/中心；默认开启）
    QAction* act_snap = menu.addAction("吸附对齐（边缘/中心）");
    act_snap->setCheckable(true);
    act_snap->setChecked(snapping_);
    // 视图已缩放/平移/旋转时提供复位入口（与双击空白同效；可发现性）
    QAction* act_reset = nullptr;
    if (!viewIsHome())
        act_reset = menu.addAction(view3d_ ? "重置视图（缺省视角）"
                                           : "重置视图（100%）");
    QAction* chosen = menu.exec(ev->globalPos());
    if (chosen == act_add) emit screenAddRequested();
    else if (chosen == act_snap) {
        snapping_ = act_snap->isChecked();
        emit statusMessage(snapping_ ? "吸附对齐已开启" : "吸附对齐已关闭");
    }
    else if (chosen == act_reset) {
        resetView();
        emit statusMessage(view3d_ ? "视图已复位（缺省视角，画布居中）"
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
                             "[canvas-dbg] wheel#%d mods=0x%x "
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
        emit statusMessage(
            QString("视图 %1%（3D · 右键拖拽旋转 · 左键空白/中键平移 · "
                    "双击空白复位）")
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
    emit statusMessage(QString("视图 %1%（滚轮缩放 · 左键空白/中键拖拽平移 · "
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
    emit statusMessage(QString("%1：%2 %3%4（Shift 大步进 · Alt 切移动）")
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
    auto snapAxis = [&](const std::vector<double>& refs, double pos,
                        double size, std::vector<double>* hit_out) -> double {
        double best = pos;
        double best_diff = thr;   // 命中上限即阈值（初值保证只收阈值内的）
        double best_ref  = 0.0;   // 命中的参考线值（绘制用）
        bool   found = false;
        for (const double ref : refs) {
            for (const double off : {0.0, size / 2.0, size}) { // left/center/right
                const double diff = std::fabs(ref - (pos + off));
                if (diff <= best_diff) {
                    best_diff = diff;
                    best  = ref - off; // 贴齐后的位置（锚点落在参考线上）
                    best_ref = ref;
                    found = true;
                }
            }
        }
        if (found) hit_out->push_back(best_ref); // 记录命中的参考线（去重）
        return best;
    };
    guide_x_.clear();
    guide_y_.clear();
    *nx = snapAxis(snap_ref_x_, *nx, w, &guide_x_);
    *ny = snapAxis(snap_ref_y_, *ny, h, &guide_y_);
}

} // namespace ms
