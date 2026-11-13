#include "BaseTimeLineView.h"
#include <QMenu>
#include <QAction>
#include <QtMath>
#include <QTimer>
#include <algorithm>
#include "DefaultTimeLineToolBar.h"

namespace {
// 2026-11-11 极限缩放加固：frameLo/Hi 是 μs 域时间量（pointToFrame 返回
// qint64）——缩到最小时视口右缘反算可达 1.6e10 μs（4.4h），超出 int 域
// （μs 进 int 即雷），截断后命中/绘制过滤窗口错乱。参数与内部值全 qint64。
void clipRowsInFrameRange(const TrackData* track, qint64 frameLo, qint64 frameHi, QList<int>& rows)
{
    rows.clear();
    if (!track)
        return;

    const int clipCount = track->clips.size();
    if (clipCount <= 0)
        return;

    auto overlaps = [&](int row) {
        AbstractClipModel* clip = track->clips[row];
        return clip && clip->end() >= frameLo && clip->start() <= frameHi;
    };

    if (track->clipsByStart.size() != clipCount) {
        for (int row = 0; row < clipCount; ++row) {
            if (overlaps(row))
                rows.push_back(row);
        }
        return;
    }

    const qint64 minStart = frameLo - qMax(0, track->maxClipSpan);
    const QList<int>& order = track->clipsByStart;
    const auto first = std::lower_bound(order.begin(), order.end(), minStart, [track](int row, qint64 value) {
        const AbstractClipModel* clip = track->clips[row];
        return (clip ? clip->start() : 0) < value;
    });
    const auto last = std::upper_bound(order.begin(), order.end(), frameHi, [track](qint64 value, int row) {
        const AbstractClipModel* clip = track->clips[row];
        return value < (clip ? clip->start() : 0);
    });
    for (auto it = first; it != last; ++it) {
        if (overlaps(*it))
            rows.push_back(*it);
    }
    std::sort(rows.begin(), rows.end());
}
}
BaseTimelineView::BaseTimelineView(BaseTimeLineModel *viewModel, QWidget *parent)
        : Model(viewModel), QAbstractItemView{parent}
    {
        setObjectName("TimelineView");
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt:: ScrollBarAlwaysOff);
        setModel(Model);
        horizontalScrollBar()->setSingleStep(10);
        horizontalScrollBar()->setPageStep(100);
        verticalScrollBar()->setSingleStep(trackHeight);
        verticalScrollBar()->setPageStep(trackHeight * 5);
        viewport()->setMinimumHeight(trackHeight + rulerHeight + toolbarHeight);
        // setMinimumHeight(trackHeight + rulerHeight + toolbarHeight);

        /**
         * @brief 选择模型说明
         * @details
         * 本视图不再在构造函数里创建 selectionModel。
         * - 由 BaseTimelineWidget 统一创建共享 QItemSelectionModel，并同时注入到 TrackListView 与 TimelineView。
         * - 这样可以保证“选择状态”只有一份，避免绑定链路复杂、状态不同步。
         */
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectItems);
        setMouseTracking(true);
        // 确保有选择模型
        setEditTriggers(QAbstractItemView::AllEditTriggers);
        setAutoScroll(true);
        setAutoScrollMargin(5);
        setAcceptDrops(true);
        // setDragEnabled(true);

    // 连接循环播放信号

        installEventFilter(this);

        // ========== 新增：初始化重绘合并定时器 ==========
        m_redrawCoalesceTimer = new QTimer(this);
        m_redrawCoalesceTimer->setSingleShot(true);
        connect(m_redrawCoalesceTimer, &QTimer::timeout, this, [this]() {
            performRedraw();
        });
        // ========== 新增结束 ==========

        // ========== 边缘自动横滚定时器（2026-11-10 滚动体感修复）==========
        // 定时器驱动而非逐事件立即滚：真实拖拽 50ms 步进一次、体感流畅，
        // 且合成事件（sendEvent 不进事件循环）不会触发定时器 → 单测零干扰。
        m_edgeScrollTimer = new QTimer(this);
        m_edgeScrollTimer->setInterval(50);
        connect(m_edgeScrollTimer, &QTimer::timeout, this, [this]() {
            // 非拖拽中或已离缘 → 停止。离缘时 handleMouseDrag 已置 0，这里兜底。
            // U4：播放头拖拽（m_playheadSelected，不设 m_interactiveDrag）
            // 也复用本定时器，停止条件相应放宽。
            if ((!m_interactiveDrag && !m_playheadSelected) || m_edgeScrollDir == 0) {
                m_edgeScrollTimer->stop();
                return;
            }
            QScrollBar* hb = horizontalScrollBar();
            // 拖拽中模型 defer 通知 → range 落后于期望落点，需要时实时扩展；
            // 松手 performRedraw 会按模型真值重算 range 覆盖。
            const int vw = viewport()->width();
            const int desiredMax = hb->value() + vw;
            if (hb->maximum() < desiredMax)
                hb->setRange(0, desiredMax);
            // U5：步长已并入 m_edgeScrollDir（±12/±24/±36 深度分档）
            hb->setValue(qBound(hb->minimum(), hb->value() + m_edgeScrollDir,
                                hb->maximum()));
            // U4：播放头拖拽复用时，滚动后播放头保持压在鼠标下
            //（时刻 = 鼠标下的新内容时刻，与 handleMouseDrag 同口径）
            if (m_playheadSelected)
                movePlayheadToFrame(pointToFrame(std::max(0, m_mouseEnd.x() + scrollX())));
        });
        // ========== 新增结束 ==========

        // ========== P5：连续滚轮缩放合并窗口（timeline-draw-zoom-review）==========
        m_zoomCoalesceTimer = new QTimer(this);
        m_zoomCoalesceTimer->setSingleShot(true);
        m_zoomCoalesceTimer->setInterval(60);
        connect(m_zoomCoalesceTimer, &QTimer::timeout, this, [this]() {
            if (m_pendingZoomRatio == 1.0)
                return;
            const double merged = qBound(0.0, currentScale * m_pendingZoomRatio, 1.0);
            setScaleAtPx(merged, m_pendingZoomAnchorPx);
            m_pendingZoomRatio = 1.0;
        });
        // ========== 新增结束 ==========

        // 从当前属性应用颜色，支持 QSS 的 qproperty- 语法
        applyStyleFromProperties();
    }

BaseTimelineView::~BaseTimelineView()
{
    finishClipDrag();
    delete m_toolbar;
    // delete timer;

}

void BaseTimelineView::initToolBar(DefaultTimeLineToolBar* new_toolbar)
{
    m_toolbar=new_toolbar;
    m_toolbar->setFixedHeight(toolbarHeight-6);
    // 设置工具栏位置
    m_toolbar->move(4, 4);

}

QRect BaseTimelineView::visualRect(const QModelIndex &index) const
{
    /**
     * @brief 获取索引在 viewport 内的可视矩形
     * @details
     * 与 TrackListView 保持一致：以滚动条 value 为唯一真值来源计算偏移，
     * 避免 m_scrollOffset 缓存状态与滚动条不同步导致命中测试/绘制错位。
     */
    const QPoint offset(horizontalScrollBar()->value(), verticalScrollBar()->value());
    return itemRect(index).translated(-offset);
}

 QModelIndex BaseTimelineView::indexAt(const QPoint &point) const
    {
        /**
         * 函数说明：根据鼠标点返回对应的模型索引
         * - 原逻辑：优先命中轨道，再命中该轨道下的剪辑矩形（visualRect）
         * - 修改点：为避免在极小缩放下片段过窄导致无法选中，
         *   在剪辑的命中测试时应用“最小命中宽度”保护：
         *   如果 clipRect.width() < minHitWidth，则围绕其中心点临时扩展命中矩形到 minHitWidth，
         *   仅用于contains判断，不影响绘制或布局的真实宽度。
         */
        if (point.y() < contentTop() || !model())
            return {};

        const int contentY = point.y() + verticalScrollBar()->value() - contentTop();
        if (contentY < 0)
            return {};
        // per-row 行高反查行号（2026-11-06 轨道展开：展开行更高，禁均分）
        const int row = static_cast<BaseTimeLineModel*>(model())->trackRowAt(contentY);
        if (row < 0 || row >= model()->rowCount())
            return {};

        const QModelIndex trackIndex = model()->index(row, 0);
        const QRect trackRect = visualRect(trackIndex);
        if (!trackRect.contains(point))
            return {};

        auto* track = static_cast<TrackData*>(trackIndex.internalPointer());
        const int minHitWidth = 15;
        const int contentX = point.x() + horizontalScrollBar()->value();
        // 2026-11-11 极限缩放加固：μs 域反算值必须 qint64 承载（缩到最小时
        // pointToFrame 可达 1.6e10 μs，int 截断 = 命中过滤窗口错乱）
        const qint64 frameLo = pointToFrame(qMax(0, contentX - minHitWidth)) - 1;
        const qint64 frameHi = pointToFrame(contentX) + 1;
        QList<int> clipRows;
        clipRowsInFrameRange(track, frameLo, frameHi, clipRows);

        for (int clipRow : clipRows) {
            const QModelIndex clipIndex = model()->index(clipRow, 0, trackIndex);
            QRect clipRect = visualRect(clipIndex);
            if (clipRect.width() < minHitWidth) {
                const int pad = minHitWidth - clipRect.width();
                clipRect.adjust(0, 0, pad, 0);
                clipRect = clipRect.intersected(trackRect);
            }
            if (clipRect.contains(point))
                return clipIndex;
        }
        return trackIndex;
    }

QRect BaseTimelineView::itemRect(const QModelIndex &index) const
{
    if (!index.isValid() || !model()) {
        return QRect();
    }
    auto* tlModel = static_cast<BaseTimeLineModel*>(model());
    // 获取轨道宽度
    int trackwidth = getTrackWdith()+ viewport()->width();
    if(index.parent()==QModelIndex())
        // 如果轨道索引
    {
        // 返回轨道矩形（2026-11-06 轨道展开：行高 per-row，展开行更高）
        return QRect(0, tlModel->trackRowY(index.row()) + contentTop(), trackwidth, tlModel->trackRowHeight(index.row()));
    }
     // 如果剪辑索引
    else{
        // 获取剪辑
        AbstractClipModel* clip = static_cast<AbstractClipModel*>(index.internalPointer());
        if (clip) {
            // 获取剪辑开始帧（2026-11-11 极限缩放加固：start/end 是 μs 域
            // qint64，int 承接截断——中转变量同域）
            const qint64 startFrame = clip->start();
            // 获取剪辑结束帧
            const qint64 endFrame = clip->end()+1;
            // 获取剪辑所在轨道行
            int trackRow = index.parent().row();
            // 获取剪辑开始点x
            int clipStartX = frameToPoint(startFrame);
            // 获取剪辑开始点y（trackRowY：展开行下方的行顶正确下移）
            int clipStartY = tlModel->trackRowY(trackRow) + contentTop();
            // 获取剪辑宽度
            int clipWidth = frameToPoint(endFrame - startFrame);
            // 获取剪辑所在轨道行开始点
            QPoint topLeft(clipStartX,clipStartY);
            // 返回剪辑矩形（clip 本体高度恒 trackHeight——展开附加区归 clip panel）
            return QRect(topLeft,QSize(clipWidth,trackHeight));
        }

    }
    return QRect();
}

int BaseTimelineView::getTrackWdith() const
{
    // 时间轴总时长是 μs（qint64）——旧 .toInt() 在 >35.8 分钟工程上溢出，
    // 滚动条 range 直接错乱（缩放焦点漂移的加重因素）
    const qint64 timelineLength =
        model()->data(QModelIndex(), TimelineRoles::TimelineLengthRole).toLongLong();
    if (timelineLength <= 0) {
        return viewport()->width();  // 如果没有片段，返回视口宽度
    }
    return frameToPoint(timelineLength + 1);  // 否则返回基于时间轴长度的宽度（+1μs 防零宽）
}

// 移动选定的剪辑
void BaseTimelineView::moveSelectedClip(int dx, int dy, bool isMouse)
{
    QModelIndexList list = selectionModel()->selectedIndexes();
    if(list.isEmpty())
        return;
    int newPos;
    if(isMouse){
        newPos = dx;
    }else{
        newPos = model()->data(list[0],TimelineRoles::ClipInRole).toInt() + dx;
    }
    // 确保不会移动到负值位置
    newPos = std::max(0, newPos);
    // 更新模型数据
    // 非拖拽时 dataChanged 会局部刷新。拖拽期间模型先不发通知，由鼠标移动自己脏矩形。
    getModel()->setData(list.first(), newPos, TimelineRoles::ClipPosRole);
}

