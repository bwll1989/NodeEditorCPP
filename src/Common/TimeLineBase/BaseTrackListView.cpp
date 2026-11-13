#include "BaseTrackListView.h"
#include "Common/AppConfig/TimeCodeDefines.h"
#include <QComboBox>
#include <QDrag>
#include <QMimeData>
#include <QMenu>
#include <QSet>
#include <QSignalBlocker>
BaseTracklistView::BaseTracklistView(BaseTimeLineModel *viemModel, QWidget *parent): Model(viemModel), QAbstractItemView{parent}{
    /**
     * @brief 构造轨道列表视图
     * @details
     * 关键点：本视图不再“自己 new selectionModel”。
     * - 以前 TrackListView 和 TimelineView 各自持有 selectionModel，会产生两套选择状态，导致逻辑越来越乱。
     * - 重构后 selectionModel 由外部容器（BaseTimelineWidget）统一创建并注入，从而保证选择状态唯一。
     */
    setObjectName("TrackListView");
    setModel(Model);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    // 行头属性读数跟随播放头（2026-11-06 用户定案：下拉框下方显示当前值
    // + 取值范围）——播放头不是模型树节点（dataChanged 表达不了），走
    // S_playheadMoved 专用轻量信号刷视口（构造内自连，宿主零接线）。
    if (Model)
        connect(Model, &BaseTimeLineModel::S_playheadMoved,
                this, [this] { viewport()->update(); });
    // selectionModel 由外部注入（BaseTimelineWidget 中的共享 selectionModel）
    // 如果有人单独使用 BaseTracklistView（不通过 BaseTimelineWidget），则会在 showEvent 中补一个兜底。

    horizontalScrollBar()->setSingleStep(10);
    horizontalScrollBar()->setPageStep(100);
    verticalScrollBar()->setSingleStep(trackHeight);
    verticalScrollBar()->setPageStep(trackHeight );
    viewport()->setMinimumHeight(trackHeight + rulerHeight+toolbarHeight);
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(true);

    // Create delete track action
    m_deleteTrackAction = new QAction("Delete track", this);
    m_deleteTrackAction->setIcon(QIcon(":/icons/icons/trash.png"));
    m_deleteTrackAction->setShortcut(QKeySequence(Qt::Key_Delete));
    QObject::connect(m_deleteTrackAction, &QAction::triggered, this, &BaseTracklistView::onDeleteTrack);
    setItemDelegate(new BaseTrackDelegate(this));
    setEditTriggers(QAbstractItemView::DoubleClicked);
    setMouseTracking(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded ) ;
    setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded) ;
    installEventFilter(this);
    
    
}

void BaseTracklistView::setTrackColor(const QColor &color)
{
    m_trackColor = color;
    viewport()->update();
}

void BaseTracklistView::setTrackSelectedColor(const QColor &color)
{
    m_trackSelectedColor = color;
    viewport()->update();
}

void BaseTracklistView::setTrackHoverColor(const QColor &color)
{
    m_trackHoverColor = color;
    viewport()->update();
}

void BaseTracklistView::setTrackTitleColor(const QColor &color)
{
    m_trackTitleColor = color;
    viewport()->update();
}

void BaseTracklistView::setTimelinePlayheadColor(const QColor& color)
{
    m_timelinePlayheadColor = color;
    viewport()->update();
}

/**
 * @brief 样式变化事件：QSS qproperty- 生效/主题切换后重绘视口
 */
void BaseTracklistView::changeEvent(QEvent *event)
{
    QAbstractItemView::changeEvent(event);
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange) {
        viewport()->update();
    }
}

/**
 * @brief 删除轨道
 */
