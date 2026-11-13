#pragma once

#include <QAbstractItemView>
#include <QPointer>
#include <functional>
#include <unordered_map>
#include "TimeLineDefines.h"
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QFileInfo>
#include <QMimeData>
#include "BaseTimeLineModel.h"
#include "TimeLineStyle.h"
#include "BaseTrackDelegate.h"
#include "DefaultTimeLineToolBar.h"
#include "QMenu"
#include "TimelineExports.hpp"
// TimelineView类继承自QAbstractItemView
class NODE_TIMELINE_PUBLIC BaseTimelineView : public QAbstractItemView
{
Q_OBJECT
    // 允许通过 QSS 设置颜色：qproperty-<name>: <color>
    Q_PROPERTY(QColor timelineBgColor READ timelineBgColor WRITE setTimelineBgColor)
    Q_PROPERTY(QColor timelineSeparatorColor READ timelineSeparatorColor WRITE setTimelineSeparatorColor)
    Q_PROPERTY(QColor timelineRulerColor READ timelineRulerColor WRITE setTimelineRulerColor)
    Q_PROPERTY(QColor timelinePlayheadColor READ timelinePlayheadColor WRITE setTimelinePlayheadColor)
    Q_PROPERTY(QColor clipFillColor READ clipFillColor WRITE setClipFillColor)
    Q_PROPERTY(QColor clipBorderColor READ clipBorderColor WRITE setClipBorderColor)
    Q_PROPERTY(QColor trackSelectedColor READ trackSelectedColor WRITE setTrackSelectedColor)
public:
    // 构造函数，初始化视图模型
    explicit BaseTimelineView(BaseTimeLineModel *viewModel, QWidget *parent = nullptr);
    ~BaseTimelineView() override ;
    /**
     * 返回给定索引的可视矩形
     * @param QModelIndex index 索引
     * @return QRect 可视矩形
     */
    virtual QRect visualRect(const QModelIndex &index) const override;
    /**
     * 滚动到指定索引
     * @param QModelIndex index 索引
     * @param ScrollHint hint 滚动提示
     */
    virtual void scrollTo(const QModelIndex &index, ScrollHint hint) override{};

    /**
     * 返回给定点的索引
     * @param QPoint point 点
     * @return QModelIndex 索引
     */
    virtual QModelIndex indexAt(const QPoint &point) const override;
    /**
     * 显示剪辑属性
     * @param QModelIndex index 片段索引
     */
    virtual void showClipProperty(const QModelIndex& index);

    virtual void initToolBar(DefaultTimeLineToolBar* toolbar);
    // 时间线工具栏
    DefaultTimeLineToolBar* m_toolbar = nullptr;