void BaseTimelineView::movePlayheadToFrame(qint64 us)
{
    // 直接设置时间码生成器的时刻位置（μs）。S_playheadMoved 会走局部刷新。
    getModel()->onSetPlayheadPos(us);
}

void BaseTimelineView::onUpdateViewport(){
    scheduleRedraw();
}

// 滚动视图
void BaseTimelineView::scheduleRedraw(int delayMs) {
    m_staticCacheDirty = true;
    if (!m_redrawCoalesceTimer) return;
    // 如果定时器已在等待中，无需重复启动；让多次请求自然合并
    if (!m_redrawCoalesceTimer->isActive()) {
        m_redrawCoalesceTimer->start(delayMs);
    }
}

void BaseTimelineView::performRedraw() {
    if (m_isRedrawing) return;
    m_isRedrawing = true;

    // P6（timeline-draw-zoom-review）：全量重绘前清空片段内容矩形缓存。
    // 该 map 只插不删曾是泄漏面：clip 销毁后键悬垂，指针地址复用时读到
    // 过期 QRect → updateClipGeometry 的脏区错算。全量重绘会经 drawTracks
    // 重建整个 map，此处清空零风险。
    m_clipContentRects.clear();

    // 内容变化才更新滚动条范围。时间轴没有持久编辑器。
    updateScrollBars();
    viewport()->update();

    m_isRedrawing = false;
}

void BaseTimelineView::onPlayheadMoved(qint64 us)
{
    const qint64 previous = m_paintedPlayheadUs;
    m_paintedPlayheadUs = us;
    if (!staticCacheReady()) {
        viewport()->update();
        return;
    }

    /**
     * U1（timeline-draw-zoom-review）：播放头跟随页翻。
     * 回放时播放头越出视口右缘 → 水平滚动一整页把它带回视口（播放头
     * 贴左缘，主流 NLE 语义）。仅在"旁观"状态生效：用户正在拖播放头
     * （m_playheadSelected）或拖片段（m_interactiveDrag）时不抢滚动，
     * 避免与手部操作打架；向左回退不跟随（手动 seek 定位不被干扰）。
     */
    if (!m_playheadSelected && !m_interactiveDrag && viewport() && us >= 0) {
        const int x = frameToPoint(us) - scrollX();
        if (x >= viewport()->width()) {
            QScrollBar* hb = horizontalScrollBar();
            // 播放头贴视口左缘；range 落后时（拖拽 defer）由 clamp 兜住
            hb->setValue(qBound(hb->minimum(), frameToPoint(us), hb->maximum()));
        }
    }

    if (previous >= 0 && previous != us)
        viewport()->update(playheadDirtyRect(previous));
    viewport()->update(playheadDirtyRect(us));
}

bool BaseTimelineView::staticCacheReady() const
{
    if (m_staticCacheDirty || m_staticCache.isNull() || !viewport())
        return false;
    const qreal dpr = viewport()->devicePixelRatio();
    const QSize px(qRound(viewport()->width() * dpr), qRound(viewport()->height() * dpr));
    return m_staticCache.size() == px;
}

QRect BaseTimelineView::playheadDirtyRect(qint64 us) const
{
    if (us < 0 || !viewport())
        return {};
    const int x = frameToPoint(us) - horizontalScrollBar()->value();
    const int pad = playheadwidth + 2;
    const int top = qMax(0, contentTop() - playheadheight);
    return QRect(x - pad, top, pad * 2 + 1, viewport()->height() - top).intersected(viewport()->rect());
}

void BaseTimelineView::ensureStaticCache()
{
    QWidget* vp = viewport();
    if (!vp || vp->width() <= 0 || vp->height() <= 0)
        return;
    if (staticCacheReady())
        return;
    if (m_interactiveDrag && Model)
        Model->syncInteractiveClipOrder();

    const qreal dpr = vp->devicePixelRatio();
    m_staticCache = QPixmap(qRound(vp->width() * dpr), qRound(vp->height() * dpr));
    m_staticCache.setDevicePixelRatio(dpr);
    m_staticCache.fill(m_timelineBgColor);

    QPainter cachePainter(&m_staticCache);
    const QRect rect(0, 0, vp->width(), vp->height());
    drawStaticLayers(&cachePainter, rect);
    m_staticCacheDirty = false;
}

QRect BaseTimelineView::clipContentBounds(AbstractClipModel* clip, int trackRow) const
{
    if (!clip || !model())
        return {};
    const int x = frameToPoint(clip->start());
    const int width = qMax(1, frameToPoint(clip->end() - clip->start() + 1));
    // 2026-11-06 轨道展开：行顶 y 走模型前缀和（展开行下移）；clip 本体高度仍 trackHeight
    const int y = static_cast<const BaseTimeLineModel*>(model())->trackRowY(trackRow) + contentTop();
    // 右侧多留一截，盖住画在片段矩形外面的标题。
    return QRect(x, y, width, trackHeight).adjusted(-4, -4, 44, 4);
}

void BaseTimelineView::updateClipGeometry(const QModelIndex& index)
{
    if (!index.isValid() || !index.parent().isValid() || !viewport())
        return;
    auto* clip = static_cast<AbstractClipModel*>(index.internalPointer());
    if (!clip)
        return;
    if (!staticCacheReady()) {
        scheduleRedraw();
        return;
    }

    const QRect now = clipContentBounds(clip, index.parent().row());
    const auto found = m_clipContentRects.find(clip);
    const QRect old = found == m_clipContentRects.end() ? now : found->second;
    m_clipContentRects[clip] = now;

    // P7：走 scrollOffset() 统一读点（滚动条真值缓存）
    const QPoint scroll = scrollOffset();
    QRegion dirty(old.translated(-scroll));
    dirty += now.translated(-scroll);
    dirty &= viewport()->rect();
    if (dirty.isEmpty())
        return;
    repaintStaticCache(dirty);
    viewport()->update(dirty.boundingRect());
}

void BaseTimelineView::syncScrollOffsetFromScrollBars()
{
    /**
     * @brief 从滚动条同步 m_scrollOffset
     * @details
     * 滚动条（horizontalScrollBar/verticalScrollBar）是 QAbstractItemView 的内建滚动状态。
     * 当前项目历史代码同时维护了 m_scrollOffset，因此必须在滚动发生后同步缓存，保证绘制/命中测试一致。
     */
    m_scrollOffset = QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());
}

void BaseTimelineView::setScrollOffsetClamped(const QPoint& offset)
{
    /**
     * @brief 设置滚动偏移并同步滚动条（自动夹紧）
     * @details
     * 禁止直接修改 m_scrollOffset；正确做法是设置滚动条 value，让 QAbstractItemView 走标准滚动路径。
     * 这里做夹紧，避免出现负值或超过 maximum 的情况。
     */
    const int x = qBound(horizontalScrollBar()->minimum(), offset.x(), horizontalScrollBar()->maximum());
    const int y = qBound(verticalScrollBar()->minimum(),   offset.y(), verticalScrollBar()->maximum());

    horizontalScrollBar()->setValue(x);
    verticalScrollBar()->setValue(y);

    syncScrollOffsetFromScrollBars();
}

// 更新滚动条
void BaseTimelineView::updateScrollBars()
{
    /**
     * @brief 更新滚动条范围（与 TrackListView 使用相同几何口径）
     * @details
     * 以滚动条为唯一真值来源：
     * - 垂直内容总高度 = contentTop(标尺+工具栏) + Σ 各行高
     *   （2026-11-06 轨道展开：展开行 = trackHeight + 附加高度）
     * - 两个视图必须使用同一公式（模型 trackRowHeight 前缀和），否则会出现滚动不同步、行对不齐
     */
    if (!model())
        return;

    const int maxX = qMax(0, getTrackWdith() - viewport()->width());
    horizontalScrollBar()->setRange(0, maxX);

    const int contentTop = rulerHeight + toolbarHeight;
    // 2026-11-06 轨道展开：总高 = Σ 各行高（展开行 = trackHeight + 附加高度）
    const int totalHeight = static_cast<BaseTimeLineModel*>(model())->totalTracksHeight() + contentTop;
    const int maxY = qMax(0, totalHeight - viewport()->height());
    verticalScrollBar()->setRange(0, maxY);

    // range 变化可能导致 value 被 Qt 自动夹紧，这里同步一次缓存
    syncScrollOffsetFromScrollBars();
}

bool BaseTimelineView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event)
{
    // 轨道名在左侧列表里双击编辑。时间轴上选中轨道时不要弹出输入框。
    if (!index.parent().isValid())
        return false;
    return QAbstractItemView::edit(index, trigger, event);
}

void BaseTimelineView::scrollContentsBy(int dx, int dy)
{
    /**
     * @brief 滚动条变化后的视口滚动
     * @details
     * dx/dy 是内容相对视口的位移：滚动条增大时为负，已有像素向左/上移动。
     * 缓存能平移时只重画新露出的条带；缩放、改内容或跨度超过视口时整页重建。
     */
    QAbstractItemView::scrollContentsBy(dx, dy);
    syncScrollOffsetFromScrollBars();
    if (dx == 0 && dy == 0)
        return;
    if (!shiftStaticCache(dx, dy))
        m_staticCacheDirty = true;
    if (viewport())
        viewport()->update();
}

bool BaseTimelineView::shiftStaticCache(int dx, int dy)
{
    QWidget* vp = viewport();
    if (!vp || !staticCacheReady())
        return false;

    const int w = vp->width();
    const int h = vp->height();
    if (w <= 0 || h <= 0)
        return false;
    // 一次跨过整个视口时，平移省不下绘制。
    if (qAbs(dx) >= w || qAbs(dy) >= h)
        return false;

    const int header = qBound(0, contentTop(), h);
    // 刻度横线画在 contentTop() 这一像素上。裁切高度若正好是 contentTop()，
    // 这一行会落进轨道区域，上下滚动时被轨道背景盖住。
    const int rulerBottom = qMin(h, header + 1);

    /**
     * P1（timeline-draw-zoom-review）：QPixmap::scroll 原地平移替代
     * "新 QPixmap 分配 + fill + 整图 blit"。原实现每次滚动都整图拷贝两遍
     * （一次分配 + 两次全尺寸 drawPixmap），滚动是最高频路径，这里是
     * 最大单点开销。scroll(dx,dy,rect) 只搬运裁剪矩形内的像素，标尺区
     * 只跟横向滚动、轨道区横竖都跟，语义与原 drawPixmap(dx[,dy]) 完全
     * 一致；平移后暴露出的像素未定义，由下方 exposed 补绘兜住。
     */
    if (rulerBottom > 0)
        m_staticCache.scroll(dx, 0, QRect(0, 0, w, rulerBottom));
    if (rulerBottom < h)
        m_staticCache.scroll(dx, dy, QRect(0, rulerBottom, w, h - rulerBottom));

    // 多画 1 像素，盖住条带接缝上的刻度线。
    QRegion exposed;
    if (dx > 0)
        exposed += QRect(0, 0, qMin(w, dx + 1), h);
    else if (dx < 0)
        exposed += QRect(qMax(0, w + dx - 1), 0, qMin(w, -dx + 1), h);
    if (dy != 0 && rulerBottom < h) {
        const int contentH = h - rulerBottom;
        const int band = qMin(contentH, qAbs(dy) + 1);
        if (dy > 0)
            exposed += QRect(0, rulerBottom, w, band);
        else
            exposed += QRect(0, h - band, w, band);
        // 轨道条带的重绘会铺背景。标尺整段再画一次，底边横线才不会被盖住。
        exposed += QRect(0, 0, w, rulerBottom);
    }
    repaintStaticCache(exposed);
    return true;
}

void BaseTimelineView::repaintStaticCache(const QRegion& region)
{
    if (region.isEmpty() || m_staticCache.isNull() || !viewport())
        return;
    QPainter painter(&m_staticCache);
    painter.setClipRegion(region);
    const QRect rect(0, 0, viewport()->width(), viewport()->height());
    drawStaticLayers(&painter, rect);
}

void BaseTimelineView::drawStaticLayers(QPainter* painter, const QRect& rect)
{
    drawBackground(painter, rect);
    drawVerticalTimeLines(painter, rect);
    drawTracks(painter);
    drawTimeRuler(painter, rect);
}

