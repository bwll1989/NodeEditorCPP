// ============================================================================
//  SceneCanvasWidget.hpp —— Scene 预览画布（第四迭代核心控件，WATCHOUT 风格）
//
//  职责（editor 端两大控件之一，与时间轴并列）：
//    * 渲染：【整个预览窗口即画布】（纯黑底 = 合成输出底色，fit 铺满
//      客户区；无虚拟画布包装——无外留边/描边/坐标轴/尺寸标注）
//      + screens 矩形（绑定 display 的输出区域）
//      + 各轨道片段的画布矩形（transform；棋盘格占位 = 媒体未渲染）；
//    * 交互：单击选中（红色虚线框）、拖动移动（写回 transform.x/y 或
//      screen.x/y）、角点缩放（选中对象四角手柄，旋转感知锚定对角，
//      Shift 等比）、旋转拖柄（顶中外延圆点，拖动改 rotation）、
//      双击弹出属性面板（信号 → EditorWindow）、右键菜单；
//    * 坐标：画布 → 控件坐标 fit 缩放（保持纵横比、铺满居中）。
//
//  数据流（与时间轴同源）：
//    [scene_work_（权威工作副本）] ──setScene(指针借用)──► 本控件只读渲染
//         ▲                                              │（拖动直接改 x/y）
//         └── EditorWindow 下发补丁 ◄── sceneEdited 信号 ◄┘
//    片段的 transform 只存在于工作副本（时间轴模型不表达几何）——画布是
//    transform 的唯一编辑入口，与双击属性对话框共同覆盖几何编辑。
// ============================================================================

#pragma once

#include <QOpenGLWidget>

#include <QImage>
#include <QMap>
#include <QPolygonF>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "model/SceneModel.hpp"
#include "PreviewRenderer.hpp" // 预览渲染器（DeliverItem 嵌套类型随此公开）
#include "Camera3D.hpp"        // 3D 视口正交轨道相机（画布 3D 化 2026-11-05）

class QItemSelectionModel;
class QMenu;
class QTimer;
class QWheelEvent;
class BaseTimelineWidget; // MediaServer 收编版 QtTimeLine 控件（全局命名空间；
                          // TimeLineBase/BaseTimeLineWidget.h——选中联动信号源）

namespace ms {

class Canvas3DGl;      ///< 3D 视口贴图层（阶段1 GL化：DXT 直传+透视校正贴图）
class GlRenderer;      ///< GL 合成后端（ExternalCtx 模式；GL 直通路径用）
class MsRuntimeBridge; ///< 下发运行时桥（不拥有；宿主注入——银幕操作执行通道）
class QtTimelineModelAdapter; ///< 模型对外接口（不拥有；下发/同步委托目标）

class SceneCanvasWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    /// 选中引用（画布/宿主共用编码）：
    /// kind 0=无 1=screen 4=clip；a=screen 下标或轨道下标；b=clip 下标
    struct Selection {
        int kind = 0;
        int a = -1;
        int b = -1;
        bool operator==(const Selection& o) const {
            return kind == o.kind && a == o.a && b == o.b;
        }
    };
    explicit SceneCanvasWidget(QWidget* parent = nullptr);
    /// 析构（out-of-line：unique_ptr<GlRenderer> 需完整类型删除器——
    /// GL 对象销毁须在持有上下文的 TU 内完成）
    ~SceneCanvasWidget() override;

    /// 绑定工作副本（借用指针，生命周期归 EditorWindow）；nullptr = 无场景
    void setScene(Scene* scene);
    /// 绑定预览渲染器（所见即所得；借用指针归 EditorWindow，须先于本控件
    /// 析构；nullptr = 退回棋盘格占位画法——测试/无素材根目录场景）
    void setPreviewRenderer(PreviewRenderer* r);
    /// 外部数据变更后刷新（重绘；拖动路径内部已自刷，无需调用）
    void refresh();

    /// 【方案 A auto-key·画布编辑】（2026-11-06"打过 K 帧后画布拖不动"
    /// 修复；2026-11-09 v3 三维打包升级）：prop 已有 tween 通道 → 新值写入
    /// 该通道播放头处关键帧【一键打包全部分量】（v0/v1/v2——位置拖拽=
    /// Position(nx,ny,当前z)，防 z 被键成 0；缩放=Size 双分量；与属性面板
    /// 打包 auto-key 同语义：通道接管后画布拖动/缩放/旋转即打键，画面才能
    /// 跟手——否则拖动写静态基准被通道求值覆盖=假死），返回 true（调用方
    /// 跳过静态写）；无通道 → 返回 false（调用方照旧写静态基准 transform）。
    /// 画布不负责启用动画（菱形开关在属性面板——拖拽不创建通道，仅跟随
    /// 已启用通道）。键 t_us = 片段局部时间（钳 [0, duration]；同刻更新
    /// 保留原插值，新键 Linear 起步）。标量属性（旋转等）v1/v2 恒 0。
    /// public：单测直接驱动（键写入是纯数据操作，不依赖 GL 状态）。
    bool tweenAutoKeyWrite(Clip& c, TweenProp prop, double v0,
                           double v1 = 0.0, double v2 = 0.0);

    /// 外部选中联动（宿主：timeline 选中同步等）：高亮对应对象并重绘
    /// \param kind 1=screen 4=clip；a=screen 下标或轨道下标；b=clip 下标
    void highlight(int kind, int a, int b = -1);