    // ---- 片段拖拽磁性吸附（2026-11-09 用户需求）--------------------------
    /// 开/关磁性吸附（默认开）：拖动或裁边片段时，被拖边缘接近其他片段
    /// 的入/出点或时间线原点（阈值内）即自动对齐，并显示对齐参考线。
    void setSnapEnabled(bool on) { m_snapEnabled = on; }
    bool snapEnabled() const { return m_snapEnabled; }
    /// 当前对齐参考线的帧位置（-1 = 未吸附；测试与调试观察锚）
    int snapLineFrame() const { return m_snapLineFrame; }
    /// 当前像素/帧（视图实例时间刻度；测试做帧↔像素换算用，避免复制公式）。
    /// 命名避让 TimeLineStyle.h 的全局变量 timescale（同名方法会让成员
    /// 初始化 m_timescale = timescale 解析成成员函数指针 → C3867）。
    double pxPerFrame() const { return m_timescale; }  // 【2026-11-10 微秒原生】语义 = 像素/μs（名字保留避免波及）
    // ---- 外控/测试辅助（2026-11-10 滚动探针；只读，不破坏"滚动条唯一真值"纪律）
    double currentScaleRatio() const { return currentScale; }
    QPoint scrollOffset() const { return m_scrollOffset; }
    /// 滚动偏移访问器（P7 统一绘制路径：直接读滚动条真值，替代散落的
    /// horizontalScrollBar()->value()/verticalScrollBar()->value() 直呼与
    /// 过期的 m_scrollOffset 缓存读点）
    int scrollX() const { return horizontalScrollBar()->value(); }
    int scrollY() const { return verticalScrollBar()->value(); }

public slots:
    /**
     * 更新可视区域（原方法）
     * 优化：改为调用 scheduleRedraw 合并重绘，避免每次信号都立即执行完整重绘。
     */
    virtual void onUpdateViewport();
    // 播放头移动：缓存不失效，只刷新旧位置和新位置
    virtual void onPlayheadMoved(qint64 us); // 播放头（μs；2026-11-10 微秒原生）
    // 片段几何或外观变化：只重画该片段的旧位置和新位置
    void updateClipGeometry(const QModelIndex& index);
    /**
     * 更新滚动条
     */
    virtual void updateScrollBars();
    /**
     * 滚动内容
     * @param int dx 水平滚动
     * @param int dy 垂直滚动
     */
    virtual void scrollContentsBy(int dx, int dy) override;
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) override;
    /**
     * 设置缩放（锚点=播放头，兼容既有调用方语义）
     * @param double value 归一化缩放值 [0,1]
     */
    virtual void setScale(double value);
    /**
     * 设置缩放并保持锚点时刻的视口位置不变（2026-11-10 缩放焦点修复）
     * @param double value 归一化缩放值 [0,1]
     * @param qint64 anchorUs 锚点时刻（μs）——缩放前后该时刻在视口中的像素位置不变
     */
    virtual void setScaleAt(double value, qint64 anchorUs);
    /**
     * U6（timeline-draw-zoom-review）：像素锚点缩放——直接保持"锚点视口
     * 像素位置不动"，消掉 μs 中间取整（pointToFrame qRound64 →
     * anchorUs*ts qRound 双重取整曾致连续缩放每步 ±0.5px 蠕动）。
     * @param double ratio 归一化缩放值 [0,1]
     * @param int anchorViewportX 锚点的视口 x 像素
     */
    virtual void setScaleAtPx(double ratio, int anchorViewportX);

    /**
     * 在指定位置添加剪辑
     * @param QModelIndex index 索引
     * @param QPoint pos 位置
     */
    virtual void addClipAtPosition(const QModelIndex& index, const QPoint& pos);