qint64 BaseTimelineView::pointToFrame(int point) const
{
    /**
     * 将像素位置转换为时刻（μs；使用实例级 m_timescale，避免跨窗口干扰）
     * - 2026-11-10 微秒原生：m_timescale 语义 = 像素/μs
     * - 当 m_timescale 越小，表示更"缩小"，则每个像素代表更长时间
     * - 2026-11-11 极限缩放加固：timescale ≤ 0（异常态）除零=整数转换 UB
     *   ——正常流程 minTimescale(1e-5) > 0 不会触发，防御性兜底返回 0
     */
    if (m_timescale <= 0.0)
        return 0;
    return qRound64(static_cast<double>(point) / m_timescale);
}

int BaseTimelineView::frameToPoint(qint64 frame) const
{
    /**
     * 将时刻（μs）转换为像素坐标（使用实例级 m_timescale，避免跨窗口干扰）
     * - 当 m_timescale 越小，表示更"缩小"，则单位时间占据更少像素
     * - 2026-11-11 极限缩放加固：同 pointToFrame，防御异常态除零
     */
    if (m_timescale <= 0.0)
        return 0;
    return qRound(static_cast<double>(frame) * m_timescale);
}

void BaseTimelineView::contextMenuEvent(QContextMenuEvent* event) {
    // 构建并弹出右键菜单（统一一个 QMenu 实例）
    QModelIndex index = indexAt(event->pos());
    QMenu contextMenu(this);

    // Focus cursor：滚动视图到播放头位置（尽量居中）
    QAction* focusCursorAction = new QAction("Focus cursor", this);
    focusCursorAction->setIcon(QIcon(":/icons/icons/location.png"));
    connect(focusCursorAction, &QAction::triggered, [this]() {
        const int playheadX = frameToPoint(getModel()->getPlayheadPos());
        const int viewWidth = viewport()->width();
        const int maxScroll = horizontalScrollBar()->maximum();
        int targetScroll = playheadX - viewWidth / 2;
        targetScroll = qBound(0, targetScroll, maxScroll);
        // 滚动条变化会进入 scrollContentsBy，在那里平移缓存。
        horizontalScrollBar()->setValue(targetScroll);
    });
    contextMenu.addAction(focusCursorAction);

    // 如果点击在轨道（无父）上，添加“Add Item”
    if (index.isValid() && !index.parent().isValid()) {
        QAction* addClipAction = new QAction("Add Item", this);
        addClipAction->setIcon(QIcon(":/icons/icons/add.png"));
        connect(addClipAction, &QAction::triggered, [this, index, event]() {
            addClipAtPosition(index, event->pos());
        });
        contextMenu.addAction(addClipAction);
    }

    // 如果点击在已选中的剪辑上，添加“Delete Clip”
    if (index.isValid() && selectionModel()->isSelected(index)) {
        // ---- 关键帧右键菜单（2026-11-05 曲线编辑带；2026-11-09 合并关键帧
        // 模式重开交互）——右键点中关键帧时优先询问模型自定义菜单（合并
        // 模式的"删除该帧全部/按参数删除"两级语义），模型未提供时回退
        // 默认 Delete Keyframe 单项（通用兜底）。片段级删除仍可通过先点
        // 选片段标题区再右键到达。
        if (index.parent().isValid()) {
            auto* kfClip = static_cast<AbstractClipModel*>(index.internalPointer());
            const QRect kfClipRect = visualRect(index).adjusted(0, clipoffset, 0, -clipoffset);
            const AbstractClipModel::KeyframeHit kf =
                kfClip ? kfClip->hitKeyframe(event->pos() - kfClipRect.topLeft(), kfClipRect)
                       : AbstractClipModel::KeyframeHit{};
            if (kf.hit && kfClip) {
                const bool custom =
                    kfClip->appendKeyframeContextMenu(contextMenu, kf);
                if (!custom) {
                    QAction* deleteKeyAction = new QAction("Delete Keyframe", this);
                    deleteKeyAction->setIcon(QIcon(":/icons/icons/trash.png"));
                    connect(deleteKeyAction, &QAction::triggered, [this, index, kfClip, kf]() {
                        if (kfClip->removeKeyframe(kf.channel, kf.key)) {
                            updateClipGeometry(index); // 局部重绘（键/曲线消失）
                        }
                    });
                    contextMenu.addAction(deleteKeyAction);
                }
            }
        }
        QAction* deleteClipAction = new QAction("Delete Clip", this);
        deleteClipAction->setIcon(QIcon(":/icons/icons/trash.png"));
        connect(deleteClipAction, &QAction::triggered, [this, index]() {
            getModel()->onDeleteClip(index);
            selectionModel()->clearSelection();
            viewport()->update();
        });
        contextMenu.addAction(deleteClipAction);
    }

    // 统一显示菜单
    contextMenu.exec(event->globalPos());

    // 调用基类处理（保持与原逻辑一致）
    QAbstractItemView::contextMenuEvent(event);
}

void BaseTimelineView::addClipAtPosition(const QModelIndex& index, const QPoint& pos) {
    if (!index.isValid() || index.parent().isValid()) {
        qDebug() << "Invalid index: Click was not on a valid track";
        return;
    }

    int trackIndex = index.row();
    if (trackIndex < 0 || trackIndex >= getModel()->getTrackCount()) {
        qDebug() << "Invalid track index: " << trackIndex;
        return;
    }
    // Calculate the start frame based on the mouse position
    int startFrame = pointToFrame(pos.x() + m_scrollOffset.x());

    getModel()->onAddClip(trackIndex,startFrame);

    // 更新视图（优化为合并重绘）
    scheduleRedraw();
}

void BaseTimelineView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        // 处理左键按下的情况
        QAbstractItemView::mouseReleaseEvent(event);
        return;
    }

    finishClipDrag();
    finishKeyframeDrag(); // 关键帧拖拽收尾（未处于拖拽时为 no-op）

    // 结束画布平移模式
    

        m_isPanning = false;
        // m_panMoved = false;
  

    mouseHeld = false;
    m_playheadSelected = false;
    // U4：播放头拖拽结束，停掉可能仍在跑的边缘自动横滚定时器
    //（tick 的兜底停止条件已不匹配 m_playheadSelected，这里主动收）
    if (m_edgeScrollTimer && m_edgeScrollTimer->isActive()) {
        m_edgeScrollDir = 0;
        m_edgeScrollTimer->stop();
    }
    m_mouseEnd = event->pos();
    updateCursorShape();
    //pressed outside of selection
    QAbstractItemView::mouseReleaseEvent(event);
}

void BaseTimelineView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    // 创建并显示属性对话框
    showClipProperty(index);
    QAbstractItemView::mouseDoubleClickEvent(event);
}

void BaseTimelineView::leaveEvent(QEvent *event)
{
    finishClipDrag();
    finishKeyframeDrag(); // 拖拽中鼠标离开视口：同样清态（光标随 updateCursorShape 恢复）
    mouseHeld = false;
    // selectionModel()->clear();
    m_hoverIndex = QModelIndex();
    QAbstractItemView::leaveEvent(event);
}

// 设置缩放比例（锚点=播放头）
void BaseTimelineView::setScale(double value)
{
    /**
     * 兼容入口：以播放头为锚点缩放（滑条等既有调用方语义不变）。
     */
    if (getModel())
        setScaleAt(value, getModel()->getPlayheadPos());
    else
        setScaleAt(value, 0);
}

void BaseTimelineView::setScaleAt(double value, qint64 anchorUs)
{
    /**
     * 根据归一化比例更新本视图实例的时间刻度（像素/μs），并保持锚点时刻
     * 在视口中的像素位置不变（2026-11-10 缩放焦点修复）。
     *
     * 旧实现三个缺陷：
     * 1. 改完 m_timescale 后未先 updateScrollBars 就 setScrollOffsetClamped
     *    ——夹紧用的是旧 timescale 口径的 maximum，放大时焦点修正量被错误
     *    夹掉（焦点漂移），且缩放后滚动条 range 不刷新（放大滚不到右侧、
     *    缩小滚进空白）；
     * 2. 焦点恒为播放头——播放头不在视口内时，缩放导致视口内容大幅平移，
     *   体感"画面乱跳"；Ctrl+滚轮应以鼠标位置为锚（主流时间轴语义）；
     * 3. 焦点像素计算每步重复 round，锚点式表达更直接：
     *    scrollNew = anchorUs*newTs - (anchorUs*oldTs - scrollOld)。
     */
    // 1. 旧刻度保存 + 输入钳制 + 无变化短路（避免同值重算引发闪烁）
    const double oldTimescale = m_timescale;
    const double ratio = qBound(0.0, value, 1.0);
    const double newTimescale = m_minTimescale
        + ratio * (static_cast<double>(m_baseTimeScale) - m_minTimescale);
    if (newTimescale == oldTimescale)
        return;
    m_timescale = newTimescale;

    // 1.5 锚点钳制到内容域（2026-11-10 滚动体感修复）：
    //     锚点在空白区/内容外时（鼠标停在右缘空白反复缩放），锚点时刻恒
    //     保持屏幕位置 → 内容被持续推离视口（"很容易就把片段缩放到视口
    //     之外"）。钳到 [0, 内容末端] 后，内容末端保持在鼠标下，缩放再
    //     激进内容也至少部分可见。
    if (model()) {
        const qint64 contentUs =
            model()->data(QModelIndex(), TimelineRoles::TimelineLengthRole)
                .toLongLong();
        if (contentUs > 0)
            anchorUs = qBound<qint64>(0, anchorUs, contentUs);
    }

    // 2. 锚点保持：目标滚动偏移 = 锚点新像素位置 - 锚点旧视口内偏移
    //    （先算好目标，再更新 range——缩小场景 Qt 会在 setRange 时自动夹紧
    //    value 并同步 m_scrollOffset，不能在 updateScrollBars 之后再用缓存值）
    const int anchorPxOld = qRound(static_cast<double>(anchorUs) * oldTimescale);
    const int anchorPxNew = qRound(static_cast<double>(anchorUs) * m_timescale);
    const int targetX = qMax(0, anchorPxNew - (anchorPxOld - m_scrollOffset.x()));

    // 3. 刻度已变，旧缓存不能按像素平移；缩放属于大变化，整页重建
    m_staticCacheDirty = true;
    currentScale = ratio;
    // P6：缩放改变所有片段的像素几何，内容矩形缓存整体失效
    m_clipContentRects.clear();

    // 4. 先按新口径刷新滚动条 range，再显式设置目标偏移（夹紧按新 range 进行）
    updateScrollBars();
    setScrollOffsetClamped(QPoint(targetX, m_scrollOffset.y()));

    // 5. 更新界面
    scheduleRedraw();
}

void BaseTimelineView::setScaleAtPx(double value, int anchorViewportX)
{
    /**
     * U6（timeline-draw-zoom-review）：像素锚点缩放。
     * 锚点内容像素（浮点，不取整）= anchorViewportX + scrollOld；
     * 保持锚点在视口中的像素位置不动 →
     *   targetScroll = anchorContentPx × newTs/oldTs − anchorViewportX
     * 与 setScaleAt 的 μs 锚公式同构，但消掉 pointToFrame(qRound64) 与
     * anchorUs×ts(qRound) 两层取整——连续缩放每步 ±0.5px 随机蠕动消除。
     */
    const double oldTs = m_timescale;
    const double ratio = qBound(0.0, value, 1.0);
    const double newTs = m_minTimescale
        + ratio * (static_cast<double>(m_baseTimeScale) - m_minTimescale);
    if (newTs == oldTs)
        return;

    // 锚点内容像素钳到内容域（与 setScaleAt 的 μs 域钳制同语义：
    // 锚点在空白区反复缩放时内容末端保持在锚点屏幕位置）
    double anchorContentPx =
        static_cast<double>(anchorViewportX) + m_scrollOffset.x();
    if (model()) {
        const qint64 contentUs = model()->data(
            QModelIndex(), TimelineRoles::TimelineLengthRole).toLongLong();
        if (contentUs > 0)
            anchorContentPx = qBound(0.0, anchorContentPx, contentUs * oldTs);
    }

    m_timescale = newTs;

    const double targetXd = anchorContentPx * (newTs / oldTs) - anchorViewportX;

    m_staticCacheDirty = true;
    currentScale = ratio;
    m_clipContentRects.clear(); // P6：缩放改变所有片段像素几何

    updateScrollBars();
    setScrollOffsetClamped(QPoint(
        qMax(0, static_cast<int>(std::lround(targetXd))), m_scrollOffset.y()));

    scheduleRedraw();
}