    /// 视图复位：缩放回 100%（fit 适配）+ 平移归零（"回家"）。
    /// 入口：场景加载/切换（setScene 自动）、双击空白处、
    /// 画布右键菜单"重置视图"（Ctrl+滚轮/中键浏览后回家）
    void resetView();

    /// 绑定下发运行时桥（借用指针，不拥有；构造序晚于本控件，宿主建桥后
    /// 注入）。银幕操作（添加/删除/属性/连接断开）画布内闭环执行；下发/
    /// 同步菜单项委托时间轴适配器复合方法（薄壳 + statusTip 反馈）。
    void setRuntimeBridge(MsRuntimeBridge* rt) { runtime_ = rt; }
    /// 绑定时间轴适配器（模型对外接口；「下发场景」「同步媒体」委托目标。
    /// 借用指针，不拥有；仅注入一次——重复注入会叠加信号连接）。
    /// 注入即建立画布编辑→适配器的自发自收闭环（原宿主接线收编，
    /// 2026-11-16——信号的发射者与消费者都是本控件已持有的对象，
    /// 宿主只负责注入、不再中转）：
    ///  * sceneEdited（拖拽/银幕编辑落定）→ syncAllClipTweenDots（画布
    ///    auto-key 只写工作副本，时间轴菱形镜像须在此重灌）+ 预览失效 +
    ///    requestAutoPublish（节流下发汇入）；
    ///  * sceneEditedLive（拖拽会话中高频）→ requestAutoPublish（O(1)）；
    ///  * adapter sceneEdited（时间轴模型侧编辑，工作副本已变）→ 预览
    ///    失效（不落 PR——渲染器只认 Scene，不认识"模型"概念）。
    /// 实现在 .cpp（lambda 调适配器方法需完整类型，前向声明不够）。
    void setTimelineAdapter(QtTimelineModelAdapter* a);

    /// 【画布编辑→属性面板联动 2026-11-18】当前选中片段（sel_.kind==4）
    /// 的工作副本编辑广播给其所有打开中的属性面板（refreshClipPanels 按
    /// clip 精确广播、非焦点控件灌值+焦点保护自治）——补齐 2026-11-16
    /// 接线收编时丢失的腿（注释承诺"画布拖拽实时同步面板显示"但
    /// sceneEdited 处理只做了菱形重灌+下发，面板从不刷新）。
    /// 非 clip 选中/越界 = 零开销直返。实现在 .cpp。
    void notifySelectedClipPanels();

    /// 绑定时间轴控件（选中联动信号源；借用指针，不拥有，不存成员——
    /// 仅取其 sharedSelectionModel 建立连接）。时间轴选中变化 → 适配器
    /// 换算工作副本坐标 → 本画布 highlight(4, tr, cl) 高亮（单向；画布
    /// →时间轴方向不存在——原 selectionChanged 死信号已删）。原宿主接线
    /// 收编（2026-11-16）：发射源/换算/接收端三方，画布持有其二，时间轴
    /// 经本注入补齐。实现在 .cpp。
    void setTimelineWidget(BaseTimelineWidget* tw);

    /// 显示端在线状态（连接测试联动）：player_id → TCP 在线。画布据此在
    /// screen 标签旁画在线圆点（●绿在线 / ●红离线；未出现 = 离线）。
    /// 【2026-11-16 收编】由本控件 200ms UI 拍（ui_timer_）自拉 displayRows
    /// 推送（原宿主 msUiTimer 迁入）；保留公开入口便于测试注入。
    void setDisplayOnlineStates(const std::map<std::string, bool>& states);