protected:
    /**
     * 移动光标
     * @param CursorAction cursorAction 光标动作
     * @param Qt::KeyboardModifiers modifiers 键盘修饰符
     * @return QModelIndex 索引
     */
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override {return QModelIndex();}

    /**
     * 水平偏移
     * @return int 水平偏移
     */
    virtual int horizontalOffset() const override{return 0;}

    /**
     * 垂直偏移
     * @return int 垂直偏移
     */
    virtual int verticalOffset() const override{return 0;}

    /**
     * 是否隐藏索引
     * @param QModelIndex index 索引
     * @return bool 是否隐藏
     */
    virtual bool isIndexHidden(const QModelIndex &index) const override{ return false; }

    /**
     * 设置选择
     * @param QRect rect 矩形
     * @param QItemSelectionModel::SelectionFlags command 命令
     */
    virtual void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override{}

    /**
     * 设置选择区域
     * @param QItemSelection selection 选择区域
     * @return QRegion 区域
     */
    virtual QRegion visualRegionForSelection(const QItemSelection &selection) const override {return QRegion();}

    /**
     * 绘制事件
     * @param QPaintEvent *event 绘制事件
     */
    virtual void paintEvent(QPaintEvent *event) override;

    /**
     * 鼠标按下事件
     * @param QMouseEvent *event 鼠标事件
     */
    virtual void mousePressEvent(QMouseEvent *event) override;

    /**
     * 鼠标移动事件
     * @param QMouseEvent *event 鼠标事件
     */
    virtual void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * 鼠标释放事件
     * @param QMouseEvent *event 鼠标事件
     */
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    /**
     * 鼠标双击事件
     * @param QMouseEvent *event 鼠标事件
     */
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    /**
     * 鼠标离开事件
     * @param QEvent *event 事件
     */
    virtual void leaveEvent(QEvent *event) override;

    // bool eventFilter(QObject *watched, QEvent *event) override;

    /**
     * 上下文菜单事件
     * @param QContextMenuEvent* event 上下文菜单事件
     */
    virtual void contextMenuEvent(QContextMenuEvent* event) override;
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    /**
     * 调整大小事件
     * @param QResizeEvent *event 调整大小事件
     */
    virtual void resizeEvent(QResizeEvent *event) override;

    /**
     * 模型行即将删除（P6：片段/轨道删除时清空内容矩形缓存，防止
     * m_clipContentRects 里残留已销毁 AbstractClipModel* 的悬垂键）
     */
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;

    /**
     * U5：边缘横滚方向/步长判定（深度分档 ±12/±24/±36），拖拽与播放头
     * 拖拽共用；启停判定留在鼠标事件流内（合成事件零干扰契约）。
     */
    void updateEdgeScrollDir();

    /**
     * 显示事件
     * @param QShowEvent *event 显示事件
     */
    virtual void showEvent(QShowEvent *event) override;

    /**
     * 拖入事件
     * @param QDragEnterEvent *event 拖入事件
     */
    virtual void dragEnterEvent(QDragEnterEvent *event) override;

    /**
     * 拖动移动事件
     * @param QDragMoveEvent *event 拖动移动事件
     */
    virtual void dragMoveEvent(QDragMoveEvent *event) override;

    /**
     * 拖动释放事件
     * @param QDropEvent *event 拖动释放事件
     */
    virtual void dropEvent(QDropEvent *event) override;

    /**
     * 滚轮事件
     * @param QWheelEvent *event 滚轮事件
     */
    virtual void wheelEvent(QWheelEvent *event) override;
    /**
     * 获取时间线背景色
     * @return QColor 背景色
     */
    QColor timelineBgColor() const { return m_timelineBgColor; }
    /**
     * 设置时间线背景色
     * @param QColor color 背景色
     */
    void setTimelineBgColor(const QColor& color);
    /**
     * 获取时间线分隔线颜色
     * @return QColor 分隔线颜色
     */
    QColor timelineSeparatorColor() const { return m_timelineSeparatorColor; }
    /**
     * 设置时间线分隔线颜色
     * @param QColor color 分隔线颜色
     */
    void setTimelineSeparatorColor(const QColor& color);
    /**
     * 获取标尺颜色
     * @return QColor 标尺颜色
     */
    QColor timelineRulerColor() const { return m_timelineRulerColor; }
    /**
     * 设置标尺颜色
     * @param QColor color 标尺颜色
     */
    void setTimelineRulerColor(const QColor& color);
    /**
     * 获取播放头颜色
     * @return QColor 播放头颜色
     */
    QColor timelinePlayheadColor() const { return m_timelinePlayheadColor; }
    /**
     * 设置播放头颜色
     * @param QColor color 播放头颜色
     */
    void setTimelinePlayheadColor(const QColor& color);
    /**
     * 获取片段填充颜色
     * @return QColor 片段填充颜色
     */
    QColor clipFillColor() const { return m_clipFillColor; }
    /**
     * 设置片段填充颜色
     * @param QColor color 片段填充颜色
     */
    void setClipFillColor(const QColor& color);
    /**
     * 获取片段边框颜色
     * @return QColor 片段边框颜色
     */
    QColor clipBorderColor() const { return m_clipBorderColor; }
    /**
     * 设置片段边框颜色
     * @param QColor color 片段边框颜色
     */
    void setClipBorderColor(const QColor& color);
    /**
     * 获取轨道选中颜色
     * @return QColor 轨道选中颜色
     */
    QColor trackSelectedColor() const { return m_trackSelectedColor; }
    /**
     * 设置轨道选中颜色
     * @param QColor color 轨道选中颜色
     */
    void setTrackSelectedColor(const QColor& color);
    /**
     * 绘制垂直时间线
     * @param QPainter* painter 绘图设备
     * @param QRect& rect 矩形
     */
    virtual void drawVerticalTimeLines(QPainter* painter, const QRect& rect);
    /**
     * 绘制背景
     * @param QPainter* painter 绘图设备
     * @param QRect& rect 矩形
     */
    virtual void drawBackground(QPainter* painter, const QRect& rect);
    /**
     * 绘制播放头
     * @param QPainter* painter 绘图设备
     */
    virtual void drawPlayhead(QPainter* painter);
    /**
     * 绘制时间刻度
     * @param QPainter* painter 绘图设备
     * @param QRect& rect 矩形
     */
    virtual void drawTimeRuler(QPainter* painter, const QRect& rect);
    /**
     * 绘制轨道
     * @param QPainter* painter 绘图设备
     */
    virtual void drawTracks(QPainter* painter);
    /**
     * 绘制时间标记
     * @param QPainter* painter 绘图设备
     * @param qint64 startUs 起始刻度时刻（μs，已规整到步长边界）
     * @param int startMarker 起始像素 x
     * @param int endMarker 结束像素 x
     * @param qint64 stepUs 刻度步长（μs）
     */
    virtual void drawTimeMarkers(QPainter* painter, qint64 startUs, int startMarker, int endMarker, qint64 stepUs);
    /**
     * 计算标尺刻度步长（μs；2026-11-10 微秒原生——按"漂亮时间档位"
     * 100ms/250ms/500ms/1s/2s/5s/10s/30s/60s/5min 选取首个不小于
     * 最小像素间距的档位，替代原帧数档位）
     * @return qint64 刻度步长（μs）
     */
    virtual qint64 calculateTimeStepUs() const;

    virtual BaseTimeLineModel* getModel() const;

    /**
     * 获取轨道宽度
     * @return int 轨道宽度
     */
    virtual int getTrackWdith() const;
    /**
     * 移动选定的剪辑
     * @param int dx 水平移动
     * @param int dy 垂直移动
     * @param bool isMouse 是否鼠标移动
     */
    virtual void moveSelectedClip(int dx, int dy,bool isMouse = true);
    /**
     * 移动播放头到指定时刻
     * @param qint64 us 微秒
     */
    virtual void movePlayheadToFrame(qint64 us);
    // 判断文件对应的track类型

    virtual QString isMimeAcceptable(const QMimeData *Mime) const;



    /**
     * 处理鼠标拖动操作
     * @param event 鼠标事件对象
     */
    virtual void handleMouseDrag(QMouseEvent *event);
    void beginClipDrag(AbstractClipModel* clip, const QModelIndex& index);
    void finishClipDrag();

    // ---- 磁性吸附（2026-11-09 用户需求：片段拖拽对齐）--------------------
    /// 收集吸附候选时刻（μs）：时间线原点 + 全部轨道所有片段的入/出点
    /// （排除拖拽片段自身）。跨轨道同权——邻轨片段边缘同样吸附（用户需求）。
    QList<qint64> collectSnapCandidates(const AbstractClipModel* exclude) const;
    /// 最近吸附：desired 落入任一候选的像素阈值内则返回该候选时刻并写
    /// *snapLine = 候选时刻；否则原样返回 desired、*snapLine = -1。
    qint64 snapFrame(qint64 desired, const QList<qint64>& candidates, qint64* snapLine) const;
    /// 绘制对齐参考线（paintEvent 直绘层；琥珀色竖线，与播放头区分）
    void drawSnapLine(QPainter* painter) const;

    // ---- 关键帧子元素拖拽（2026-11-05 Tween 集成；通用基础设施）------------
    // 视图不认识"关键帧"长什么样——命中与落点全部经
    // AbstractClipModel::hitKeyframe / moveKeyframe 询问模型（默认不命中，
    // 既有子类零影响）。视图只负责：press 时优先询问、拖拽中帧量化 +
    // 片段边界钳制、release/leave 清态、悬停光标反馈。
    /// 计算片段绘制矩形（与 drawTracks/paintEvent 直绘同一几何）并询问模型命中
    AbstractClipModel::KeyframeHit hitKeyframeAt(const QModelIndex& index,
                                                 const QPoint& pos) const;
    /// 结束关键帧拖拽（清状态；不触发模型侧收尾——写回在 moveKeyframe 内）
    void finishKeyframeDrag();

    /**
     * 更新鼠标悬停状态
     * @param event 鼠标事件对象
     */
    virtual void updateMouseHoverState(QMouseEvent *event);

    /**
     * 根据鼠标悬停状态更新鼠标指针形状
     */
    virtual void updateCursorShape();