void BaseTimelineView::updateEdgeScrollDir()
{
    /**
     * U5（timeline-draw-zoom-review）：边缘横滚深度分档——越缘越深滚越快。
     * 缘带 48px 内按"距被压缘的深度"分三档：内带 <16px = 36px/tick、
     * 中带 16~32 = 24、外带 32~48 = 12（旧实现恒速 24）。步长直接并入
     * m_edgeScrollDir（带符号 px/tick），tick 逻辑不变；启停仍在
     * handleMouseDrag 事件流内判定——合成事件不进事件循环 → 零干扰契约保持。
     */
    const int edge = 48;
    const int vw = viewport()->width();
    const int mx = m_mouseEnd.x();
    int step = 0;
    if (mx < edge) {
        const int d = qMax(0, mx); // 距左缘深度（鼠标出界 = 压边）
        step = (d < 16) ? 36 : (d < 32 ? 24 : 12);
        m_edgeScrollDir = -step;
    } else if (mx > vw - edge) {
        const int d = qMax(0, vw - mx);
        step = (d < 16) ? 36 : (d < 32 ? 24 : 12);
        m_edgeScrollDir = step;
    } else if (m_edgeScrollDir != 0) {
        m_edgeScrollDir = 0; // 离缘即停（定时器 tick 兜底 stop）
        m_edgeScrollTimer->stop();
        return;
    }
    if (step != 0 && !m_edgeScrollTimer->isActive())
        m_edgeScrollTimer->start();
}

void BaseTimelineView::resizeEvent(QResizeEvent *event)
{
    scheduleRedraw();
    QAbstractItemView::resizeEvent(event);

    // 更新ZoomController
    // emit timelineInfoChanged(getTrackWdith(), viewport()->width(), m_scrollOffset.x());
}

void BaseTimelineView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    /**
     * P6（timeline-draw-zoom-review）：行删除（片段或整条轨道）前清空
     * 内容矩形缓存。m_clipContentRects 的键是裸 AbstractClipModel*，行删除
     * 后这些对象随之销毁，残留条目=悬垂键；指针地址被新 clip 复用时，
     * updateClipGeometry 会读到过期的 QRect 造成脏区错算/伪影。
     * 基类处理后走全量重绘路径重建缓存。
     */
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    m_clipContentRects.clear();
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}

void BaseTimelineView::showEvent(QShowEvent *event)
{
    /**
     * @brief 显示事件
     * @details
     * 兜底逻辑：
     * - 如果外部没有注入共享 selectionModel，则创建一个本地 selectionModel。
     * - 如果本视图未被 Controller 托管（独立使用），则在此处按需绑定“模型变化 -> 合并重绘”。
     */
    if (!selectionModel() && model()) {
        setSelectionModel(new QItemSelectionModel(model(), this));
    }

    // 独立使用场景：没有 Controller 时，视图需要自己监听模型变化来刷新
    if (!property("qtimeline_managed").toBool() && !property("qtimeline_model_bound").toBool() && Model) {
        connect(Model, &QAbstractItemModel::modelReset,   this, [this]() { scheduleRedraw(); });
        connect(Model, &QAbstractItemModel::rowsInserted, this, [this]() { scheduleRedraw(); });
        connect(Model, &QAbstractItemModel::rowsRemoved,  this, [this]() { scheduleRedraw(); });
        connect(Model, &QAbstractItemModel::rowsMoved,    this, [this]() { scheduleRedraw(); });
        connect(Model, &QAbstractItemModel::dataChanged,  this, [this]() { scheduleRedraw(); });

        connect(Model, &BaseTimeLineModel::S_playheadMoved, this, [this](qint64 us) { onPlayheadMoved(us); });
        connect(Model, &BaseTimeLineModel::S_LengthChanged,       this, [this]() { scheduleRedraw(); });

        setProperty("qtimeline_model_bound", true);
    }

    updateScrollBars();
    QAbstractItemView::showEvent(event);
}

void BaseTimelineView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        QAbstractItemView::mousePressEvent(event);
        return;
    }

    m_mouseStart = event->pos();
    m_mouseEnd = event->pos();
    mouseHeld = true;

    // 清除当前选择（selectionModel 可能由外部注入；兜底情况也可能尚未创建）
    if (selectionModel()) {
        selectionModel()->clearSelection();
    }

    //鼠标点击位置在标尺上就移动播放头
    if (m_mouseStart.y() <= rulerHeight + toolbarHeight)
    {
        m_playheadSelected = true;
        return;
    }
    QModelIndex item = indexAt(event->pos());
    //如果点击在轨道上的的空白处
    //【2026-11-06 用户定案】内容区点击/拖拽不再移动播放头——播放头仅
    // 标尺拖拽生效（上方 y<=rulerHeight+toolbarHeight 分支）。原语义
    // 会与展开区 TweenPanel 拖键冲突（panel 空白按下冒泡到视图误触
    // 播放头横移）。内容区空白现在只清除选择（上方已做），不再选播放头。
    if(item.isValid() && !item.parent().isValid()) {
        m_playheadSelected = false;
        // ---- 片段外缘按下（2026-11-11 "向外判断"定案）----------------------
        // 贴片段外缘 ±kTrimBandPx 按下也要能抓到拉伸（悬停外扩判定同款
        // 几何）：updateMouseHoverState 外扩分支若判出边缘 → 选中该片段
        // 并进入拉伸拖拽（片段内按下同款状态机；m_mouseUnderClipEdge 已
        // 锁边，mouseHeld 期间不重算）。判不出（真空白）则维持原语义。
        updateMouseHoverState(event);
        if (m_mouseUnderClipEdge == hoverState::LEFT
                || m_mouseUnderClipEdge == hoverState::RIGHT) {
            selectionModel()->setCurrentIndex(m_hoverIndex,
                                              QItemSelectionModel::ClearAndSelect);
            setCursor(Qt::ClosedHandCursor);
            viewport()->update();
            event->accept();
            return;
        }
        return;
    }
    m_playheadSelected = false;

    // 如果点击到了片段
    if(item.isValid() && item.parent().isValid()) {
        // 设置当前项和选择
        selectionModel()->setCurrentIndex(item, QItemSelectionModel::ClearAndSelect);
        // ---- 关键帧命中优先（2026-11-05；WATCHOUT 式拖拽 > 片段拖拽）----
        // 向模型询问该点是否落在关键帧标记上（默认不命中）；命中则进入
        // 关键帧拖拽状态，不再设置 ClosedHandCursor/片段拖拽偏移。
        const QRect kfClipRect = visualRect(item).adjusted(0, clipoffset, 0, -clipoffset);
        const AbstractClipModel::KeyframeHit kf = hitKeyframeAt(item, m_mouseStart);
        if (kf.hit) {
            m_keyframeDragClip = static_cast<AbstractClipModel*>(item.internalPointer());
            m_keyframeDragIndex = item;
            m_keyframeDragChannel = kf.channel;
            m_keyframeDragKey = kf.key;
            m_keyframeDragRect = kfClipRect; // 拖拽垂直分量按此矩形换算 local y
            // SizeAll（四向）：水平拖=时间、垂直拖=值（WATCHOUT tween 点语义）
            setCursor(Qt::SizeAllCursor);
            event->accept();
            return;
        }
        // ---- 点击带内空白 = 片段拖拽（2026-11-09 合并关键帧模式）--------
        // 点击加键已退役（加键唯一入口 = 属性面板改值 auto-key）——模型
        // addKeyframeAt 保留接口但实现恒空；点击空白不再消费，直接落回
        // 片段拖拽（ClosedHand + 拖拽偏移）。
        // 拉伸模式按下锁定（2026-11-11）：先按当前点位重算边缘判定——
        // m_mouseUnderClipEdge 随后整个拖拽过程不再变（mouseHeld 期间
        // 不重算悬停），拖拽分支（handleMouseDrag switch）据此选边。
        // 旧实现直接沿用上一次悬停残留值，快速按下会拖错模式。
        updateMouseHoverState(event);
        setCursor(Qt::ClosedHandCursor);
        // μs 域 qint64 直传（2026-11-11：原 toInt() 截断——入点 >35.8min
        // 回绕致移动偏移错位；frameToPoint 参数本就是 qint64）
        m_mouseOffset.setX(frameToPoint(
            m_hoverIndex.data(TimelineRoles::ClipInRole).toLongLong())
            - m_mouseStart.x());
        viewport()->update();
        return;
    }
    if(!item.isValid() && !item.parent().isValid()) {
        // 点击在空白处，不在轨道、片段、播放头上
        m_isPanning = true;
        m_panStart = event->pos();
        // 开始平移前同步一次滚动偏移缓存，避免历史状态不一致
        syncScrollOffsetFromScrollBars();
        m_panInitialOffset = m_scrollOffset;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    
}

/**
 * 鼠标移动事件处理函数
 * 处理鼠标移动时的各种交互行为
 * @param event 鼠标事件对象
 */
void BaseTimelineView::mouseMoveEvent(QMouseEvent *event)
{
    // 处理右键事件
    if (event->button() == Qt::RightButton) {
        QAbstractItemView::mouseMoveEvent(event);
        return;
    }
    // 处理鼠标按住拖动的情况
    if (mouseHeld) {
        handleMouseDrag(event);
        return;
    }

    // 处理鼠标悬停状态
    updateMouseHoverState(event);
    updateCursorShape();
    
    QAbstractItemView::mouseMoveEvent(event);
}

/**
 * 处理鼠标拖动操作
 * @param event 鼠标事件对象
 */