void BaseTracklistView::onDeleteTrack() {
    /**
     * @brief 删除当前选中的轨道
     * @details
     * persistent editor 的创建/关闭统一收敛到 onUpdateViewport/ensureVisiblePersistentEditors。
     */
    if (!Model || !selectionModel()) {
        return;
    }

    const QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        return;
    }

    const int deleteRow = selected.first().row();
    if (deleteRow < 0 || deleteRow >= Model->rowCount()) {
        return;
    }

    const QModelIndex deleteIndex = Model->index(deleteRow, 0);
    if (indexWidget(deleteIndex)) {
        closePersistentEditor(deleteIndex);
    }

    // 从缓存中移除该 editor（若启用了可视范围 editor 缓存）
    for (int i = m_openPersistentEditors.size() - 1; i >= 0; --i) {
        if (!m_openPersistentEditors[i].isValid() || m_openPersistentEditors[i] == deleteIndex) {
            m_openPersistentEditors.remove(i);
        }
    }

    Model->onDeleteTrack(deleteRow);
    selectionModel()->clearSelection();
    m_hoverIndex = QModelIndex();
    onUpdateViewport();
}
/**
 * @brief 上下文菜单事件
 */
void BaseTracklistView::contextMenuEvent(QContextMenuEvent* event) {
        QMenu contextMenu(this);
        //qDebug()<<"contextMenuEvent";
        // Add track creation submenu
        QMenu* addTrackMenu = contextMenu.addMenu("Add Track ");
        addTrackMenu->setIcon(QIcon(":/icons/icons/add.png"));
        // Get available track types from BasePluginLoader
        QStringList availableTypes = Model->getPluginLoader()->getAvailableTypes();
        for (const QString& type : availableTypes) {
            QAction* addTrackAction = addTrackMenu->addAction("Add " + type + " Track");
            connect(addTrackAction, &QAction::triggered, [this, type]() {
                Model->onAddTrack(type);
            });
        }

        // Add delete track option if a track is selected
        QModelIndex index = indexAt(event->pos());
        if (index.isValid()) {
            contextMenu.addAction(m_deleteTrackAction);
        }

        contextMenu.exec(event->globalPos());
    }
    
/**
 * @brief 鼠标按下事件
 */
void BaseTracklistView::mousePressEvent(QMouseEvent *event) {
    /**
     * @brief 轨道列表鼠标按下
     * @details
     * - 这里只处理“轨道行”的选择。
     * - 选择状态由共享 selectionModel 承载；若 selectionModel 尚未注入，则直接走基类逻辑，避免空指针崩溃。
     */
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
        // ---- 展开按钮命中（2026-11-06 轨道展开）：先于行选择判定 ----
        // 按钮几何与 paintEvent 绘制共用 trackExpandButtonRect（同源纪律）。
        // 命中即切换 TrackExpandedRole 并消费事件（不改变轨道选中态——
        // 避免误清当前片段选择）。
        if (model() && selectionModel()) {
            const QModelIndex hitTrack = indexAt(event->pos());
            // 展开按钮隐藏时（2026-11-09 合并关键帧模式）不拦截——点击
            // 直接走行选择
            if (hitTrack.isValid() && m_expandButtonVisible &&
                trackExpandButtonRect(visualRect(hitTrack)).contains(event->pos())) {
                Model->setTrackExpanded(hitTrack.row(),
                                        !Model->isTrackExpanded(hitTrack.row()));
                event->accept();
                return; // 不进入行选择/拖拽
            }
        }
        if (selectionModel()) {
            QModelIndex item = indexAt(event->pos());
            if (item.isValid()) {
                selectionModel()->select(item, QItemSelectionModel::ClearAndSelect);
                selectionModel()->setCurrentIndex(item, QItemSelectionModel::Current);
            }
        }
        viewport()->update();
    }

    QAbstractItemView::mousePressEvent(event);
}

/**
 * @brief 轨道行头展开按钮矩形（paintEvent 绘制与 mousePress 命中共用）
 * @details 2026-11-06 用户定案：名称组【对齐到 clip 本体行】不再全行居中
 * ——clip 本体恒占行内顶部 52px，按钮在其内垂直居中（中心 26）。普通
 * 52px 行 52/2-9=17 同值，两种行态视觉一致。
 */