    // ---- 画布 ↔ 控件坐标换算 --------------------------------------------
    /// 【2D fit 映射（历史语义，恒可用）】画布 → 控件（fit×zoom+pan）。
    /// 3D 模式下本映射不再描述视觉位置（视觉经 3D 投影）——3D 换算请用
    /// canvasToWidgetAt/widgetToCanvasAt（高度平面感知）
    QPointF canvasToWidget(const QPointF& p) const;
    QPointF widgetToCanvas(const QPointF& p) const;

signals:
    /// 拖动落点/右键删除等已直接写入工作副本（EditorWindow 置脏提示）
    void sceneEdited();
    /// 【画布 live 编辑→display 节流下发 2026-11-13】拖动/旋转/缩放会话中
    /// mouse-move 实时发出（工作副本已写）——经 setTimelineAdapter 注入时
    /// 建立的自连汇入适配器 requestAutoPublish 节流链（200ms 一拍），编辑
    /// 过程中 display 持续跟手。此前 live 中零信号、松手才 sceneEdited →
    /// display "停手才动"的不连续感。screen 与 clip 会话都发。高频信号，
    /// 接收端只做节流决策（O(1)）。
    void sceneEditedLive();

protected:
    /// 【GL 直通（2026-11-04）】基类换 QOpenGLWidget——绘制入口从
    /// paintEvent 变为 paintGL（Qt 提供上下文 + widget 尺寸 FBO）：
    ///   * GpuDeliver 模式：先裸 GL 调 GlRenderer(ExternalCtx) 合成最新
    ///     逐项交付帧（GPU shader 合成 + 视口子矩形贴进内容域），
    ///     随后 QPainter 标注层叠加（Qt 官方"原生 GL + QPainter"混用
    ///     模式——QPainter 在 GL 内容之上正常自绘，互不干扰）；
    ///   * SoftCompose 模式 / GL 失败回退 / 无渲染器：QPainter 路径
    ///     （黑底 + pull 帧贴图），与历史 paintEvent 逐行等价。
    /// 交互/hitTest/坐标换算零改动（仍走 widget 逻辑坐标）。
    void paintGL() override;
    /// 【诊断（2026-11-05 seek 跳帧定位）】paint 事件全程计时——QOpenGLWidget
    /// 的 swapBuffers/doneCurrent 发生在 paintGL 返回【之后】（Qt 私有实现），
    /// paintGL 内 RAII 探针测不到；此处覆写调基类前后计时，覆盖
    /// makeCurrent→paintGL→swap→doneCurrent 全链，定位"播放期 WM_TIMER
    /// 全程被挡 250ms"是否为 paint/swap 阻塞（非侵入：仅加时序观测）。
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    /// 键盘微调（方向键族）：素材/屏幕 选中后
    ///   * 方向键     = 调整大小（→/← 改宽、↓/↑ 改高；锚定左上角）
    ///   * Shift+方向 = 大步进（10 画布单位；普通步进 1 画布单位）
    ///   * Alt+方向   = 移动位置（x/y；步进约定同上）
    /// 修改直接写工作副本并 emit sceneEdited（走防抖自动下发链路）
    void keyPressEvent(QKeyEvent* event) override;
    /// 缩放浏览：Ctrl+滚轮（以光标为锚点：光标下的画布点缩放前后不动）。
    /// 【防误触】裸滚轮/触摸板双指滑动不缩放——否则"滑一下"画布就缩没
    /// （两轮用户反馈"画布缩成一小块"的共同根源）；中键平移仍可用
    void wheelEvent(QWheelEvent* event) override;

private:
    // ---- 命中与几何 ---------------------------------------------------------
    // （Selection 结构已上移至 public——宿主高亮联动需要该编码）

    /// fit 适配比例（控件像素/画布单位；不含用户缩放 view_zoom_）
    double fitScale() const;
    /// 画布区域在控件中的映射矩形（fit × 用户缩放 + 居中 + 平移；
    /// 全部坐标换算/命中/绘制/合成尺寸都基于它——缩放一处生效全链路）。
    /// 【3D 模式仅作合成尺寸基准】视觉映射走 3D 投影，本矩形不再上屏
    QRectF canvasRect() const;
    /// screen i 的控件矩形（无效下标返回空矩形）
    QRectF screenRect(int i) const;
    /// 片段 (track,clip) 的控件矩形（transform.w/h<=0 回退素材宽高再回退 640x360）
    QRectF clipRect(int track_idx, int clip_idx) const;
    /// 命中测试【2026-11-15 选中分工】screen-only：画布只选 screen（z 大者
    /// 优先 / 3D 视深最近）；clip 选中唯一入口 = 时间轴（画布"只显当前"
    /// 是舞台快照，非素材清单——快照上做全量选择必产生"找不到"困惑）。
    /// 未命中返回空选中。
    Selection hitTest(const QPointF& widget_pos) const;
    /// 点击点是否落在【当前已选中片段】（时间轴选中经直连高亮而来）的
    /// 变形感知矩形内：命中 = 画布承担该 clip 的空间编辑（拖动/手柄，
    /// "时间轴选谁，画布摆哪"）；不命中 = 按 screen 命中处理。
    /// 只显当前口径：片段此刻不活跃（画布不可见）= 不可编辑目标。
    bool hitSelectedClip(const QPointF& widget_pos) const;
    /// 片段画布矩形（画布坐标；含 w/h 兜底逻辑；内部经 clipResolvedTransform
    /// ——tween 通道在播放头时刻的求值结果直接体现为绘制/命中矩形）
    QRectF clipCanvasRect(const Clip& c) const;
    /// 片段在【当前预览播放头】时刻的有效变换（画布消费统一入口）：
    /// 有 tween 通道 → TweenEngine::resolveTransform（纯函数，暂停=冻结值
    /// 语义与预览合成同源）；无通道/无预览器 → 静态基准 transform。
    /// 绘制（prims/棋盘格/选中标注）、命中（hitTest/手柄）、吸附参考线
    /// 共用此入口 → 显示与命中天然一致，暂停态曲线编辑即时生效。
    Transform clipResolvedTransform(const Clip& c) const;
    /// 片段在预览播放头时刻是否活跃（[start, start+duration) 开区间，与
    /// 合成/TimelineEngine 可见性同口径）。【只显当前 2026-11-11 用户定案】
    /// 预览画布只显示播放头时刻活跃的素材（WATCHOUT Stage 语义）——
    /// 绘制（2D 标注/3D Prim）与命中共用此判定；preview_ 为空（无预览
    /// 运行时，纯静态查看）恒 true——无时间上下文保持全部显示。
    bool clipActiveAtPlayhead(const Clip& c) const;

    // ---- 选中统一入口（赋值 + 重绘）--------------------------------------
    /// 值未变化时静默返回（press 高频路径防重复重绘）
    void applySelection(const Selection& s);