void BaseTimelineView::handleMouseDrag(QMouseEvent *event)
{
    m_mouseEnd = event->pos();

    // ---- 关键帧拖拽（最具体的状态优先判断）----
    // 帧量化：像素 → 片段本地帧（pointToFrame 已四舍五入到整帧）；
    // 片段边界钳制在此完成，邻居升序钳制由模型 moveKeyframe 负责
    // （它最了解自己关键帧的语义）。落点变化经 updateClipGeometry 走
    // 静态缓存局部重绘——菱形/曲线即时跟手。
    // 垂直分量（2026-11-05 曲线编辑带）：片段内 local y 传给模型，由模型
    // 换算为业务值（值域映射是模型知识，视图不做归一化假设）。
    if (m_keyframeDragClip && m_keyframeDragIndex.isValid()) {
        const qint64 absUs = pointToFrame(qMax(0, m_mouseEnd.x() + m_scrollOffset.x()));
        double local = static_cast<double>(absUs - m_keyframeDragClip->start());
        local = qBound(0.0, local, static_cast<double>(qMax<qint64>(1, m_keyframeDragClip->length())));
        const double localY =
            static_cast<double>(m_mouseEnd.y() - m_keyframeDragRect.top());
        m_keyframeDragClip->moveKeyframe(m_keyframeDragChannel, m_keyframeDragKey,
                                         local, localY, m_keyframeDragRect);
        updateClipGeometry(m_keyframeDragIndex);
        event->accept();
        return;
    }

    // 处理播放头拖动
    if (m_playheadSelected) {
        movePlayheadToFrame(pointToFrame(std::max(0, m_mouseEnd.x() + m_scrollOffset.x())));

        // U5：深度分档判定收编进共用 updateEdgeScrollDir（±12/±24/±36）
        updateEdgeScrollDir();
        return;
    }
    // 处理画布平移
    if (m_isPanning) {
        const int dx = event->pos().x() - m_panStart.x();
        const int dy = event->pos().y() - m_panStart.y();
        // 仅水平平移
        QPoint newOffset = m_panInitialOffset - QPoint(dx, 0);

        // 约束滚动范围：水平范围由滚动条/视口宽度决定
        const int minX = 0;
        const int maxX = qMax(minX, getTrackWdith() - viewport()->width());
        newOffset.setX(qBound(minX, newOffset.x(), maxX));

        // 通过滚动条驱动平移，避免直接写 m_scrollOffset 导致状态不同步
        if (newOffset != m_scrollOffset) {
            // 滚动条变化会进入 scrollContentsBy，在那里平移缓存。
            setScrollOffsetClamped(newOffset);
        }
        event->accept();
        return;
    }
    if (!selectionModel())
        return;
    const QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty())
        return;

    const QModelIndex clipIndex = selectedIndexes.first();
    auto* clip = static_cast<AbstractClipModel*>(clipIndex.internalPointer());
    if (!clip)
        return;

    const bool moving = m_mouseUnderClipEdge == hoverState::CENTER;
    const bool resizing = m_mouseUnderClipEdge == hoverState::LEFT || m_mouseUnderClipEdge == hoverState::RIGHT;
    if (!moving && !resizing)
        return;

    if (!m_interactiveDrag)
        beginClipDrag(clip, clipIndex);

    // Trigger 等固定长度片段会把标题画到几何矩形右侧外（向右约 40px）。
    // 只刷新 visualRect 时，这段旧文字清不掉，拖动就会叠成一串。
    const QRect oldRect = visualRect(clipIndex).adjusted(-4, -4, 44, 4);
    // ---- 磁性吸附（2026-11-09）：候选 = 原点 + 他片段入/出点（跨轨道同权）--
    // 移动时拖拽片段的入/出双边缘竞争吸附（像素距离近者胜）；裁边时仅被拖
    // 的边缘参与。命中后在 paintEvent 画对齐参考线，拖离/松手清除。
    const qint64 prevSnapLine = m_snapLineFrame;
    const QList<qint64> snapCandidates = collectSnapCandidates(clip);

    // ---- 边缘自动横滚（2026-11-10 滚动体感修复）--------------------------
    // 拖动/裁边时鼠标压住视口左右缘 → 内容自动跟随（主流 NLE 语义）。此前
    // 无此机制：片段拖到视口外后既无跟随、拖拽中模型 defer 通知 →
    // m_lengthFrame 冻结 → 滚动 range 冻结，用户无法把片段拉回视口
    //（"无法拉回来"）。松手 performRedraw 会按模型真值重算 range 覆盖。
    // 实现=定时器驱动（构造函数里的 m_edgeScrollTimer）：这里只计算方向并
    // 启停定时器，不逐事件立即滚——真实拖拽 50ms 步进体感流畅，合成事件
    //（sendEvent 不进事件循环）不触发定时器，吸附/探针单测零干扰。
    // U5：恒速改深度分档（越缘越快），判定收编进 updateEdgeScrollDir。
    updateEdgeScrollDir();

    switch (m_mouseUnderClipEdge) {
        case hoverState::CENTER: {
            // 期望新入点（μs）。m_mouseOffset 混合内容 x 与视口 x，但含
            // (mouseEnd - press) 差值，天然滚动不变（与原实现同式）。
            const qint64 desiredStart = pointToFrame(m_mouseEnd.x() + m_mouseOffset.x());
            const qint64 len = clip->length();
            qint64 lineStart = -1, lineEnd = -1;
            const qint64 snappedStart = snapFrame(desiredStart, snapCandidates, &lineStart);
            const qint64 snappedEnd   = snapFrame(desiredStart + len, snapCandidates, &lineEnd);
            qint64 newStart = desiredStart;
            if (lineStart >= 0 && lineEnd >= 0) {
                // 双边缘同时落入阈值：距离小者胜（同一 timescale 下帧距
                // 与像素距等价，直接比较帧距）
                if (qAbs(snappedStart - desiredStart)
                        <= qAbs(snappedEnd - (desiredStart + len))) {
                    newStart = snappedStart;
                    m_snapLineFrame = lineStart;
                } else {
                    newStart = snappedEnd - len;
                    m_snapLineFrame = lineEnd;
                }
            } else if (lineStart >= 0) {
                newStart = snappedStart;
                m_snapLineFrame = lineStart;
            } else if (lineEnd >= 0) {
                newStart = snappedEnd - len;
                m_snapLineFrame = lineEnd;
            } else {
                m_snapLineFrame = -1;
            }
            moveSelectedClip(newStart, m_mouseEnd.y() + m_mouseOffset.y());
            break;
        }
        case hoverState::LEFT: {
            const qint64 desired = pointToFrame(m_mouseEnd.x() + m_scrollOffset.x());
            qint64 line = -1;
            const qint64 snapped = snapFrame(desired, snapCandidates, &line);
            m_snapLineFrame = line;
            getModel()->setData(clipIndex, snapped, TimelineRoles::ClipInRole);
            break;
        }
        case hoverState::RIGHT: {
            const qint64 desired = pointToFrame(m_mouseEnd.x() + m_scrollOffset.x());
            qint64 line = -1;
            const qint64 snapped = snapFrame(desired, snapCandidates, &line);
            m_snapLineFrame = line;
            getModel()->setData(clipIndex, snapped, TimelineRoles::ClipOutRole);
            break;
        }
        default:
            break;
    }
    // 吸附参考线出现/消失/移位：线可能远离片段矩形，下方脏矩形覆盖不到。
    // P2（timeline-draw-zoom-review）：只重绘新旧线位置的窄条带（1px 线
    // ±2px 余量），替代整页 update——拖拽中吸附线频繁进出，整页重绘曾是
    // 拖拽帧耗的大头。paintEvent 的 clip 自动收窄到 update 区域，静态缓存
    // blit 与动态层绘制都只做条带。
    if (m_snapLineFrame != prevSnapLine) {
        const int pad = 2;
        const int vh = viewport()->height();
        QRegion strip;
        if (prevSnapLine >= 0) {
            const int x = frameToPoint(prevSnapLine) - scrollX();
            strip += QRect(x - pad, 0, pad * 2 + 1, vh);
        }
        if (m_snapLineFrame >= 0) {
            const int x = frameToPoint(m_snapLineFrame) - scrollX();
            strip += QRect(x - pad, 0, pad * 2 + 1, vh);
        }
        viewport()->update(strip); // 都为空（-1→-1 不可能；均 <0 越界）时 no-op
    }
    const QRect nextRect = visualRect(clipIndex).adjusted(-4, -4, 44, 4);
    viewport()->update(oldRect.united(nextRect));
}

void BaseTimelineView::beginClipDrag(AbstractClipModel* clip, const QModelIndex& index)
{
    if (!clip || !index.isValid() || !Model || m_interactiveDrag)
        return;
    m_interactiveDrag = true;
    m_dragClip = clip;
    m_dragIndex = index;
    m_snapLineFrame = -1; // 新拖拽开始：清上一轮参考线
    Model->beginInteractiveGeometryChange(clip);
    // 不在 QAbstractScrollArea 上 grabMouse。按下时鼠标已经由视口捕获，
    // 再抢到外层视图会让松手事件到不了 mouseReleaseEvent，拖拽无法结束。
    m_staticCacheDirty = true;
}

void BaseTimelineView::finishClipDrag()
{
    if (!m_interactiveDrag)
        return;
    m_interactiveDrag = false;
    // 边缘自动横滚随拖拽结束停止（2026-11-10）
    m_edgeScrollDir = 0;
    if (m_edgeScrollTimer && m_edgeScrollTimer->isActive())
        m_edgeScrollTimer->stop();
    m_dragClip = nullptr;
    m_dragIndex = QPersistentModelIndex();
    // dataChanged 会按旧位置和新位置补进缓存，避免松手后再整页重建。
    if (Model)
        Model->endInteractiveGeometryChange();
    // 吸附参考线随拖拽结束消失（release/leave 两路都经此处收尾）
    if (m_snapLineFrame >= 0) {
        m_snapLineFrame = -1;
        if (viewport())
            viewport()->update();
    }
}

// ---- 磁性吸附（2026-11-09 用户需求：片段拖拽对齐）实现 --------------------

QList<qint64> BaseTimelineView::collectSnapCandidates(const AbstractClipModel* exclude) const
{
    // 候选 = 时间线原点 + 全部轨道全部片段的入/出点。
    // 跨轨道同权：邻轨片段边缘同样吸附（用户需求"相邻的轨道间片段拖拽
    // 时有对齐效果"）。重复候选无害（最近距离搜索，先到先得）。
    QList<qint64> candidates;
    if (!Model)
        return candidates;
    candidates.append(0);
    for (int row = 0; row < Model->rowCount(); ++row) {
        const QModelIndex trackIndex = Model->index(row, 0);
        const auto* track = static_cast<const TrackData*>(trackIndex.internalPointer());
        if (!track)
            continue;
        for (const AbstractClipModel* c : track->clips) {
            if (!c || c == exclude)
                continue;
            candidates.append(c->start());
            candidates.append(c->end());
        }
    }
    return candidates;
}

qint64 BaseTimelineView::snapFrame(qint64 desired, const QList<qint64>& candidates, qint64* snapLine) const
{
    *snapLine = -1;
    if (!m_snapEnabled)
        return desired;
    // 像素阈值 → 时刻阈值（随缩放换算）：任何缩放下吸附半径手感一致
    const qint64 thr = qMax<qint64>(1, qRound64(static_cast<double>(m_snapThresholdPx) / m_timescale));
    qint64 best = desired;
    qint64 bestDist = thr + 1;
    for (qint64 c : candidates) {
        const qint64 d = qAbs(desired - c);
        if (d <= thr && d < bestDist) {
            bestDist = d;
            best = c;
            *snapLine = c;
        }
    }
    return best;
}

void BaseTimelineView::drawSnapLine(QPainter* painter) const
{
    if (m_snapLineFrame < 0 || !viewport())
        return;
    const int x = frameToPoint(m_snapLineFrame) - m_scrollOffset.x();
    if (x < 0 || x > viewport()->width())
        return;
    painter->save();
    QPen pen(m_snapLineColor, 1);
    pen.setStyle(Qt::DashLine); // 虚线：与播放头实线区分（临时参考线语义）
    painter->setPen(pen);
    painter->drawLine(x, contentTop(), x, viewport()->height());
    painter->restore();
}

// ---- 拉伸方向光标（2026-11-11 用户定案：光标本身分左右 > 边缘绿条）--------

namespace {
// 拉伸判定带（像素）——固定值不随缩放（用户定案：缩放改变的是片段像素宽，
// 判定带恒定才能保证片段极窄时依然准确抓到边缘）。跨边语义：内侧 band+
// 外侧 band 都算"贴边"（光标贴片段外缘也判拉伸——窄片段不再需要瞄准
// 几像素的本体）。
constexpr int kTrimBandPx = 6;

// 自定义 trim 光标：中竖线（压在被拉的边缘上）+ 单侧三角（指示拖动方向）。
// 白身黑边——深/浅色轨道上都可见。QCursor 按值持有 pixmap，懒构造一次。
// 32 逻辑像素（2026-11-11 用户反馈 16 太小）+ 按 devicePixelRatio 物理超
// 采样绘制（1x/2x 屏都清晰；热点恒在竖线中心）。
QCursor makeTrimCursor(bool left, qreal dpr)
{
    const int phys = qRound(32 * dpr);
    QPixmap pm(phys, phys);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    // 中竖线（热点处，pointer 落点=被拉边缘）：4 逻辑 px 宽黑底 + 2px 白芯
    p.fillRect(14, 6, 4, 20, Qt::black);
    p.fillRect(15, 7, 2, 18, Qt::white);
    // 单侧三角箭头：左缘光标朝左、右缘光标朝右（拖动方向预告）
    QPolygonF tri;
    if (left)  tri << QPointF(4, 16)  << QPointF(13.5, 9)  << QPointF(13.5, 23);
    else       tri << QPointF(27, 16) << QPointF(17.5, 9)  << QPointF(17.5, 23);
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(Qt::white);
    p.drawPolygon(tri);
    p.end();
    // 热点=竖线中心（与悬停判定的 event->pos() 同点位，视觉零偏移）
    return QCursor(pm, 16, 16);
}
} // namespace



// ---- 关键帧子元素命中/拖拽（2026-11-05 Tween 集成）实现 --------------------

AbstractClipModel::KeyframeHit BaseTimelineView::hitKeyframeAt(
        const QModelIndex& index, const QPoint& pos) const
{
    if (!index.isValid() || !index.parent().isValid())
        return {};
    auto* clip = static_cast<AbstractClipModel*>(index.internalPointer());
    if (!clip)
        return {};
    // 与 drawTracks / paintEvent 直绘完全同一套片段矩形（clipoffset 内缩
    // 的"浮起"外观）——关键帧命中坐标才能和画出来的菱形逐像素对上。
    const QRect clipRect = visualRect(index).adjusted(0, clipoffset, 0, -clipoffset);
    if (!clipRect.contains(pos))
        return {};
    // 是否可交互由模型自决（2026-11-09 合并关键帧模式：有键片段的片段带
    // 是主编辑界面；无键片段模型恒不命中——"一处守卫"移交模型 hasTweenKeys）
    return clip->hitKeyframe(pos - clipRect.topLeft(), clipRect);
}

void BaseTimelineView::finishKeyframeDrag()
{
    if (!m_keyframeDragClip)
        return; // 未处于关键帧拖拽：no-op
    m_keyframeDragClip = nullptr;
    m_keyframeDragIndex = QPersistentModelIndex();
    m_keyframeDragChannel = -1;
    m_keyframeDragKey = -1;
    m_keyframeDragRect = QRect();
    updateCursorShape(); // m_mouseUnderClipEdge 仍持旧值，悬停判定下一拍会重算
}