QRect BaseTracklistView::trackExpandButtonRect(const QRect& rowRect) const
{
    return QRect(rowRect.left() + 6, rowRect.top() + 17, 18, 18);
}

/**
 * @brief 获取某一点的轨道索引
 */
QModelIndex BaseTracklistView::indexAt(const QPoint &point) const {
    const int header = rulerHeight + toolbarHeight;
    if (!model() || point.y() < header)
        return {};

    const int contentY = point.y() + verticalScrollBar()->value() - header;
    if (contentY < 0)
        return {};
    // 2026-11-06：per-row 行高反查（展开行更高，禁均分）
    const int row = static_cast<BaseTimeLineModel*>(model())->trackRowAt(contentY);
    if (row < 0 || row >= model()->rowCount())
        return {};

    const QModelIndex index = model()->index(row, 0);
    if (visualRect(index).contains(point))
        return index;
    return {};
}

BaseTimeLineModel* BaseTracklistView::getModel()
{
    return Model;
}
/**
 * @brief 选择改变
 */
void BaseTracklistView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QAbstractItemView::selectionChanged(selected, deselected);

    viewport()->update();
}

/**
 * @brief 鼠标移动事件
 */
void BaseTracklistView::mouseMoveEvent(QMouseEvent *event) {
    // 如果不是左键拖动，只更新悬停状态
    if (!(event->buttons() & Qt::LeftButton)) {
        const QModelIndex newHover = indexAt(event->pos());
        if (newHover != m_hoverIndex) {
            m_hoverIndex = newHover;
            viewport()->update();
        }
        return;
    }

    // 检查是否达到拖动启动距离，避免轻微抖动就触发拖拽
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    // 获取拖动起始位置的轨道索引
    const QModelIndex index = indexAt(m_dragStartPosition);
    if (!index.isValid()) {
        return;
    }

    // 创建拖动对象
    auto* drag = new QDrag(this);
    auto* mimeData = new QMimeData;

    // 存储源轨道索引
    mimeData->setData("application/x-track-index", QByteArray::number(index.row()));
    drag->setMimeData(mimeData);

    // 创建预览图
    const QRect itemRect = visualRect(index);
    const int previewW = qMax(1, viewport()->width());
    const int previewH = qMax(1, itemRect.height());
    QPixmap pixmap(previewW, previewH);

    // 拖拽预览底色使用视图成员颜色（QSS 可配置），不再直读全局变量
    QColor fillColor = m_trackSelectedColor;
    fillColor.setAlphaF(0.8);
    pixmap.fill(fillColor);

    // 将热点设置为鼠标相对于轨道的位置
    const QPoint relativePos = m_dragStartPosition - itemRect.topLeft();
    drag->setPixmap(pixmap);
    drag->setHotSpot(relativePos);

    // 开始拖动操作
    drag->exec(Qt::MoveAction);

    // 防止一次按下过程中重复触发拖拽
    m_dragStartPosition = QPoint();
}

/**
 * @brief 离开事件
 */
void BaseTracklistView::leaveEvent(QEvent *event) {
    // selectionModel()->clear();
    if (m_hoverIndex.isValid()) {
        m_hoverIndex = QModelIndex();
        viewport()->update();
    }
    QAbstractItemView::leaveEvent(event);
}
/**
 * @brief 大小改变事件
 */
void BaseTracklistView::resizeEvent(QResizeEvent *event) {
    onUpdateViewport();
    updateScrollBars();
    QAbstractItemView::resizeEvent(event);
}

/**
 * @brief 显示事件
 */