protected slots:
    /**
     * 选择更改
     * @param QItemSelection selected 选择
     * @param QItemSelection deselected 取消选择
     */
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

    /**
     * 获取索引的代理
     * @param QModelIndex index 索引
     * @return QAbstractItemDelegate* 代理
     */
    // QAbstractItemDelegate* itemDelegateForIndex(const QModelIndex &index) const override ;

protected:
    // 当前缩放
    /**
     * 每个视图实例自己的时间刻度参数（避免跨窗口相互影响）
     * - m_timescale：当前像素/μs，仅用于本视图实例
     * - m_minTimescale：本视图允许的最小像素/μs，用于缩放下限
     * - m_baseTimeScale：本视图允许的最大像素/μs，用于缩放上限
     * 说明：这些成员在构造时从 TimeLineStyle.h 的默认值初始化，但后续不再写回全局变量
     */
    double m_timescale     = timescale;
    double m_minTimescale  = minTimescale;
    double m_baseTimeScale = baseTimeScale;

    /**
     * 当前缩放归一化比例（0~1）：与本实例的 m_timescale 保持一致
     * 公式：ratio = (m_timescale - m_minTimescale) / (m_baseTimeScale - m_minTimescale)
     * 这样首次滚轮缩放不会跳变
     */
    double currentScale = (m_timescale - m_minTimescale) / (m_baseTimeScale - m_minTimescale);
    // 时间线模型
    BaseTimeLineModel *Model;
    /**
     * 获取索引的矩形
     * @param QModelIndex index 索引
     * @return QRect 矩形
     */
    QRect itemRect(const QModelIndex &index) const;

    /**
     * @brief 内容区顶部偏移（统一几何口径）
     * @details
     * TrackListView 与 TimelineView 的坐标体系必须一致，否则会出现“同一行不对齐/滚动不同步”。
     * 这里将内容区顶部定义为：标尺高度 + 工具栏高度。
     */
    int contentTop() const { return rulerHeight + toolbarHeight; }

    // 当前视口里能看见的轨道行，闭区间。没有可见行时 lastRow < firstRow。
    void visibleTrackRange(int& firstRow, int& lastRow) const;

    /**
     * @brief 当前滚动偏移（与滚动条保持一致）
     * @details
     * 历史代码在多个位置直接读写 m_scrollOffset，并与 QAbstractItemView 内部滚动条机制并存，
     * 容易产生“滚动条值”和“m_scrollOffset 值”不一致，进而导致绘制/命中测试/拖拽计算错乱。
     * 重构策略：滚动条为唯一真值来源，m_scrollOffset 仅作为缓存，必须通过同步函数更新。
     */
    QPoint m_scrollOffset;

    /**
     * @brief 从滚动条同步 m_scrollOffset（滚动条为真值来源）
     * @details
     * - 当 scrollBar valueChanged 触发 QAbstractItemView::scrollContentsBy 后，应调用此函数更新缓存。
     * - 所有使用 m_scrollOffset 参与绘制/计算的代码，都依赖此同步保证正确性。
     */
    void syncScrollOffsetFromScrollBars();

    /**
     * @brief 设置滚动偏移并同步滚动条（自动夹紧到合法范围）
     * @param offset 目标偏移（x=水平，y=垂直）
     * @details
     * - 禁止直接写 m_scrollOffset（会导致滚动条不动、视图内部状态不一致）。
     * - 统一通过此函数写入：先夹紧，再写滚动条，再同步缓存。
     */
    void setScrollOffsetClamped(const QPoint& offset);

    /**
     * 点（像素 x）转换为时刻（μs）
     */
    qint64 pointToFrame(int point) const;
    /**
     * 时刻（μs）转换为点（像素 x）
     */
    int frameToPoint(qint64 frame) const;
    // 鼠标开始
    QPoint m_mouseStart;
    // 鼠标结束
    QPoint m_mouseEnd;
    // 鼠标偏移
    QPoint m_mouseOffset;
    // 鼠标是否按下
    bool mouseHeld = false;
    bool m_interactiveDrag = false;
    AbstractClipModel* m_dragClip = nullptr;
    QPersistentModelIndex m_dragIndex;
    // ---- 磁性吸附状态（2026-11-09）----
    bool m_snapEnabled     = true;                  // 吸附开关（默认开）
    qint64 m_snapLineFrame = -1;                    // 对齐参考线时刻（μs；-1 = 不画）
    int  m_snapThresholdPx = 8;                     // 吸附半径（像素；帧阈值 = px/m_timescale）
    QColor m_snapLineColor = QColor(255, 200, 64);  // 对齐参考线颜色（琥珀黄，QSS 可覆盖）
    // ---- 关键帧拖拽状态（2026-11-05；与片段拖拽互斥，press 时二选一）----
    AbstractClipModel* m_keyframeDragClip = nullptr; // 拖拽中的片段（弱引用，模型生命周期归适配器）
    QPersistentModelIndex m_keyframeDragIndex;       // 拖拽中的片段索引（脏刷新定位用）
    int m_keyframeDragChannel = -1;                  // 命中的通道下标（模型语义）
    int m_keyframeDragKey     = -1;                  // 命中的关键帧下标（模型语义）
    // press 时的片段绘制矩形快照（2026-11-05 曲线编辑带）：拖拽中片段自身
    // 不移动（关键帧水平拖 + 垂直值拖均不改变片段几何），矩形稳定——拖拽
    // 垂直分量按此矩形换算为片段内 local y 传给模型（值域映射是模型知识）
    QRect m_keyframeDragRect;
    // 【2026-11-09 合并关键帧模式】片段带重新成为主编辑界面（拖拽合并菱形
    // + 右键删除），原"只读预览"开关 m_tweenBandInteractive 退役——可交互
    // 与否由模型自决（SceneClipModel 按 hasTweenKeys 命中，无键片段恒不命中）。
    // 播放头是否选中
    bool m_playheadSelected = false;
    // 是否正在拖放媒体
    bool m_isSupportMedia = false;
    // 鼠标最后拖动位置
    QPoint m_lastDragPos;
    // 鼠标悬停状态
    hoverState m_mouseUnderClipEdge = NONE;
    // 鼠标悬停索引
    QModelIndex m_hoverIndex = QModelIndex();

    // ========== 新增：重绘合并机制 ==========
    /**
     * 合并重绘请求（去抖）
     * - 将多次重绘请求在 delayMs 毫秒内合并为一次，默认 16ms（约一帧）
     * - 降低 updateEditorGeometries/updateScrollBars/viewport()->update 的调用频率
     */
    void scheduleRedraw(int delayMs = 16);

    /**
     * 统一执行一次重绘（实际操作）
     * - 防重入：使用 m_isRedrawing，避免重入引发递归或重复开销
     * - 内容变化时更新滚动条并整页刷新。播放头移动不走这里。
     */
    void performRedraw();
    void ensureStaticCache();
    bool staticCacheReady() const;
    // 滚动时平移已有缓存，只重画新露出来的条带。平移失败时返回 false，调用方整页重建。
    bool shiftStaticCache(int dx, int dy);
    void repaintStaticCache(const QRegion& region);
    /// 静态层四连绘（背景/竖网格/轨道/时间标尺）——ensureStaticCache 全量
    /// 重建与 repaintStaticCache 脏区补绘共用同一顺序（2026-11-10 整理：
    /// 两处手写四连调用收敛为一个出口，防后续增层只改一处导致两路不一致）。
    void drawStaticLayers(QPainter* painter, const QRect& rect);
    QRect clipContentBounds(AbstractClipModel* clip, int trackRow) const;
    QRect playheadDirtyRect(qint64 us) const;

    // 合并重绘用的定时器（单次触发）
    QTimer* m_redrawCoalesceTimer {nullptr};

    // 拖拽边缘自动横滚（2026-11-10 滚动体感修复）：定时器驱动——真实拖拽
    // 50ms 步进一次，体感流畅且与事件频率解耦；合成事件（sendEvent 不进
    // 事件循环）不触发定时器 → 探针/吸附测试零干扰。
    QTimer* m_edgeScrollTimer {nullptr};
    int m_edgeScrollDir {0}; // U5：带符号步长 px/tick（±12 外带 / ±24 中带 / ±36 压边），0 停

    // ---- P5（timeline-draw-zoom-review）：连续滚轮缩放合并 ----
    // 60ms 窗口内的后续滚轮只累计 ratio，窗口到期一次 setScaleAtPx——
    // 大工程全页重建从每事件一次降为每 60ms 一次。首事件立即应用开窗
    //（合成事件不进事件循环 → 探针/测试零干扰）。
    QTimer* m_zoomCoalesceTimer {nullptr};
    double m_pendingZoomRatio {1.0};  // 窗口内累计的缩放倍率（连乘）
    int m_pendingZoomAnchorPx {0};    // 窗口内最后一次事件的锚点视口像素

    // 防重入标志：防止 performRedraw 期间再次触发 performRedraw
    bool m_isRedrawing {false};

    // 不含播放头的视口缓存。播放头移动时贴上这张图，再画播放头。
    QPixmap m_staticCache;
    bool m_staticCacheDirty = true;
    qint64 m_paintedPlayheadUs = -1; // 上次绘制的播放头位置（μs）
    // 片段在内容坐标中的上次绘制范围，供局部刷新擦掉旧位置。
    std::unordered_map<AbstractClipModel*, QRect> m_clipContentRects;

    /**
     * P3（timeline-draw-zoom-review）：标尺刻度文本缓存。
     * 每个 tick 的 timecode 换算 + fontMetrics 文本测量是标尺绘制的
     * 最大热点；同一 (tickUs, 制式) 的文本恒定，缓存后滚动帧零换算。
     * 制式变化（m_tickLabelTcType）或超容量时整体失效。
     */
    struct TickLabel { QString text; int width; };
    std::unordered_map<qint64, TickLabel> m_tickLabelCache;
    int m_tickLabelTcType = -1; // TimeCodeType 的 int 投影（-1 = 未缓存）

    /**
     * 画布平移状态
     * - m_isPanning: 是否处于拖动画布模式
     * - m_panStart: 鼠标按下的起始点（viewport 坐标）
     * - m_panInitialOffset: 拖拽开始时的滚动偏移
     * - m_panMoved: 是否在拖拽中产生了足够位移（用于区分点击与拖拽）
     */
    bool m_isPanning = false;
    QPoint m_panStart;
    QPoint m_panInitialOffset;
    /**
     * 处理样式变化事件（QSS/调色板变更）
     * @param QEvent* e 事件
     */
    virtual void changeEvent(QEvent* e) override;
    /**
     * 从属性应用颜色到全局样式（兼容旧绘制逻辑）
     */
    void applyStyleFromProperties();
    // 基于 TimeLineStyle.h 的默认值初始化，可被 QSS 属性覆盖
    QColor m_timelineBgColor       = bgColour;
    QColor m_timelineSeparatorColor= seperatorColour;
    QColor m_timelineRulerColor    = rulerColour;
    QColor m_timelinePlayheadColor = playheadColour;
    QColor m_clipFillColor         = ClipColor;
    QColor m_clipBorderColor       = ClipBorderColour;
    QColor m_trackSelectedColor    = trackSelectedColour;
};