    // ---- 自动吸附对齐（拖动时贴齐其他图元的边缘/中心线）---------------------
    /// 拖动会话启动时收集参考线：其他全部图元（screens+clips，排除被拖
    /// 对象自身）的左/中/右 x 线与上/中/下 y 线（画布坐标，升序去重）
    void collectSnapReferences();
    /// 位置拖动吸附：对移动后外接框的 left/center/right 与 top/middle/bottom
    /// 寻找阈值内的参考线（多候选取差最小者），命中则贴合并记录参考线
    /// \param nx,ny 未吸附的目标位置（画布坐标）→ 原地修正
    void applySnap(double* nx, double* ny);

    // ---- 角点缩放 / 旋转拖柄（旋转感知；几何下沉 model/CanvasGeometry.hpp）--
    /// 选中对象的画布外接框与旋转角（kind==0 返回 false）
    bool selectionCanvasRect(QRectF* r, double* rotation_deg) const;
    /// 当前选中对象所在的高度平面（画布域 h；3D 反投影/手柄投影共用）：
    /// 片段 = pos_z（深度 Z——三维空间真实 z 位置，2026-11-18 语义终定）；
    /// 屏幕 = 地面 0（屏幕是物理输出平面，始终贴地）
    double selectionPlaneH() const;
    /// 命中选中对象的角点缩放手柄（控件域 ~5px 容差）
    /// \return 局部角编码 0..3（CanvasGeometry::Corner）；未命中 -1
    int hitResizeHandle(const QPointF& widget_pos) const;
    /// 命中选中对象的旋转拖柄（顶边中点外延；控件域 ~6px 容差）
    bool hitRotateHandle(const QPointF& widget_pos) const;

    // ---- 素材画布手柄（2026-11-20 交互优化）：四角 = 角点位移拖拽
    //      （corner pin，写 Transform.corner）、四边 = 尺寸缩放（写
    //      w/h，对边固定）——与属性面板数值实时联动 ---------------
    /// 选中片段的视觉四角（画布域；透视投影含滚转/锚点/角点/深度/
    /// 全场透视——与绘制/合成同源唯一真相）
    QPolygonF clipVisualQuadCanvas(const Clip& c, const Transform& tf) const;
    /// 命中片段角点手柄（控件域 ~8px 容差）\return CornerIdx 0..3，未中 -1
    int hitClipCornerHandle(const QPointF& widget_pos) const;
    /// 命中片段边线（控件域 ~6px 容差；角点 14px 让位区除外）
    /// \return 边编码 0=上 1=右 2=下 3=左，未中 -1
    int hitClipEdgeHandle(const QPointF& widget_pos) const;
    /// 片段手柄绘制（四边线 + 四角方块；hover/拖动高亮）
    void drawClipHandles(QPainter& p, const QPolygonF& quad_widget) const;
    /// 角点拖拽会话 tick：鼠标画布点 − 基准角 = 新角点位移（auto-key
    /// 双模；面板「形变」行实时灌值）
    void tickClipCornerDrag(const QPointF& canvas_pos);
    /// 边线缩放会话 tick：沿被拖边法向投影增量 → 新 w/h，对边中点回贴
    /// 锚定（任意旋转/锚点/姿态下对边视觉固定）
    void tickClipEdgeDrag(const QPointF& canvas_pos);
    /// 命中全局消隐点 VP 手柄（3D 视口地面标记；仅 persp_focal>0 时
    /// 存在——未配置时 VP 走画布派生默认，拖动无意义）
    bool vpHandleHit(const QPointF& widget_pos) const;
    /// 命中 Eye 圆环（VP 正上方 F 高度处；点它 = 选中 VP——Eye 与 VP
    /// 是同一全局透视参数的两个伴生标记，拖动归 VP 自由拖会话）
    bool eyeHandleHit(const QPointF& widget_pos) const;

    // ---- 移动 Gizmo（Blender 式三轴箭头；3D 视口选中对象位移）----------
    /// 选中对象名义矩形中心（画布域）+ 高度（clip=pos_z，screen=0 贴地）。
    /// 无效选中返回 false。
    bool    moveGizmoAnchor(double& cx, double& h, double& cy) const;
    /// 轴"世界 1 单位"的屏幕投影向量（正交轨道相机仿射——长度即灵敏度）。
    /// 轴向：0=+画布X（红） 1=+画布Y（绿，轴球绿球同标签） 2=+高度（蓝）
    QPointF moveGizmoAxisVec(int axis, double cx, double h, double cy) const;
    /// 移动 Gizmo 轴命中（返回轴 0/1/2，未中 -1；屏幕恒长轴杆+锥一域）
    int     hitMoveGizmo(const QPointF& widget_pos) const;
    /// 绘制三轴箭头（paintGL3D 尾段调用；恒最上层，hover/拖动轴高亮）
    void    drawMoveGizmo(QPainter& p) const;
    /// Gizmo 通用命中核心（对象 Gizmo 与 VP Gizmo 共用轴域判定）——
    /// 沿轴距离 ∈ [min_t, 74] 且垂距 ≤ 10px；min_t = 中心让位半径
    ///（对象版 -10 全域、VP 版 9 = 中心圈归自由拖）
    int     hitGizmoAxes(const QPointF& widget_pos, double cx, double h,
                         double cy, int max_axis, double min_t) const;
    /// Gizmo 通用绘制核心（轴杆+锥形箭头+标签+中心圈；轴向量经
    /// moveGizmoAxisVec 以锚点求取，退化轴跳过）
    void    drawGizmoAxes(QPainter& p, double cx, double h, double cy,
                          int max_axis, int drag_axis, int hover_axis) const;
    /// VP Gizmo 轴命中（2026-11-18 用户反馈“VP 的移动没有这个 Gizmo”）
    /// ——锚点 = VP 地面标记，X/Y 两轴 + 蓝 Z（Eye 高度 F，VP 贴地无 Z
    /// 而 Eye 有——蓝轴拖 F 与 Eye 环升降闭环）；**选中 VP 后才显示/可
    /// 命中**（与其他对象 Gizmo 同规则，2026-11-18 用户定案）
    int     hitVpGizmo(const QPointF& widget_pos) const;
    /// 绘制 VP 三轴 Gizmo（选中 VP 且 F>0 时；蓝 Z 轴拖动 = Eye 高度 F）
    void    drawVpGizmo(QPainter& p) const;
    /// 全局透视（Eye 点）编辑对话框：焦距 F + 消隐点 X/Y（对标
    /// WATCHOUT Stage Perspective）——OK 写回 Scene + 重合成 + sceneEdited
    void openEyeDialog();