/**
 * 更新鼠标悬停状态
 * @param event 鼠标事件对象
 *
 * 拉伸边缘判定（2026-11-11 用户定案"向外判断"）：
 *   * 判定带 = 跨边内外各 kTrimBandPx（固定像素，不随缩放）——光标贴在
 *     片段**外侧**边缘也能抓到拉伸，片段再窄无须瞄准几像素的本体；
 *   * 片段内路径：indexAt 命中（含 <15px 最小命中宽扩展）→ 菱形优先 →
 *     跨边带内近缘优先（dL/dR 取绝对值比远近）；
 *   * 片段外路径：indexAt 未命中片段时，同轨道 ±kTrimBandPx 帧域窗口
 *     搜最近边缘（间隙贴边/宽片段外缘都能判定）；
 *   * 窄片段（≤2×band）中线二分保持——左半恒左缘、右半恒右缘。
 */
void BaseTimelineView::updateMouseHoverState(QMouseEvent *event)
{
    QPoint pos = event->pos();
    // 拉伸边缘高亮（2026-11-11）：变化检测用——旧状态/旧悬停片段的边缘
    // 条带需要在状态切换时擦除
    const hoverState oldEdge = m_mouseUnderClipEdge;
    const QModelIndex oldHover = m_hoverIndex;
    m_hoverIndex = indexAt(pos);
    QRect rect = visualRect(m_hoverIndex);

    // 默认情况下无悬浮状态
    m_mouseUnderClipEdge = hoverState::NONE;

    // 检查是否悬停在片段上
    if (m_hoverIndex.isValid() && m_hoverIndex.parent().isValid())
     {
        // 获取片段对象
        AbstractClipModel* clip = static_cast<AbstractClipModel*>(m_hoverIndex.internalPointer());
        if (!clip) {
            return;
        }
        // 设置悬停状态为中心
        m_mouseUnderClipEdge = hoverState::CENTER;
        // ---- 关键帧菱形悬停优先于边缘 resize（菱形是小目标，必须抢在
        // 整条左/右边缘判定之前；未命中则落回边缘/中心判定）----
        if (hitKeyframeAt(m_hoverIndex, pos).hit) {
            m_mouseUnderClipEdge = hoverState::KEYFRAME;
            return;
        }
        // 检查是否可调整大小的片段
        if (clip->isResizable()) {
            // ---- 跨边判定带（2026-11-11 "向外判断"定案）------------------
            // dL/dR 为负 = 光标在片段外侧（indexAt 的最小命中宽扩展把
            // <15px 窄片段的外侧点击也带进本分支，负值参与近缘比较自然
            // 归位）。固定 kTrimBandPx 像素带，不随缩放。
            const int w  = rect.width();
            const int dL = pos.x() - rect.left();
            const int dR = rect.right() - pos.x();
            if (w <= 2 * kTrimBandPx) {
                // 窄片段（判定带互相覆盖且溢出片段本体）：中线二分，
                // 左半恒左缘、右半恒右缘，物理上无歧义
                if (pos.x() * 2 <= rect.left() + rect.right())
                    m_mouseUnderClipEdge = hoverState::LEFT;
                else
                    m_mouseUnderClipEdge = hoverState::RIGHT;
            }
            // 带内近缘优先：同时落入左右带（间隙极窄时）取更近侧
            else if (qAbs(dL) <= kTrimBandPx && qAbs(dL) <= qAbs(dR)) {
                m_mouseUnderClipEdge = hoverState::LEFT;
            }
            else if (qAbs(dR) <= kTrimBandPx) {
                m_mouseUnderClipEdge = hoverState::RIGHT;
            }
        }
    }
    else if (pos.y() >= contentTop() && model()) {
        // ---- 片段外：外扩带查找（光标贴片段外缘 ±band 也能抓到拉伸）----
        // indexAt 只在片段矩形（含窄片段最小命中宽扩展）内命中；宽片段
        // 外缘与片段间隙的光标落点走这里——按"边缘距离最近"归位。
        const int contentY = pos.y() + verticalScrollBar()->value() - contentTop();
        const int row = static_cast<BaseTimeLineModel*>(model())->trackRowAt(contentY);
        if (row >= 0 && row < model()->rowCount()) {
            const QModelIndex trackIndex = model()->index(row, 0);
            const QRect trackRect = visualRect(trackIndex);
            if (trackRect.contains(pos)) {
                auto* track = static_cast<TrackData*>(trackIndex.internalPointer());
                const int contentX = pos.x() + horizontalScrollBar()->value();
                // ±band 帧域过滤窗口（qint64——μs 反算量级防御，同 click 命中）
                const qint64 frameLo = pointToFrame(qMax(0, contentX - kTrimBandPx)) - 1;
                const qint64 frameHi = pointToFrame(contentX + kTrimBandPx) + 1;
                QList<int> clipRows;
                clipRowsInFrameRange(track, frameLo, frameHi, clipRows);
                int bestDist = 1 << 30;
                for (int clipRow : clipRows) {
                    const QModelIndex ci = model()->index(clipRow, 0, trackIndex);
                    auto* clip = static_cast<AbstractClipModel*>(ci.internalPointer());
                    if (!clip || !clip->isResizable()) continue;
                    const QRect r = visualRect(ci);
                    if (!r.isValid()) continue;
                    // 光标到该片段左右缘的距离（间隙贴边时两侧片段都入
                    // 选，取边缘距离最近者；区间内光标已被 indexAt 分支
                    // 处理，qAbs 使内外侧判定几何一致）
                    const int dL = qAbs(pos.x() - r.left());
                    const int dR = qAbs(r.right() - pos.x());
                    const int   dist   = qMin(dL, dR);
                    const hoverState edge = dL <= dR ? hoverState::LEFT
                                                     : hoverState::RIGHT;
                    if (dist < bestDist) {
                        bestDist  = dist;
                        m_hoverIndex        = ci;
                        m_mouseUnderClipEdge = edge;
                    }
                }
            }
        }
    }

    // ---- 拉伸边缘高亮的状态变化刷新：新旧悬停片段的左右缘窄条带重绘
    // （高亮条出现/消失/跨片段移动都覆盖；条带 2px 高亮 ±2px 余量）----
    if (oldEdge != m_mouseUnderClipEdge || oldHover != m_hoverIndex) {
        const int vh = viewport()->height();
        QRegion dirty;
        const auto addEdgeStrips = [this, &dirty, vh](const QModelIndex& idx) {
            if (!idx.isValid() || !idx.parent().isValid())
                return;
            const QRect r = visualRect(idx);
            if (!r.isValid())
                return;
            dirty += QRect(r.left() - 2, 0, 6, vh);
            dirty += QRect(r.right() - 3, 0, 6, vh);
        };
        addEdgeStrips(oldHover);
        addEdgeStrips(m_hoverIndex);
        if (!dirty.isEmpty())
            viewport()->update(dirty);
    }
}

/**
 * 根据鼠标悬停状态更新鼠标指针形状
 */
void BaseTimelineView::updateCursorShape()
{
    switch (m_mouseUnderClipEdge) {
        case hoverState::LEFT:
            // 拉伸方向光标（2026-11-11 用户定案替代边缘绿条）：中竖线+朝左
            // 三角——指针落点即被拉边缘，方向一眼可辨
            setCursor(makeTrimCursor(/*left=*/true, devicePixelRatioF()));
            break;
        case hoverState::RIGHT:
            setCursor(makeTrimCursor(/*left=*/false, devicePixelRatioF()));
            break;
        case hoverState::KEYFRAME:
            // 关键帧（曲线编辑带）：四向箭头——水平拖=时间、垂直拖=值
            //（WATCHOUT tween point 双向拖拽语义，2026-11-05 升级）
            setCursor(Qt::SizeAllCursor);
            break;
        case hoverState::CENTER:
            setCursor(Qt::OpenHandCursor);
            break;
        case hoverState::NONE:
        default:
            unsetCursor();
            break;
    }
}

void BaseTimelineView::dragEnterEvent(QDragEnterEvent *event)
{

        event->acceptProposedAction();
        m_isSupportMedia = true;
        // QAbstractItemView::dragEnterEvent(event);

}

void BaseTimelineView::dragMoveEvent(QDragMoveEvent *event)
{

    m_lastDragPos = event->position().toPoint();

    // 检查当前位置是否在轨道上
    QModelIndex trackIndex;


    // 排除工具栏和标尺区域
    if (m_lastDragPos.y() < rulerHeight + toolbarHeight) {
        event->ignore();
        m_isSupportMedia = false;
        return;
    }

    // 查找轨道
    bool onTrack = false;
    for (int i = 0; i < getModel()->rowCount(); i++) {
        if (visualRect(getModel()->index(i, 0)).contains(m_lastDragPos)) {
            trackIndex = getModel()->index(i, 0);
            onTrack = true;
            break;
        }
    }
    if (onTrack) {

        // 获取文件类型
        QString fileType = isMimeAcceptable(event->mimeData());
        // 获取轨道类型
        QString trackType = getModel()->data(trackIndex, TimelineRoles::TrackTypeRole).toString();

        // 判断文件类型和轨道类型是否匹配
        if (!fileType.isEmpty() && fileType == trackType) {
            // 类型匹配时接受拖动
            m_isSupportMedia = true;
            event->acceptProposedAction();

        }
        else {
            // 类型不匹配时拒绝拖动
            m_isSupportMedia = false;
            event->ignore();
            unsetCursor();
        }

    } else {
        QAbstractItemView::dragMoveEvent(event);
    }
}

void BaseTimelineView::dropEvent(QDropEvent *event)
{
    if(m_isSupportMedia){
        m_lastDragPos = event->position().toPoint();
        QModelIndex trackIndex;
        QRect rullerRect(-m_scrollOffset.x(),0,viewport()->width() + m_scrollOffset.x(),rulerHeight);
        /* If above or on the ruler drop on the first track*/
        if(m_lastDragPos.y()<0 || rullerRect.contains(m_lastDragPos)){
            if(getModel()->rowCount()>0)
                trackIndex = model()->index(0, 0);
        }else{
            /* Find track at drop point */
            for(int i = 0; i < getModel()->rowCount(); i++){
                if (visualRect(getModel()->index(i, 0)).contains(m_lastDragPos)){
                    trackIndex = getModel()->index(i,0);
                }
            }
        }
        /* If dropped out side of tracks */
        if(!trackIndex.isValid()){
            return;
        }
        const qint64 posUs = pointToFrame(m_lastDragPos.x());

        // 拖入文件 → 模型虚钩子（2026-11-11：文件路径不再丢弃——默认实现
        // 仍建占位片段；适配层覆写完成"导入+绑定+按素材时长设长"）
        const QString dropFile = event->mimeData()->urls().first().toLocalFile();
        getModel()->onDropMediaFile(trackIndex.row(), posUs, dropFile);
        viewport()->update();

    }else{
        QAbstractItemView::dropEvent(event);
    }
}

 bool BaseTimelineView::eventFilter(QObject *watched, QEvent *event)
 {
      if (event->type() == QEvent::KeyPress) {
          QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
          QModelIndexList list = selectionModel()->selectedIndexes();
          BaseTimeLineModel* timelinemodel = (BaseTimeLineModel*)model();
          if (watched == this) {
              switch (keyEvent->key()){
                  case Qt::Key_Right :
                      moveSelectedClip(1,0,false);
                      return true;  // 统一返回 true
                  case Qt::Key_Left:
                      moveSelectedClip(-1,0,false);
                      return true;
                  case Qt::Key_Delete: {
                      QModelIndex currentIndex = selectionModel()->currentIndex();
                      getModel()->onDeleteClip(currentIndex);
                      // 清除选择并发送 nullptr
                      selectionModel()->clearSelection();
                      viewport()->update();
                      return true;
                  }
                  case Qt::Key_Home:
                      // 一键回片头（2026-11-10 滚动体感修复："拉不回来"的兜底）
                      horizontalScrollBar()->setValue(0);
                      return true;
                  case Qt::Key_End:
                      // 一键跳内容末端
                      horizontalScrollBar()->setValue(
                          horizontalScrollBar()->maximum());
                      return true;
                  default:
                      break;
              }
          }
      }
      return QObject::eventFilter(watched, event);
 }