void BaseTracklistView::showEvent(QShowEvent *event) {
    /**
     * @brief 显示事件
     * @details
     * 兜底逻辑：
     * - 如果外部没有注入共享 selectionModel，则创建一个本地 selectionModel。
     * - 如果本视图未被 Controller 托管（独立使用），则在此处按需绑定“模型变化 -> 刷新”。
     */
    if (!selectionModel() && model()) {
        setSelectionModel(new QItemSelectionModel(model(), this));
    }

    // 独立使用场景：没有 Controller 时，TrackList 需要自己监听模型变化来刷新
    if (!property("qtimeline_managed").toBool() && !property("qtimeline_model_bound").toBool() && Model) {
        connect(Model, &QAbstractItemModel::modelReset,   this, &BaseTracklistView::onUpdateViewport);
        connect(Model, &QAbstractItemModel::rowsInserted, this, &BaseTracklistView::onUpdateViewport);
        connect(Model, &QAbstractItemModel::rowsRemoved,  this, &BaseTracklistView::onUpdateViewport);
        connect(Model, &QAbstractItemModel::rowsMoved,    this, &BaseTracklistView::onUpdateViewport);

        connect(Model, &QAbstractItemModel::dataChanged,
                this, [this](const QModelIndex& topLeft, const QModelIndex&, const QList<int>&) {
                    // Clip 层数据变化不影响 TrackList；Track 层变化轻量重绘即可
                    if (!topLeft.parent().isValid()) {
                        viewport()->update();
                    }
                });


        connect(Model, &BaseTimeLineModel::S_playheadMoved, this, [this](qint64) {
            updatePlayheadHeader();
        });

        setProperty("qtimeline_model_bound", true);
    }

    updateScrollBars();
    QAbstractItemView::showEvent(event);
}

/**
 * @brief 滚动事件
 */
void BaseTracklistView::scrollContentsBy(int dx, int dy) {
    /**
     * @brief QAbstractItemView 内部滚动回调
     * @details
     * 统一以滚动条 value 为唯一真值来源：
     * - 这里不再维护 m_scrollOffset。
     * - 通过 visualRect() 读取 scrollBar value 来计算偏移。
     */
    QAbstractItemView::scrollContentsBy(dx, dy);
    updateEditorGeometries();
    onUpdateViewport();
}

/**
 * @brief 滚动事件
 */


/**
 * @brief 更新视图
 */
void BaseTracklistView::onUpdateViewport(){
    /**
     * @brief 更新视图
     * @details
     * persistent editor（轨道行内的编辑控件）由本函数统一管理：
     * - 仅在可视范围（含少量缓冲行）内按需 openPersistentEditor。
     * - 对离开可视范围的行按需 closePersistentEditor，避免滚动过程中 editor 只增不减。
     */
    updateScrollBars();
    ensureVisiblePersistentEditors();
    viewport()->update();
}

void BaseTracklistView::ensureVisiblePersistentEditors()
{
    // 轨道名平时只画文字，不再为可见行打开常驻输入框。
    // 这里只关掉以前留下的 persistent editor。
    for (int i = m_openPersistentEditors.size() - 1; i >= 0; --i) {
        const QPersistentModelIndex& p = m_openPersistentEditors[i];
        if (p.isValid() && indexWidget(p))
            closePersistentEditor(p);
    }
    m_openPersistentEditors.clear();
}

/**
 * @brief 更新滚动条
 */
void BaseTracklistView::updatePlayheadHeader()
{
    const int header = rulerHeight + toolbarHeight;
    viewport()->update(QRect(0, 0, viewport()->width(), header));
}

void BaseTracklistView::updateScrollBars(){
    if(!model())
        return;
    int max =0;
    // 2026-11-06 轨道展开：总高 = Σ 各行高（展开行 = trackHeight + 附加高度），
    // 与 TimelineView 同口径（BaseTimeLineModel::totalTracksHeight），保证滚动同步
    verticalScrollBar()->setRange(0, static_cast<BaseTimeLineModel*>(model())->totalTracksHeight()
                                      + rulerHeight+toolbarHeight - viewport()->height());
}

/**
 * @brief 获取视图矩形
 */