    // ---- 绘制子过程 ---------------------------------------------------------
    /// 【GL 直通合成】GpuDeliver 交付帧经 GlRenderer(ExternalCtx) 合成进
    /// 本 widget 的 FBO（惰性建 renderer；失败置 failed 走 QPainter 回退）。
    /// \return true = GL 已合成（背景+内容），QPainter 只画标注层；
    ///         false = GL 失败（QPainter 走逐项 drawImage 回退全量画法）
    bool composeGpuFrame(const std::vector<PreviewRenderer::DeliverItem>& items,
                         int comp_w, int comp_h, const QRectF& domain);
    void drawChecker(QPainter& p, const QRectF& r) const;   // 棋盘格占位
    /// \param show_body false = 不填屏体底色（真实合成帧已铺在下面，只描边标注）
    void drawScreen(QPainter& p, int i, bool selected, bool show_body) const;
    /// \param show_checker false = 不画棋盘格（真实合成帧已含片段画面）
    void drawClip(QPainter& p, int tr, int cl, bool selected,
                  bool show_checker) const;
    /// 红色虚线选中框（rotation!=0 时框随对象旋转——与选中对象姿态一致）
    void drawSelectionBox(QPainter& p, const QRectF& r,
                          double rotation_deg) const;
    /// 选中对象四角缩放手柄（实心方块画在旋转后的角点上）
    /// \param plane_h 手柄所在高度平面（3D 投影用；2D 恒 0）
    void drawResizeHandles(QPainter& p, const QRectF& r,
                           double rotation_deg, double plane_h = 0.0) const;
    /// 旋转拖柄（顶中外延圆点 + 连线；画在旋转后位置，控件域恒定大小）
    /// \param plane_h 柄所在高度平面（3D 投影用；2D 恒 0）
    void drawRotateHandle(QPainter& p, const QRectF& r,
                          double rotation_deg, double plane_h = 0.0) const;