void BaseTimelineView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    syncScrollOffsetFromScrollBars();
    ensureStaticCache();

    QPainter painter(viewport());
    if (!m_staticCache.isNull())
        painter.drawPixmap(QPoint(0, 0), m_staticCache);
    if (m_interactiveDrag && m_dragClip && m_dragIndex.isValid()) {
        const QRect clipRect = visualRect(m_dragIndex).adjusted(0, clipoffset, 0, -clipoffset);
        const bool selected = selectionModel() && selectionModel()->isSelected(m_dragIndex);
        m_dragClip->paint(&painter, clipRect, selected);
    }
    drawSnapLine(&painter); // 磁性吸附对齐参考线（拖拽且吸附命中时出现）
    drawPlayhead(&painter);

    if (Model)
        m_paintedPlayheadUs = Model->getPlayheadPos();
}

void BaseTimelineView::drawBackground(QPainter* painter, const QRect& rect)
{
    // 绘制背景
    painter->fillRect(rect, m_timelineBgColor);

    // 设置分割线颜色并绘制轨道分隔线
    painter->setPen(m_timelineSeparatorColor);
    int firstRow = 0;
    int lastRow = -1;
    visibleTrackRange(firstRow, lastRow);
    if (lastRow < firstRow)
        return;
    for (int i = qMax(0, firstRow - 1); i <= lastRow; ++i) {
        // 2026-11-06 轨道展开：分隔线 y = 前面各行高之和（展开行使下方行下移）
        const int trackSplitter = static_cast<BaseTimeLineModel*>(model())->trackRowY(i + 1)
                                  + contentTop() - m_scrollOffset.y();
        if (trackSplitter < contentTop() || trackSplitter > rect.bottom())
            continue;
        painter->drawLine(0, trackSplitter, rect.width(), trackSplitter);
    }
}

void BaseTimelineView::drawVerticalTimeLines(QPainter* painter, const QRect& rect)
{
    // 2026-11-06 轨道展开：垂直网格线延伸到全部行高之和（展开行计入）
    int lineheight = static_cast<BaseTimeLineModel*>(model())->totalTracksHeight() + contentTop();

    // 计算刻度间隔（μs 漂亮档位；2026-11-10 微秒原生）
    const qint64 stepUs = calculateTimeStepUs();

    // 计算起始和结束位置（像素）
    qint64 firstUs = pointToFrame(m_scrollOffset.x());
    firstUs -= (firstUs % stepUs);
    const int startMarker = frameToPoint(firstUs);
    const int endMarker = rect.width() + m_scrollOffset.x();

    /**
     * P7（timeline-draw-zoom-review）：竖线 y 修正。
     * 旧实现起点写死 rulerHeight（把线画进工具栏/标尺区，靠绘制顺序掩盖），
     * 终点 lineheight 未减 scrollY（纵向滚动后线越出内容底伸进空白区）。
     * 修正：起点 = 标尺之下；终点 = 内容底转视口坐标后夹住视口。
     */
    const int lineTop = std::max(contentTop(), rect.top());
    const int lineBottom = std::min(lineheight - m_scrollOffset.y(), rect.bottom());
    if (lineBottom < lineTop)
        return;

    // 绘制垂直时间线（整数 tick 序号步进，消浮点累加的累积误差）
    // U7（timeline-draw-zoom-review）：主格像素间距足够宽时（与标尺文本
    // 同阈值），每主格内画 1/5 分格淡线——深放大（U3 上限 2.5e-3）时
    // 帧级/子帧级编辑的定位参照。子格 = 分隔色 40% 透明度。
    const double pxStep = m_timescale * static_cast<double>(stepUs);
    const bool drawSubTicks = pxStep >= 80.0;
    QPen subPen = painter->pen();
    if (drawSubTicks) {
        QColor c = m_timelineSeparatorColor;
        c.setAlpha(c.alpha() * 2 / 5);
        subPen.setColor(c);
    }
    for (qint64 k = 0;; ++k) {
        const int x = static_cast<int>(static_cast<double>(startMarker) + static_cast<double>(k) * pxStep);
        if (x >= endMarker)
            break;
        if (drawSubTicks) {
            painter->setPen(subPen);
            for (int j = 1; j < 5; ++j) {
                const int sx = static_cast<int>(
                    static_cast<double>(x) + static_cast<double>(j) * pxStep / 5.0);
                if (sx >= endMarker)
                    break;
                painter->drawLine(sx - m_scrollOffset.x(), lineTop,
                                  sx - m_scrollOffset.x(), lineBottom);
            }
            painter->setPen(m_timelineSeparatorColor);
        }
        painter->drawLine(x - m_scrollOffset.x(), lineTop,
                          x - m_scrollOffset.x(), lineBottom);
    }
}

qint64 BaseTimelineView::calculateTimeStepUs() const
{
    /**
     * 函数说明（2026-11-10 微秒原生）：
     * - 根据当前 m_timescale（像素/μs）和字体大小，计算一个不会导致标签
     *   重叠的时间步长（μs）。
     * - 从"漂亮时间档位"（100ms/250ms/500ms/1s/2s/5s/10s/30s/60s/5min/
     *   10min/30min/1h）中选取第一个不小于最小像素间距的档位。
     * - 2026-11-10 冗余清理：删除未使用的 frameRate 形参（μs 档位与
     *   帧率无关——帧域时代按帧步进才需要它）。
     */

    // 估算标签的最小像素间距：与字体大小相关（hh:mm:ss:mmm 文本较宽，
    // 与 drawTimeMarkers 的 canDrawText 阈值保持同一口径）
    const int minPixelSpacing = qMax(80, static_cast<int>(fontSize * 4.0));

    // 像素间距 => 时间间距（μs；ceil 保证足够间距）
    const qint64 stepByPixels = qMax<qint64>(
        1, static_cast<qint64>(std::ceil(static_cast<double>(minPixelSpacing) / m_timescale)));

    // "漂亮"时间档位（μs；static 只构造一次——本函数每帧被标尺与网格
    // 各调一次， QVector 局部变量曾是每帧两次堆分配）
    static const qint64 niceSteps[] = {
        10'000, 25'000, 50'000,                          // 10/25/50ms（U3：深放大档位）
        100'000, 250'000, 500'000,                       // 100ms/250ms/500ms
        1'000'000, 2'000'000, 5'000'000, 10'000'000,     // 1/2/5/10s
        30'000'000, 60'000'000,                          // 30s/60s
        300'000'000LL, 600'000'000LL,                    // 5min/10min
        1'800'000'000LL, 3'600'000'000LL                 // 30min/1h
    };
    constexpr int kNiceStepCount = sizeof(niceSteps) / sizeof(niceSteps[0]);

    // 选择第一个 >= stepByPixels 的漂亮档位；若都小于，则用最后一个
    qint64 step = niceSteps[kNiceStepCount - 1];
    for (int i = 0; i < kNiceStepCount; ++i) {
        if (niceSteps[i] >= stepByPixels) { step = niceSteps[i]; break; }
    }

    return step;
}

void BaseTimelineView::drawTimeRuler(QPainter* painter, const QRect& rect)
{
    /**
     * 函数说明：
     * - 绘制标尺背景，计算起止范围与时间步长（μs 漂亮档位）。
     * - 使用当前 timescale 与步长确定像素间距，避免文本重叠。
     */

    // 绘制标尺背景
    painter->setPen(m_timelineRulerColor);
    painter->setBrush(QBrush(m_timelineBgColor));
    painter->drawRect(-m_scrollOffset.x(), 0,
                     rect.width() + m_scrollOffset.x(),
                     contentTop());

    // 计算刻度间隔（μs 漂亮档位；2026-11-10 微秒原生）
    const qint64 stepUs = calculateTimeStepUs();

    // 计算起始/结束位置（以时刻为单位，规整到步长边界）
    qint64 startUs = pointToFrame(m_scrollOffset.x());
    startUs -= (startUs % stepUs);

    const int startX = frameToPoint(startUs);
    const int endX   = rect.width() + m_scrollOffset.x();

    // 绘制时间标记和文本（根据像素间距决定是否绘制文本）
    drawTimeMarkers(painter, startUs, startX, endX, stepUs);
}

void BaseTimelineView::drawTimeMarkers(QPainter* painter, qint64 startUs, int startMarker, int endMarker, qint64 stepUs)
{
    /**
     * 函数说明：
     * - 按时间步长（μs）绘制刻度线。
     * - 当像素间距不足最小间距时，仅绘制刻度线不绘制文本，避免重叠。
     * - 2026-11-10 性能/精度：整数 tick 序号步进（每 tick 一次乘法，
     *   消浮点累加的累积误差）；刻度 μs 值 = startUs + k*stepUs（精确
     *   均匀，不再经像素回算产生 ±1μs 抖动）。
     */

    // 计算每个刻度的像素间距（实例级 m_timescale，避免跨窗口共享状态）
    const int tickPixelStep = static_cast<int>(std::round(m_timescale * static_cast<double>(stepUs)));

    // 依据字体大小设定最小文本间距（hh:mm:ss:mmm 共 11 字符，需更宽空间）
    const int minLabelPixelSpacing = qMax(80, static_cast<int>(fontSize * 4.0));
    const bool canDrawText = tickPixelStep >= minLabelPixelSpacing;

    /**
     * 标尺带布局重排（2026-11-11 用户反馈：刻度线与时间码文字叠在一起）。
     * 旧布局 tick 线画满 textoffset(13px) 高的"文本带"——fontSize(25) 的
     * 数字字身约 18px 高，几乎占满 rulerHeight(30)，tick 线从字身顶一直
     * 穿到标尺底。重排为上下两段互不侵入：
     *   tick 短线贴标尺底 kRulerTickHeight(8px)；文本 baseline 在 tick 顶
     *   上方 3px（数字无下伸部，baseline 即字身底）→ 字身与线完全分离。
     */
    constexpr int kRulerTickHeight = 8;

    const double pxStep = m_timescale * static_cast<double>(stepUs);
    for (qint64 k = 0;; ++k) {
        const int xInt = static_cast<int>(static_cast<double>(startMarker) + static_cast<double>(k) * pxStep);
        if (xInt >= endMarker)
            break;
        const qint64 tickUs = startUs + k * stepUs;

        // 画刻度线（贴标尺底短线，不再伸进文字带）
        painter->drawLine(xInt - m_scrollOffset.x(),
                          contentTop() - kRulerTickHeight,
                          xInt - m_scrollOffset.x(),
                          contentTop());

        if (canDrawText) {
           // 文本格式化为 hh:mm:ss:mmm（时:分:秒:毫秒，段不足补零显示
           // ——用户需求：显示真实时间而非帧号）。μs→秒→时间码，复用
           // TimeCodeDefines 既有换算（多制式时间码，WATCHOUT 语义）。
           // P3（timeline-draw-zoom-review）：换算+fontMetrics 测量走
           // 缓存——同一 (tickUs, 制式) 的文本恒定，滚动帧零换算零测量；
           // 制式切换整体失效，超容量整体清空（标签有限重建便宜）。
            const int tcTypeInt = static_cast<int>(Model->getTimeCodeType());
            if (m_tickLabelTcType != tcTypeInt) {
                m_tickLabelCache.clear();
                m_tickLabelTcType = tcTypeInt;
            }
            QString text;
            int textW = 0;
            const auto found = m_tickLabelCache.find(tickUs);
            if (found != m_tickLabelCache.end()) {
                text = found->second.text;
                textW = found->second.width;
            } else {
                if (m_tickLabelCache.size() >= 1024)
                    m_tickLabelCache.clear();
                const auto tc = time_to_timecode_frame(
                    static_cast<double>(tickUs) / 1'000'000.0, Model->getTimeCodeType());
                text = timecode_frame_to_string(tc, Model->getTimeCodeType());
                textW = painter->fontMetrics().boundingRect(text).width();
                m_tickLabelCache.emplace(tickUs, TickLabel{text, textW});
            }

            // 将文本居中到刻度线：baseline = tick 顶上方 3px
            painter->drawText(xInt - textW / 2 - m_scrollOffset.x(),
                              contentTop() - kRulerTickHeight - 3, text);
        }
    }
}

void BaseTimelineView::drawPlayhead(QPainter* painter)
{
    // 绘制播放头三角形
    QPoint kite[5] = {
        QPoint(0,0),
        QPoint(-playheadwidth, -playheadCornerHeight),
        QPoint(-playheadwidth, -playheadheight),
        QPoint(playheadwidth, -playheadheight),
        QPoint(playheadwidth, -playheadCornerHeight)
    };

    // 播放头 x 与 itemRect/visualRect 同源（滚动条实时值）——m_scrollOffset
    // 是 scrollContentsBy 平移的缓存，极端时序下与滚动条值有偏差，会造成
    // 播放头竖线与 clip 内嵌 panel 游标线错位（2026-11-06 对齐修复）
    int playheadPos = frameToPoint(getModel()->getPlayheadPos())
                      - horizontalScrollBar()->value();
    
    // 移动播放头到正确位置
    for (QPoint &p : kite) {
        p.setX(p.x() + playheadPos);
        p.setY(p.y() + contentTop());
    }

    // 设置画笔和画刷颜色并绘制
    painter->setPen(m_timelinePlayheadColor);
    painter->setBrush(m_timelinePlayheadColor);
    painter->drawConvexPolygon(kite, 5);
    painter->drawLine(QPoint(playheadPos, contentTop()),
                     QPoint(playheadPos, viewport()->height()));
}