QRect BaseTracklistView::visualRect(const QModelIndex &index) const{
    /**
     * @brief 计算索引的可视矩形
     * @details
     * 以滚动条 value 为偏移来源，避免 m_scrollOffset 与滚动条不同步。
     */
    const QPoint offset(horizontalScrollBar()->value(), verticalScrollBar()->value());
    return itemRect(index).translated(-offset);
}
/**
 * @brief 获取项目矩形
 */
QRect BaseTracklistView::itemRect(const QModelIndex &index) const{
    if(!index.isValid() || !model())
        return QRect();
    // 2026-11-06 轨道展开：行顶 y = 前面各行高之和；展开行返回扩展高度
    auto* tlModel = static_cast<BaseTimeLineModel*>(model());
    return QRect(0, tlModel->trackRowY(index.row()) + rulerHeight+toolbarHeight,
                 viewport()->width(), tlModel->trackRowHeight(index.row()));
};
/**
 * @brief 更新编辑几何
 */
void BaseTracklistView::updateEditorGeometries() {
    QAbstractItemView::updateEditorGeometries();

    for (int i = 0; i < model()->rowCount(); ++i){
        QModelIndex trackIndex = model()->index(i,0);

        QWidget *editor = indexWidget(trackIndex);
        if(!editor){
            continue;
        }

        QRect rect  = editor->rect();
        QPoint topInView = editor->mapToParent(rect.topLeft());
        if(topInView.y()<rulerHeight+toolbarHeight){
            int offset = rulerHeight+toolbarHeight-topInView.y();
            editor->setMask(QRegion(0,offset,editor->width(),rulerHeight+toolbarHeight));
        }

    }
}
/**
 * @brief 事件过滤
 */
bool BaseTracklistView::eventFilter(QObject *watched, QEvent *event)
{

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (watched == this) {
            // 处理 view1 的 keyPressEvent
            // 注意：可以添加其他逻辑来处理特定的键盘输入
            
            if(keyEvent->key()==Qt::Key_Delete){
                onDeleteTrack();
            }
            return true;  // 表示事件已处理，不再向其他控件传递
        }
    }
    return QObject::eventFilter(watched, event);
}
/**
 * @brief 设置选择
 */
void BaseTracklistView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) {
    if (!model() || !selectionModel())
    return;

    // 将视图坐标转换为模型索引围
    QModelIndex topLeft = indexAt(rect.topLeft());
    QModelIndex bottomRight = indexAt(rect.bottomRight());

    if (!topLeft.isValid() || !bottomRight.isValid())
        return;

    // 创建选择范围
    QItemSelection selection(topLeft, bottomRight);

    // 更新选择模型
    selectionModel()->select(selection, command);
}
/**
 * @brief 绘制事件
 */