    // ---- 3D 视口（2026-11-05 画布 3D 化；MS_CANVAS_3D=0 回退 2D）------------
    /// 3D 换算：画布点 p（位于高度 h 平面）→ 控件点。
    /// 2D 模式 = canvasToWidget（h 忽略）；3D = Camera3D 正交投影
    QPointF canvasToWidgetAt(const QPointF& p, double h) const;
    /// 3D 换算：控件点 → 视线与高度 h 水平面的交点（画布域 x/y）。
    /// 2D 模式 = widgetToCanvas（h 忽略）
    QPointF widgetToCanvasAt(const QPointF& wp, double h) const;
    /// 交互换算分发：拖动会话固定在 drag_plane_h_ 平面反投影（按下时冻结
    /// ——拖动全程鼠标射线与对象所在平面求交，几何增量语义与 2D 一致）
    QPointF canvasFromScreen(const QPointF& wp) const;
    /// 3D 世界点直接投影（x=画布x，h=高度，z=画布y）——画网格/轴线用
    /// （含高度差线段，canvasToWidgetAt 的双参数形式覆盖不了竖直线）
    QPointF project3(double x, double h, double z) const;
    /// 3D 平移锚定：调整注视点使 widget 位置的地面点恒等于 world
    ///（中键/左键空白平移 + 滚轮缩放共用——光标下世界点跟随光标）
    void panAnchorTo(const QPointF& widget_pos, const QPointF& world);
    /// 3D 渲染主路径（paintGL 分派；items = GpuDeliver 逐项交付，可空——
    /// 空则全片段棋盘格占位，布局与标注照常）
    /// \param comp_w/comp_h/dom 合成域尺寸 + 画布域矩形（仅用于给 Display
    ///        面片贴"全局 Eye 总画面"——把交付项合成一张 QImage，各屏按
    ///        窗口 UV 裁切；与片段面片贴图链正交）
    void paintGL3D(const std::vector<PreviewRenderer::DeliverItem>& items,
                   int comp_w, int comp_h, const QRectF& dom);
    /// 3D 图元四角（画布域，绕中心转 rotation 后）投影成屏幕四边形。
    /// \param anchor 锚点编码（Transform.anchor：0=中心默认——历史路径
    ///               逐位不变；1=左上 2=右上 3=右下 4=左下）
    /// \param corner 角点位移数组（画布 px，序 LT/RT/RB/LB ×(dx,dy)）；
    ///               nullptr = 无位移（screen 恒传 nullptr）
    QPolygonF projectQuad(const QRectF& r, double rotation_deg, double h,
                          int anchor = 0,
                          const double* corner = nullptr) const;
    /// 片段四角（画布域）：绕锚点旋转 + 角点位移——薄转发
    /// cg::quadCornersWithAnchor（唯一真相在 CanvasGeometry.hpp，
    /// core_tests 直测共享数学；2026-11-09 X 形根治）。
    /// 角序 0=LT 1=RT 2=RB 3=LB（Transform::CornerIdx 同构）；corner 可空。
    void clipQuadCorners(const QRectF& r, double rotation_deg, int anchor,
                         const double* corner, QPointF out[4]) const;
    /// 视图是否处于"回家"状态（双击空白复位判定 + 右键菜单"重置视图"显隐）
    bool viewIsHome() const;
    /// 右上角轴球视角控件（Blender navigation gizmo 同位）绘制：
    /// 三轴投影方向线 + 端点球（正端实心带字母/负端空心），点击端点
    /// 跳标准视角，球内拖拽 orbit
    void drawAxisGizmo(QPainter& p) const;
    /// 轴球命中：返回端点编号 0..4（+X/−X 画布左右、+Z 高度顶视、
    /// ±Y 画布正/背面——命名=属性面板语义，Z=高度）、球体其它区 = −2、
    /// 未命中 = −1（编号→视角映射见 applyStandardView）
    int  hitAxisGizmo(const QPointF& wp) const;
    /// 跳转标准视角（保持注视点与缩放；仅改 yaw/pitch）
    void applyStandardView(int axis_end);
    /// 银幕右键「连接/断开显示端」执行：按 screen_idx 解析
    /// display_id = screen.player_id → 桥 setDisplayEnabled（停用立即断开；
    /// 启用交看门狗重连；在线角标随宿主 200ms 拍推送刷新）
    void setDisplayConnect(int screen_idx, bool connect_);
    /// 右键空白「添加屏幕」（唯一入口；工具条按钮已退役）：
    /// id/player_id 自动
    /// 化（scr-N 线性探测防删屏重名 / player-N 与屏号对齐）+ 默认几何
    /// 960×540 阶梯排布 + 显示端自动连带登记（已登记同名沿用——幂等）。
    /// 写工作副本 + 全链刷新 + qDebug 留痕。改绑/远程显示端走屏幕属性对话框。
    /// 【2026-11-18 统一】2D/3D 降格为创建预设而非类型——底层只有一种
    /// Screen（取景窗 + 可选 3D 姿态 rot_x/rot_y）；新建默认零姿态 = 旧
    /// 2D 行为，斜装/立面在属性对话框调俯仰/偏航。投影仪视锥（Eye/Target/
    /// Lens Shift）作为字段扩展在统一模型上做，不另立 3D 类型。
    void addScreen();
    /// 银幕右键「删除屏幕」：从工作副本移除 + 全链刷新。显示端登记不连带
    /// 删除（登记无害可复用；改绑/断开走银幕右键与属性对话框）。
    void deleteScreen(int screen_idx);
    /// 交互提示统一出口（状态栏已退役——qDebug 直印对接 spdlog 日志存留）
    void statusTip(const QString& text);
    /// 银幕右键「下发场景」：委托适配器 publishScene 复合操作（兜底同步 →
    /// 音频快照 → 全量广播），失败仅留痕忽略。
    void publishScene();
    /// 银幕右键「同步媒体」：委托适配器 syncMedia 复合操作（兜底同步 →
    /// 素材清单分发）；注册数/在线显示端数经 statusTip 反馈。
    void syncMedia();
    /// 屏幕属性对话框（银幕双击/右键「屏幕属性…」唯一入口）：模态 exec +
    /// WATCHOUT 式三页（通用/几何结构/绑定）。实时预览两条链（warp + 几何
    /// 六字段）编辑中即时写工作副本并刷新视图，取消 = 还原打开时备份；
    /// OK = applyTo + 显示端登记同步（改绑注销旧条目；变化重登记）。
    void openScreenProperty(int screen_idx);
    /// 视图刷新 + display 节流下发（sceneEditedLive 走适配器 requestAutoPublish 节流链）
    void refreshViews();