/**
 * 设置时间线背景色
 * @param QColor color 背景色
 */
void BaseTimelineView::setTimelineBgColor(const QColor& color) {
    m_timelineBgColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置时间线分隔线颜色
 * @param QColor color 分隔线颜色
 */
void BaseTimelineView::setTimelineSeparatorColor(const QColor& color) {
    m_timelineSeparatorColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置标尺颜色
 * @param QColor color 标尺颜色
 */
void BaseTimelineView::setTimelineRulerColor(const QColor& color) {
    m_timelineRulerColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置播放头颜色
 * @param QColor color 播放头颜色
 */
void BaseTimelineView::setTimelinePlayheadColor(const QColor& color) {
    m_timelinePlayheadColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置片段填充颜色
 * @param QColor color 片段填充颜色
 */
void BaseTimelineView::setClipFillColor(const QColor& color) {
    m_clipFillColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置片段边框颜色
 * @param QColor color 片段边框颜色
 */
void BaseTimelineView::setClipBorderColor(const QColor& color) {
    m_clipBorderColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置轨道选中颜色
 * @param QColor color 轨道选中颜色
 */
void BaseTimelineView::setTrackSelectedColor(const QColor& color) {
    m_trackSelectedColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 样式变化事件处理，确保 QSS 生效后刷新颜色
 * @param QEvent* e 事件
 */
void BaseTimelineView::changeEvent(QEvent* e) {
    QAbstractItemView::changeEvent(e);
    if (e->type() == QEvent::StyleChange || e->type() == QEvent::PaletteChange) {
        applyStyleFromProperties();
        viewport()->update();
    }
}
/**
 * 从属性应用颜色到全局样式（兼容旧绘制逻辑和其他文件使用的全局色）
 */
void BaseTimelineView::applyStyleFromProperties() {
    // 更新旧代码使用的全局颜色（TimeLineStyle.h）
    bgColour            = m_timelineBgColor;
    seperatorColour     = m_timelineSeparatorColor;
    rulerColour         = m_timelineRulerColor;
    playheadColour      = m_timelinePlayheadColor;
    ClipColor           = m_clipFillColor;
    ClipBorderColour    = m_clipBorderColor;
    trackSelectedColour = m_trackSelectedColor;
    m_staticCacheDirty = true;
}

void BaseTimelineView::visibleTrackRange(int& firstRow, int& lastRow) const
{
    firstRow = 0;
    lastRow = -1;
    if (!model())
        return;
    const int count = model()->rowCount();
    if (count <= 0)
        return;

    // 2026-11-06 轨道展开：行高 per-row（展开行更高），前缀累加求可视行区间，
    // 禁止再用 bottomContent / trackHeight 均分行（展开行会导致错位）。
    const int scrollY = verticalScrollBar()->value();
    const int top = contentTop();
    const int bottomContent = scrollY + viewport()->height() - top - 1;
    if (bottomContent < 0)
        return;

    int y = 0;
    firstRow = -1;
    lastRow = -1;
    auto* tlModel = static_cast<BaseTimeLineModel*>(model());
    for (int row = 0; row < count; ++row) {
        const int h = tlModel->trackRowHeight(row);
        if (y + h > scrollY - top && firstRow < 0)
            firstRow = row; // 行底越过视口顶 = 首个可见行
        if (y > bottomContent)
            break;          // 行顶已越过视口底 = 后续全部不可见
        lastRow = row;
        y += h;
    }
    if (firstRow < 0)
        firstRow = 0;
    if (firstRow > lastRow)
        lastRow = -1;
}

void BaseTimelineView::drawTracks(QPainter* painter)
{
    if (!model() || !painter)
        return;

    int firstRow = 0;
    int lastRow = -1;
    visibleTrackRange(firstRow, lastRow);
    if (lastRow < firstRow)
        return;

    const int scrollX = this->scrollX(); // P7：统一访问器（滚动条真值）
    const int scrollY = this->scrollY();
    const int viewRight = scrollX + viewport()->width();
    // 2026-11-11 极限缩放加固：padFrames/过滤窗口 μs 域 qint64（缩到最小时
    // pointToFrame(2000px) ≈ 1.6e10 μs，int 截断曾使过滤窗口错乱）
    const qint64 padFrames = qMax<qint64>(1, pointToFrame(4));
    const qint64 frameLo = pointToFrame(qMax(0, scrollX)) - padFrames;
    const qint64 frameHi = pointToFrame(viewRight) + padFrames;
    const bool clipped = painter->hasClipping();
    const QRect clipBounds = clipped ? painter->clipBoundingRect().toAlignedRect() : QRect();
    const int top = contentTop();
    const AbstractClipModel* selectedClip = nullptr;
    if (selectionModel()) {
        const QModelIndex current = selectionModel()->currentIndex();
        if (current.parent().isValid())
            selectedClip = static_cast<AbstractClipModel*>(current.internalPointer());
    }
    QList<int> clipRows;

    for (int i = firstRow; i <= lastRow; ++i) {
        // 2026-11-06 轨道展开：行顶 y 走前缀和（展开行使下方行下移）；clip
        // 本体高度仍恒 trackHeight（展开附加区归 clip panel，不参与片段绘制）
        const int trackTop = static_cast<BaseTimeLineModel*>(model())->trackRowY(i) + top - scrollY;
        if (clipped && (trackTop + trackHeight <= clipBounds.top() || trackTop >= clipBounds.bottom()))
            continue;

        const QModelIndex trackIndex = model()->index(i, 0);
        auto* track = static_cast<TrackData*>(trackIndex.internalPointer());
        if (!track)
            continue;
        clipRowsInFrameRange(track, frameLo, frameHi, clipRows);
        for (int clipRow : clipRows) {
            if (clipRow < 0 || clipRow >= track->clips.size())
                continue;
            AbstractClipModel* clip = track->clips.at(clipRow);
            if (!clip || (m_interactiveDrag && clip == m_dragClip))
                continue;

            const int startX = frameToPoint(clip->start());
            const int clipWidth = frameToPoint(clip->end() - clip->start() + 1);
            if (startX + clipWidth <= scrollX || startX >= viewRight)
                continue;
            const int paintWidth = qMax(clipWidth, 1);
            if (clipped && !QRect(startX - scrollX, trackTop, paintWidth, trackHeight).intersects(clipBounds))
                continue;

            // 与 clipContentBounds 同一套几何，避免再走 visualRect / parent()。
            m_clipContentRects[clip] = QRect(startX, trackTop + scrollY, paintWidth, trackHeight).adjusted(-4, -4, 44, 4);
            const QRect clipRect(startX - scrollX, trackTop + clipoffset, clipWidth, trackHeight - clipoffset * 2);
            clip->paint(painter, clipRect, clip == selectedClip);
        }
    }
}

void BaseTimelineView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QAbstractItemView::selectionChanged(selected, deselected);

    m_staticCacheDirty = true;
    viewport()->update();
}

void BaseTimelineView::wheelEvent(QWheelEvent *event){
    /**
     * 函数说明：支持 Ctrl+滚轮进行缩放；否则进行水平滚动
     * - 缩放使用乘法步进，并钳制到 [minNormalized, 1.0]，避免无限缩小
     * - 缩放锚点 = 鼠标位置下的时刻（setScaleAt）——鼠标指哪放大哪，
     *   旧实现锚点恒为播放头且滚动条 range 未刷新，导致缩放焦点漂移
     */
    if (event->modifiers() & Qt::ControlModifier) {
        const QPoint pos = event->position().toPoint();
        const double delta = event->angleDelta().y() / 120.0; // 标准滚轮步长
        const double scaleChange = (delta > 0 ? 1.1 : 0.9);   // 每次滚动的缩放倍率

        // P5（timeline-draw-zoom-review）：60ms 窗口合并连续滚轮缩放——
        // 首事件立即应用并开窗（体感零延迟），窗口内后续事件只累计倍率，
        // 到期一次 setScaleAtPx。大工程全页重建从每事件一次降为每 60ms
        // 一次；合成事件不进事件循环 → 定时器不触发，首事件直通（测试零干扰）。
        if (m_zoomCoalesceTimer && m_zoomCoalesceTimer->isActive()) {
            m_pendingZoomRatio *= scaleChange;
            m_pendingZoomAnchorPx = pos.x();
        } else {
            // U6：像素锚点直传（消 μs 中间取整的连续缩放蠕动）
            setScaleAtPx(qBound(0.0, currentScale * scaleChange, 1.0), pos.x());
            m_pendingZoomRatio = 1.0;
            m_pendingZoomAnchorPx = pos.x();
            if (m_zoomCoalesceTimer)
                m_zoomCoalesceTimer->start();
        }
    } else {
        /**
         * @brief 非 Ctrl 情况下滚轮用于水平滚动
         * @details
         * 以滚动条为真值来源：通过设置 horizontalScrollBar()->setValue 驱动滚动，
         * 避免只改 m_scrollOffset 造成“视觉滚动了但滚动条没动/命中测试不一致”。
         * 步长自适应（2026-11-10）：固定 60px/格在深放大时（range 数万 px）
         * 体感"滚不动/拉不回来"——按内容宽/40 动态加速（约 40 格滚完全程）。
         * U2（timeline-draw-zoom-review）：触控板 pixelDelta 平滑滚动分支。
         * 高精度触控板事件携带像素级 pixelDelta，用 angleDelta×步长的离散
         * 换算会出现"一次手势跳一大格"的顿挫。pixelDelta 直接驱动滚动条
         * （与 QAbstractScrollArea 内建语义一致：value -= pixelDelta，内容
         * 跟手）；x 分量优先，纯纵向滑动映射到水平轴（时间轴主轴）。
         * 滚轮鼠标（pixelDelta 为空）保留原自适应步长路径。
         */
        QScrollBar* hb = horizontalScrollBar();
        const QPoint pd = event->pixelDelta();
        if (!pd.isNull()) {
            const int delta = pd.x() != 0 ? pd.x() : pd.y();
            const int newX = qBound(hb->minimum(), hb->value() - delta, hb->maximum());
            if (newX != hb->value()) {
                // 滚动条变化会进入 scrollContentsBy，在那里平移缓存。
                hb->setValue(newX);
            }
        } else {
            const int contentW = qMax(1, getTrackWdith() - viewport()->width());
            const int step = qMax(60, contentW / 40);
            const int deltaX = -event->angleDelta().y() * step / 120;

            const int oldX = hb->value();
            const int newX = qBound(hb->minimum(), oldX + deltaX, hb->maximum());

            if (newX != oldX) {
                // 滚动条变化会进入 scrollContentsBy，在那里平移缓存。
                hb->setValue(newX);
            }
        }
    }
    event->accept();
}

void BaseTimelineView::showClipProperty(const QModelIndex& index)
{
    //判断索引不为为片段时
    if(!index.isValid() || !index.parent().isValid()){
        return;
    }
    //获取片段
    AbstractClipModel* clip = static_cast<AbstractClipModel*>(index.internalPointer());
    if (!clip) return;
        //获取属性窗口
        clip->showPropertyWidget();
}

BaseTimeLineModel* BaseTimelineView::getModel() const
{
    return Model;
}

/**
 * @brief 检查文件是否可接受
 * @param QString filePath 文件路径
 * @return bool 是否可接受
 */
QString BaseTimelineView::isMimeAcceptable(const QMimeData *Mime) const
{
    // 获取拖拽的文件路径

    if (Mime->hasUrls())
    {
        QString filePath = Mime->urls().first().toLocalFile();
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix().toLower();
        // 根据后缀判断文件类型
        if(VideoTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Video";
        }
        else if(AudioTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Audio";
        }
        else if(ImageTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Image";
        }
        else if(ControlTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Control";
        }
    }
    // 【2026-11-10 剪裁】上游 OSC 地址拖放分支（application/x-osc-address →
    // Control）已随 OSC 体系退役——发送端 startDrag 删除后无人产生此 mime。
    return "";
}