void BaseTracklistView::paintEvent(QPaintEvent *event) {
    /**
     * @brief 绘制轨道列表
     * @details
     * 这里做一个关键的“绑定简化”：
     * - 当 TimelineView 选中 clip 时，selectionModel 的 currentIndex 是“clipIndex（parent 有效）”。
     * - TrackListView 的行索引是“trackIndex（parent 无效）”，两者不是同一个 index。
     * - 旧实现只用 isSelected(trackIndex) 判断，导致“选中 clip 时左侧轨道不高亮”。
     * - 新实现：根据 currentIndex 推导出“当前轨道行号”，让视觉反馈一致，但不强行改变 selectionModel 的内容。
     */
    QPainter painter(viewport());
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setBrush(QBrush(m_trackBgColor));

    const QItemSelectionModel* sel = selectionModel();
    const QModelIndex current = sel ? sel->currentIndex() : QModelIndex();
    int currentTrackRow = -1;
    if (current.isValid()) {
        currentTrackRow = current.parent().isValid() ? current.parent().row() : current.row();
    }

    const int header = rulerHeight + toolbarHeight;
    const bool drawRows = event->rect().bottom() >= header;
    if (drawRows) {
    const int scrollY = verticalScrollBar()->value();
    const int bottomContent = scrollY + viewport()->height() - header - 1;
    const int count = model() ? model()->rowCount() : 0;
    // 2026-11-06 轨道展开：可视行区间用前缀累加求（展开行高 ≠ 常量，禁均分）。
    // 可见条件：行底 y+h > 视口顶（scrollY-header）且行顶 y ≤ 视口底（bottomContent）
    int firstRow = 0;
    int lastRow = -1;
    if (count > 0 && bottomContent >= 0) {
        auto* tlModel = static_cast<BaseTimeLineModel*>(model());
        int y = 0;
        firstRow = -1;
        for (int row = 0; row < count; ++row) {
            const int h = tlModel->trackRowHeight(row);
            if (y > bottomContent)
                break; // 行顶已越过视口底 = 后续全部不可见
            if (y + h > scrollY - header && firstRow < 0)
                firstRow = row; // 行底越过视口顶 = 首个可见行
            lastRow = row;
            y += h;
        }
        if (firstRow < 0)
            firstRow = 0;
        if (firstRow > lastRow)
            lastRow = -1;
    }

    for (int i = firstRow; i <= lastRow; ++i){
        QModelIndex trackIndex = model()->index(i,0);

        const bool isCurrentTrack = (i == currentTrackRow);
        const bool isTrackSelected = sel ? sel->isSelected(trackIndex) : false;

        if (isCurrentTrack || isTrackSelected) {
            painter.setBrush(QBrush(m_trackSelectedColor));
        } else if (m_hoverIndex==trackIndex) {
            painter.setBrush(QBrush(m_trackHoverColor));
        } else {
            painter.setBrush(QBrush(m_trackColor));
        }

        const QRect rowRect = visualRect(trackIndex);
        painter.drawRect(rowRect);

        painter.save();
        painter.setPen(m_trackTitleColor);
        // 轨道名文本区：对齐 clip 本体行（行内顶部 52px 垂直居中，2026-11-06
        // 用户定案——展开行不再全行居中悬空；左侧留展开按钮、右侧留 grip）
        const QRect textRect(rowRect.left() + 30, rowRect.top(),
                             rowRect.width() - 58, 52);
        painter.drawText(textRect, Qt::AlignCenter,
                         trackIndex.data(TimelineRoles::TrackNameRole).toString());

        // ---- 展开按钮（行头左侧 18px 方块）：▸ 折叠 / ▾ 已展开 ----
        // 几何与 mousePressEvent 命中区共用 trackExpandButtonRect（同源纪律）。
        // 【2026-11-09】m_expandButtonVisible=false 时不绘制（clip 内嵌
        // panel 退役，展开行无内容——宿主隐藏入口，轨道行恒 52px）。
        if (m_expandButtonVisible) {
            const QRect btnRect = trackExpandButtonRect(rowRect);
            painter.setBrush(QBrush(m_trackBgColor));
            painter.setPen(QPen(m_trackTitleColor, 1.2));
            painter.drawRect(btnRect);
            const bool expanded = trackIndex.data(TimelineRoles::TrackExpandedRole).toBool();
            // 手画 chevron（展开 = 向下，折叠 = 向右），绘制与命中同一矩形
            const QPointF c = btnRect.center();
            QPolygonF chevron;
            if (expanded) {
                chevron << QPointF(c.x() - 4, c.y() - 2)
                        << QPointF(c.x(), c.y() + 3)
                        << QPointF(c.x() + 4, c.y() - 2);
            } else {
                chevron << QPointF(c.x() - 2, c.y() - 4)
                        << QPointF(c.x() + 3, c.y())
                        << QPointF(c.x() - 2, c.y() + 4);
            }
            painter.drawPolyline(chevron);
        }
        painter.restore();

        // 轨道拖动 grip（名称行内右侧，与轨道名/展开按钮同一水平线——
        // 2026-11-06 用户定案"名称和移动图标一行"，且名称组对齐 clip 本体
        // 行：y=top+16 即 clip 行内 52px 的垂直居中（中心 26），普通行原值）
        painter.save();
        const QRect gripRect(rowRect.right() - 24, rowRect.top() + 16, 20, 20);
        painter.drawPixmap(gripRect, QPixmap(":/icons/icons/move.png").scaled(
                               gripRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        painter.restore();
    }
    }

    if (event->rect().top() < header)
        drawTitle(&painter);

    painter.restore();
}

void BaseTracklistView::drawTitle(QPainter *painter)
{
    if (!painter) {
        return;
    }

    painter->save();

    painter->setBrush(QBrush(m_trackBgColor));
    QRect rect(0,  0, viewport()->width(), rulerHeight+toolbarHeight);
    painter->drawRect(rect);

    if (Model) {
        painter->setPen(m_timelinePlayheadColor);
        QFont font;
        font.setPixelSize(fontSize);
        painter->setFont(font);
        // 播放头时间码显示（hh:mm:ss:mmm，2026-11-10 微秒原生修正）：
        // getPlayheadPos() 已是 μs——此前误当帧号传入 frames_to_timecode_frame
        // （30.8s 的 μs 值被当作 3 千万帧 ÷25fps → 显示 342 小时的假时间码）。
        // 现与标尺刻度（drawTimeMarkers）同一换算链：μs→秒→时间码。
        const qint64 playheadUs = Model->getPlayheadPos();
        const TimeCodeType tcType  = Model->getTimeCodeType();
        const auto tc = time_to_timecode_frame(
            static_cast<double>(playheadUs) / 1'000'000.0, tcType);
        painter->drawText(rect, Qt::AlignCenter,
                          timecode_frame_to_string(tc, tcType));
    }

    painter->restore();
}

void BaseTracklistView::dragEnterEvent(QDragEnterEvent *event) {
    // 检查是否是我们的自定义MIME类型

    if (event->mimeData()->hasFormat("application/x-track-index")) {
        // 设置移动鼠标形状

        event->acceptProposedAction();

        // unsetCursor();
    } else {
        unsetCursor();
    }
}

void BaseTracklistView::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat("application/x-track-index")) {
        event->acceptProposedAction();
        // 获取当前拖动位置的轨道索引
        QModelIndex dropIndex = indexAt(event->position().toPoint());
        if (dropIndex.isValid()) {
            // 只高亮显示目标位置，不进行选择
            m_hoverIndex = dropIndex;
            viewport()->update();
        } else {
            // 如果不在有效轨道上，设置禁止鼠标形状
            QAbstractItemView::dragMoveEvent(event);

        }
    }
}

void BaseTracklistView::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat("application/x-track-index")) {
        // 获取源轨道和目标轨道的索引
        QByteArray data = event->mimeData()->data("application/x-track-index");
        int sourceRow = data.toInt();
        int targetRow = indexAt(event->position().toPoint()).row();

        // 只在源和目标不同且都有效时进行移动
        if (sourceRow != targetRow && targetRow >= 0 && sourceRow >= 0) {
            if (Model) {
                // 移动前关闭当前已打开的 persistent editor（数量应接近可视行数）
                for (const QPersistentModelIndex& p : m_openPersistentEditors) {
                    if (p.isValid() && indexWidget(p)) {
                        closePersistentEditor(p);
                    }
                }
                m_openPersistentEditors.clear();

                Model->onMoveTrack(sourceRow, targetRow);

                // 清除选择和悬停状态（selectionModel 可能由外部注入；兜底情况下也可能为空）
                if (selectionModel()) {
                    selectionModel()->clearSelection();
                }
                m_hoverIndex = QModelIndex();
                onUpdateViewport();
            }
        }

        event->acceptProposedAction();
    }else
    {
        QAbstractItemView::dropEvent(event);
    }
}