    // ---- 状态 ---------------------------------------------------------------
    Scene* scene_ = nullptr;   // 工作副本（不拥有；EditorWindow 拥有）
    PreviewRenderer* preview_ = nullptr; // 预览渲染器（不拥有；可空=占位画法）
    MsRuntimeBridge* runtime_ = nullptr; // 下发运行时桥（不拥有；银幕操作执行通道）
    QtTimelineModelAdapter* adapter_ = nullptr; // 模型对外接口（不拥有；下发/同步委托目标）
    /// 显示端在线状态（player_id → TCP 在线；ui_timer_ 200ms 拍自拉刷新）
    std::map<std::string, bool> display_online_;
    /// 上一拍在线显示端集合（connected && enabled 口径）：与当拍对比检出
    /// "新上线"→ 自动追平下发场景（覆盖 display 晚于项目打开上线路径）
    std::set<std::string> last_connected_;
    // ---- 视图缩放/平移（用户浏览操作；不写场景数据）----
    /// 用户缩放系数（1.0 = fit 适配 100%；0.1~16.0）。叠加在 fit 比例上：
    /// 实际比例 = fitScale() × view_zoom_（fitScale 随窗口自适应，zoom 保值）
    /// 【仅 2D 回退路径使用（MS_CANVAS_3D=0）；3D 缩放走 cam3d_.scale】
    double  view_zoom_ = 1.0;
    /// 视图平移偏移（控件域；fit 居中位置之外的额外位移）【仅 2D 回退路径】
    QPointF view_pan_{0, 0};
    bool    panning_ = false;            ///< 平移会话进行中（中键 或 左键空白
                                         ///  ——2026-11-05 空白左键拖拽平移）
    QPointF pan_start_widget_{0, 0};     ///< 平移会话起始光标位置（控件域）
    QPointF pan_start_view_{0, 0};       ///< 平移会话起始视图偏移（2D）
    QPointF pan_anchor_world_{0, 0};     ///< 平移锚点（3D：光标下地面世界点
                                         ///  ——按下时冻结，移动中保持跟随）
    // ---- 3D 视口（2026-11-05 画布 3D 化；MS_CANVAS_3D=0 回退历史 2D）--------
    /// 3D 视口开关：缺省开（用户定案"整个画布预览控件改为三维的"）；
    /// 环境变量 MS_CANVAS_3D=0 回退 2D fit 视图（排障/回归对照用）
    bool     view3d_       = true;
    Camera3D cam3d_;                     ///< 正交轨道相机（yaw/pitch/scale/注视点）
    /// 3D 缩放倍率（相对 fitScale 基准，0.1~16；与 2D view_zoom_ 同构）。
    /// 【勿把绝对比例快照进 cam3d_.scale】setScene 在窗口布局前触发 resetView，
    /// 此时 fitScale 是小窗口值，快照会在窗口就位后永远偏小（首版踩坑：
    /// HUD 显示 40% 且视口过挤）。绝对比例 = fitScale × view3d_zoom_，
    /// 每帧 paint/交互前同步进 cam3d_.scale——窗口 resize 自动跟随。
    double   view3d_zoom_  = 1.0;
    bool     orbiting_      = false;    ///< 右键轨道旋转会话进行中
    bool     orbit_moved_   = false;    ///< 本次右键会话发生真实位移
                                         ///  （真位移=旋转视图；无位移=弹菜单）
    bool     suppress_menu_ = false;    ///< 轨道旋转刚结束 → 吞掉随后的
                                         ///  contextMenuEvent（Windows 右键
                                         ///  松开即发菜单事件，不吞则拖完弹菜单）
    QPointF  orbit_last_pos_{0, 0};     ///< 轨道会话上一点（增量计算）
    bool     dragging_vp_  = false;    ///< 全局消隐点 VP 拖动会话进行中
                                     ///  （3D 视口地面标记；改 persp_vp_x/y）
    // ---- 移动 Gizmo（Blender 式；3D 视口轴约束拖动会话/悬停态）----------
    int     gizmo_drag_axis_  = -1; ///< 轴约束拖动会话轴（0=画布X 1=画布Y
                                    ///  2=高度 pos_z；-1=无会话）
    int     gizmo_hover_axis_ = -1; ///< 悬停高亮轴（非会话时；重绘加粗）
    double  gizmo_orig_z_     = 0.0; ///< 高度轴会话起始 pos_z
    QPointF gizmo_axis_scr_{0, 0};   ///< 被拖轴"世界 1 单位"屏幕向量
                                     ///  （按下时冻结；拖动全程相机不变）
    QPointF gizmo_press_widget_{0, 0}; ///< 会话按下点（widget 域——屏幕
                                       ///  delta 投影到轴方向的基准）
    // ---- VP Gizmo（VP 选中态三轴；轴约束会话与对象 Gizmo 互斥）---------
    bool    vp_selected_   = false; ///< VP 选中态（点 VP 十字/Eye 环选中；
                                    ///  点对象/空白/时间轴换选取消）——Gizmo
                                    ///  仅选中时显示（与其他对象同规则）
    int     vp_drag_axis_  = -1;   ///< VP 轴约束会话轴（0=画布X 1=画布Y
                                    ///  2=Eye 高度 F；-1 = 中心圈自由拖）
    int     vp_hover_axis_ = -1;   ///< VP Gizmo 悬停高亮轴（非会话时）
    double  vp_orig_x_     = 0.0;  ///< VP 会话起始 persp_vp_x（轴约束基准）
    double  vp_orig_y_     = 0.0;  ///< VP 会话起始 persp_vp_y
    double  vp_orig_f_     = 0.0;  ///< 蓝轴会话起始 persp_focal（Eye 高度）
    double   drag_plane_h_ = 0.0;       ///< 拖动会话高度平面（画布域 h；
                                         ///  按下时冻结=对象所在平面，全程
                                         ///  反投影到该平面求画布坐标）
    // 【第八迭代：拉帧泵】预览窗口定时从引擎拉最新帧（对齐 display 端
    // "渲染循环独立、窗口只拉帧"）——paintEvent 不再驱动渲染、不管时间
    QTimer*  pull_timer_  = nullptr; ///< ~33ms 拉帧泵（tick+pump+pull）
    /// 200ms UI 拍（2026-11-16 自宿主 msUiTimer 收编）：拉 displayRows →
    /// 在线角标刷新 + 新上线 display 检出自动追平下发。runtime_ 未注入/
    /// 未启动时空转（一行判空）。
    QTimer*  ui_timer_    = nullptr;
    QImage   latest_frame_;          ///< 最近拉到的合成帧（paint 直接画）
    qint64   latest_frame_key_ = 0;  ///< 帧变化检测（QImage::cacheKey）
    /// 最近帧的内容域（画布坐标；预览虚拟屏=内容包围盒，起点可为负——
    /// 负坐标素材/出画布屏幕也进域）。贴图矩形 = 域经 fit 映射到控件；
    /// 与 latest_frame_ 同源更新（pull 一并带出）
    QRectF   frame_domain_{0, 0, 0, 0};
    // ---- GL 直通（GpuDeliver 模式；paintGL 消费）----------------------------
    std::unique_ptr<GlRenderer> gl_; ///< GL 合成器（ExternalCtx；惰性创建——
                                     ///  须 paintGL 内上下文 current 时构造）
    /// 3D 视口贴图层（阶段1 GL 化 2026-11-17）：片段面片裸 GL 绘制——
    /// HAP DXT 压缩直传（worker 软解退役的前提）+ 单应透视校正（真 3D
    /// 姿态画面）。惰性创建于首个有贴图的面片（上下文 current 时）；
    /// MS_CANVAS_GL3D=0 会话级禁用（QPainter 原路径兜底）
    std::unique_ptr<Canvas3DGl> gl3d_;
    Selection sel_;            // 当前选中
    // 拖动状态（press 命中后才激活；缩放/旋转会话 = dragging_ && 对应标志）
    bool   dragging_          = false;
    bool   resizing_          = false;  // true = 角点缩放（false = 位置拖动）
    bool   rotating_          = false;  // true = 旋转拖柄拖拽会话
    int    resize_corner_     = -1;     ///< 被拖角编码（CanvasGeometry 0..3）
    // ---- 素材手柄会话（2026-11-20）：四角角点位移 / 四边尺寸缩放 -------
    int    corner_drag_       = -1;     ///< 角点拖拽会话（CornerIdx 0..3）
    int    edge_drag_         = -1;     ///< 边线缩放会话边（0=上 1=右 2=下
                                        ///  3=左；-1 = 非边线会话）
    int    corner_hover_      = -1;     ///< 角点手柄悬停高亮（非会话）
    int    edge_hover_        = -1;     ///< 边线悬停高亮
    double edge_orig_x_       = 0.0;    ///< 边线会话冻结的标称矩形（画布域）
    double edge_orig_y_       = 0.0;
    double edge_orig_w_       = 0.0;
    double edge_orig_h_       = 0.0;
    double edge_orig_rot_     = 0.0;    ///< 会话冻结姿态（滚转/俯仰/偏航/锚点）
    double edge_orig_rx_      = 0.0;
    double edge_orig_ry_      = 0.0;
    int    edge_orig_anchor_  = 0;
    QPointF edge_dir_x_{0, 0};          ///< 局部 x 轴单位向量（画布域，冻结）
    QPointF edge_dir_y_{0, 0};          ///< 局部 y 轴（向下）单位向量（冻结）
    QPointF edge_opp_mid_{0, 0};        ///< 对边中点（画布域，冻结——回贴锚）
    double resize_rotation_   = 0.0;    ///< 缩放会话冻结的旋转角（度）
    double resize_orig_x_     = 0.0;    ///< 会话起始外接框（画布坐标，未旋转）
    double resize_orig_y_     = 0.0;
    double resize_orig_w_     = 0.0;
    double resize_orig_h_     = 0.0;
    double rotate_orig_rot_   = 0.0;    ///< 旋转会话起始 rotation（度）
    double rotate_start_ang_  = 0.0;    ///< 按下点相对中心的方向角（度）
    bool   moved_             = false;  // 本次按下后发生过真实位移
                                          //（防"单击即 sceneEdited"假脏）
    QPointF drag_start_canvas_{0, 0};   // 按下点（画布坐标）
    double  drag_orig_x_ = 0.0;         // 被拖对象原始 x（画布坐标）
    double  drag_orig_y_ = 0.0;
    // ---- 自动吸附对齐（拖动时贴齐其他图元）----
    bool              snapping_ = true; ///< 吸附开关（右键菜单可切换）
    std::vector<double> snap_ref_x_;    ///< 参考线 x 集合（画布坐标，升序）
    std::vector<double> snap_ref_y_;    ///< 参考线 y 集合（画布坐标，升序）
    std::vector<double> guide_x_;       ///< 本次命中的吸附线 x（绘制品红竖线）
    std::vector<double> guide_y_;       ///< 本次命中的吸附线 y（绘制品红横线）
    // 【预览透视开关 preview_perspective_ 已删除 2026-11-18】正交视图
    // 功能及其函数全退役——预览恒与 display 同源（透视）。【2026-11-20
    // 用户拍板】素材三轴旋转 = 透视投影压平到 XY 平面（quadCorners3DP
    // + Scene::effectivePersp，与 display ScreenMapper 同源）；站立路径
    //（quadCorners3DWorld）退役无消费者。
};

} // namespace ms
