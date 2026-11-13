#include "BaseTimeLineModel.h"
#include <QTimer>


BaseTimeLineModel::BaseTimeLineModel(QObject* parent) : QAbstractItemModel(parent), m_pluginLoader(nullptr)
{
      // 创建并初始化插件加载器
    m_pluginLoader = new BasePluginLoader(this);
    m_pluginLoader->loadPlugins();
}
// 获取播放头位置
qint64 BaseTimeLineModel::getPlayheadPos() const{
    return m_currentFrame;
}
// 设置播放头位置（μs）
void BaseTimeLineModel::onSetPlayheadPos(qint64 newPlayheadPos)
{
    m_currentFrame = newPlayheadPos;
    emit S_playheadMoved(newPlayheadPos);

}

QModelIndex BaseTimeLineModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    if (!parent.isValid()) {
        // Track level
        if (row < m_tracks.size()) {
            return createIndex(row, column, m_tracks[row]);
        }
    } else {
        // Clip level
        TrackData* track = static_cast<TrackData*>(parent.internalPointer());
        if (track && row < track->clips.size()) {
            return createIndex(row, column, track->clips[row]);
        }
    }
    return QModelIndex();
}

QModelIndex BaseTimeLineModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return {};

    void* ptr = child.internalPointer();
    if (!ptr || isTrackPointer(ptr))
        return {};

    const auto it = m_clipLocation.find(static_cast<AbstractClipModel*>(ptr));
    if (it == m_clipLocation.end())
        return {};

    const int trackRow = it->second.trackRow;
    if (trackRow < 0 || trackRow >= m_tracks.size())
        return {};
    return createIndex(trackRow, 0, m_tracks[trackRow]);
}

int BaseTimeLineModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_tracks.size();

    void* ptr = parent.internalPointer();
    if (!isTrackPointer(ptr))
        return 0;
    return static_cast<TrackData*>(ptr)->clips.size();
}

int BaseTimeLineModel::columnCount(const QModelIndex &parent) const
{
    // 如果父索引无效，返回1
    Q_UNUSED(parent);
    return 1; // Only one column for both tracks and clips
}

QVariant BaseTimeLineModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        switch (role) {
            case Qt::ToolTipRole:
                return QVariant::fromValue(QString("Length:%1").arg(m_lengthFrame));
            case TimelineLengthRole:
                return QVariant::fromValue(m_lengthFrame);
            case TimeCodeTypeRole:
                return QVariant::fromValue(getTimeCodeType());
            default:
                return QVariant();
        }
    }

    void* ptr = index.internalPointer();
    if (!ptr)
        return {};

    if (!isTrackPointer(ptr)) {
        AbstractClipModel* clip = static_cast<AbstractClipModel*>(ptr);
        if (!clip)
            return {};
        switch (role) {
            case Qt::ToolTipRole:
                return QVariant::fromValue(QString(clip->type()+ ":%1 to %2").arg(clip->start()).arg(clip->end()));
            //获取剪辑模型
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(clip);
            //获取剪辑开始时间
            case TimelineRoles::ClipInRole:
                return QVariant::fromValue(clip->start());
            //获取剪辑结束时间
            case TimelineRoles::ClipOutRole:
                return QVariant::fromValue(clip->end());
            //获取剪辑长度
            case TimelineRoles::ClipLengthRole:
                return QVariant::fromValue(clip->length());
            //获取剪辑类型
            case TimelineRoles::ClipTypeRole:
                return QVariant::fromValue(clip->type());
            //获取剪辑是否显示小部件
            case TimelineRoles::ClipShowWidgetRole:
                return QVariant::fromValue(clip->isEmbedWidget());
            //获取剪辑是否可调整大小
            case TimelineRoles::ClipResizableRole:
                return QVariant::fromValue(clip->isResizable());
            //获取剪辑是否显示边框
            case TimelineRoles::ClipShowBorderRole:
                return QVariant::fromValue(clip->isShowBorder());
            //获取剪辑ID
            case TimelineRoles::ClipIdRole:
                return QVariant::fromValue(clip->id());
            case TimelineRoles::TimeCodeTypeRole:
                return QVariant::fromValue(clip->getTimeCodeType());
            case TimelineRoles::ClipColorRole:
                return clip->color();
            case TimelineRoles::ClipNoteRole:
                return clip->noteName();
            default:
                return clip->data(role);
        }
    } else {
        TrackData* track = static_cast<TrackData*>(ptr);
            switch (role){
            //获取轨道工具提示
            case Qt::ToolTipRole:
                return QVariant::fromValue(track->name);
            //获取轨道类型
            case TrackTypeRole:
                return QVariant::fromValue(TrackTypeString(track->type));
            //获取轨道名称
            case TrackNameRole:
                return QVariant::fromValue(track->name);
            //获取轨道模型
            case TrackModelRole:
                return QVariant::fromValue(track);
            //获取轨道剪辑数量
            case TrackClipsCountRole:
                return QVariant::fromValue(track->clips.size());
            //获取轨道剪辑列表
            case TrackClipsRole:
                return QVariant::fromValue(track->clips);
            //轨道展开态（2026-11-06 轨道展开 + clip 内嵌 panel）
            case TrackExpandedRole:
                return QVariant::fromValue(track->expanded);
            default:
                return QVariant();
        }
    }
    return QVariant();
}

// 设置数据
bool BaseTimeLineModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    //如果索引是时间轴本身
    if (!index.isValid()) {
        return false;
    }
    if (index.parent().isValid()){
        //如果索引是剪辑
        AbstractClipModel* clip = static_cast<AbstractClipModel*>(index.internalPointer());

        if (!clip)
            return false;

        switch (role) {
            // 设置剪辑开始时间
            case TimelineRoles::ClipInRole: {
                int newStart = value.toInt();
                clip->setStart(newStart);
                return true;
            }
            case TimelineRoles::ClipPosRole:{
                int newPos=value.toInt();
                int clipLength = clip->length();
                clip->setStart(newPos);
                clip->setEnd(newPos+clipLength);
                return true;
            }
            case TimelineRoles::ClipIdRole:{
                clip->setId(value.toInt());
                emit dataChanged(index, index, {TimelineRoles::ClipIdRole});
                return true;
            }
            // 设置剪辑是否显示小部件（AbstractClipModel::setEmbedWidget 不发信号，需要 Model 主动通知）
            case TimelineRoles::ClipShowWidgetRole: {
                const bool newShowWidget = value.toBool();
                clip->setEmbedWidget(newShowWidget);
                emit dataChanged(index, index, {TimelineRoles::ClipShowWidgetRole});
                return true;
            }
            // 设置剪辑是否可调整大小（AbstractClipModel::setResizable 不发信号，需要 Model 主动通知）
            case TimelineRoles::ClipResizableRole: {
                const bool newResizable = value.toBool();
                clip->setResizable(newResizable);
                emit dataChanged(index, index, {TimelineRoles::ClipResizableRole});
                return true;
            }
            // 设置剪辑结束时间（clip->setEnd 会触发 lengthChanged，Model 已监听并转为 dataChanged）
            case TimelineRoles::ClipOutRole: {
                const int newEnd = value.toInt();
                clip->setEnd(newEnd);
                return true;
            }
            // 设置剪辑长度（clip->setEnd 会触发 lengthChanged，Model 已监听并转为 dataChanged）
            case TimelineRoles::ClipLengthRole: {
                const int newLength = value.toInt();
                clip->setEnd(clip->start()+newLength);
                return true;
            }
            // 设置剪辑是否显示边框（AbstractClipModel::setShowBorder 不发信号，需要 Model 主动通知）
            case TimelineRoles::ClipShowBorderRole: {
                const bool newShowBorder = value.toBool();
                clip->setShowBorder(newShowBorder);
                emit dataChanged(index, index, {TimelineRoles::ClipShowBorderRole});
                return true;
            }
            // 设置时间码类型（使用 TimeCodeTypeRole 表达，避免误用 ClipTypeRole）
            case TimelineRoles::TimeCodeTypeRole: {
                const TimeCodeType type = static_cast<TimeCodeType>(value.toInt());
                clip->setTimeCodeType(type);
                emit dataChanged(index, index, {TimelineRoles::TimeCodeTypeRole});
                return true;
            }
            case TimelineRoles::ClipColorRole: {
                QColor color = value.value<QColor>();
                if (!color.isValid())
                    color = QColor(value.toString());
                clip->setColor(color);
                return true;
            }
            case TimelineRoles::ClipNoteRole:
                clip->setNoteName(value.toString());
                return true;
            default:
                return false;
        }

    }else{
        TrackData* track = static_cast<TrackData*>(index.internalPointer());
        if (!track)
            return false;
        switch (role){
            //设置轨道名称
            case TimelineRoles::TrackNameRole: {
                /**
                 * @brief 更新轨道名称
                 * @details
                 * TrackListView 使用 openPersistentEditor 创建持久编辑器。
                 * 若这里不发 dataChanged，编辑器可能一直显示创建时的旧文本，
                 * 导致“重新导入后顺序对但 TrackName 显示错误”。
                 */
                track->name = value.toString();
                emit dataChanged(index, index, {role});
                return true;
            }
            //设置轨道展开态（2026-11-06 轨道展开 + clip 内嵌 panel）：
            //dataChanged 通知两视图重绘并按新行高重排（滚动条/附属面板随
            //performRedraw → layoutClipPanels 统一处理）。
            case TimelineRoles::TrackExpandedRole: {
                const bool on = value.toBool();
                if (track->expanded == on)
                    return true;
                track->expanded = on;
                invalidateRowPrefix(); // P4：行高变化，前缀和失效
                emit dataChanged(index, index, {TimelineRoles::TrackExpandedRole});
                return true;
            }
            default:
                return false;
        }

    }
}

// 获取支持的拖放操作
Qt::DropActions BaseTimeLineModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

// ---- 轨道行几何（2026-11-06 轨道展开：per-row 行高，两视图同源同规则）----
// P4（timeline-draw-zoom-review）：前缀和缓存替代逐行累加——行展开/增删/
// 移动/清空置脏，查询时懒重建一次 O(n)，此后 trackRowY/totalTracksHeight/
// trackRowAt 均 O(1)。几十轨道无感，百级轨道长工程的全页重建/缩放从
// O(n²) 行高求值降为线性。
// 所有视图几何（绘制/命中/滚动）统一经由本组函数取行高与行顶 y，禁止再
// 直接写 row * trackHeight（否则展开行错位）。

int BaseTimeLineModel::trackRowHeight(int row) const
{
    if (row < 0 || row >= m_tracks.size())
        return trackHeight;
    return m_tracks.at(row)->expanded ? (trackHeight + trackExpandedExtra)
                                      : trackHeight;
}

void BaseTimeLineModel::ensureRowPrefix() const
{
    // P4：懒重建。行数一致且未置脏时直接复用（防漏置脏的 size 兜底）。
    if (!m_rowPrefixDirty
        && m_rowPrefixY.size() == static_cast<size_t>(m_tracks.size()) + 1)
        return;
    m_rowPrefixY.assign(static_cast<size_t>(m_tracks.size()) + 1, 0);
    for (int i = 0; i < m_tracks.size(); ++i)
        m_rowPrefixY[static_cast<size_t>(i) + 1] =
            m_rowPrefixY[static_cast<size_t>(i)] + trackRowHeight(i);
    m_rowPrefixDirty = false;
}

int BaseTimeLineModel::trackRowY(int row) const
{
    if (row <= 0 || m_tracks.isEmpty())
        return 0;
    ensureRowPrefix();
    // 原实现对 row 超界返回全部行之和，此处语义一致
    return m_rowPrefixY[static_cast<size_t>(qMin(row, m_tracks.size()))];
}

int BaseTimeLineModel::trackRowAt(int contentY) const
{
    if (contentY < 0 || m_tracks.isEmpty())
        return -1;
    ensureRowPrefix();
    for (int row = 0; row < m_tracks.size(); ++row) {
        if (contentY < m_rowPrefixY[static_cast<size_t>(row) + 1])
            return row;
    }
    return -1;
}

int BaseTimeLineModel::totalTracksHeight() const
{
    ensureRowPrefix();
    return m_rowPrefixY.empty() ? 0 : m_rowPrefixY.back();
}

bool BaseTimeLineModel::isTrackExpanded(int row) const
{
    return row >= 0 && row < m_tracks.size() && m_tracks.at(row)->expanded;
}

void BaseTimeLineModel::setTrackExpanded(int row, bool on)
{
    if (row < 0 || row >= m_tracks.size())
        return;
    const QModelIndex idx = index(row, 0);
    setData(idx, QVariant::fromValue(on), TimelineRoles::TrackExpandedRole);
}

// 获取项目标志
Qt::ItemFlags BaseTimeLineModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.isValid() && !index.parent().isValid())
        flags |= Qt::ItemIsEditable;
    return flags; 
}


void BaseTimeLineModel::clear(){
    while(rowCount()>0){
        onDeleteTrack(0);
    }

}

// 计算时间线长度
qint64 BaseTimeLineModel::onUpdateTimeLineLength()
{
    // qDebug()<<"onTimelineLengthChanged";
    // 2026-11-11 极限缩放崩溃根因：max 曾是 int——μs 域 clip->end() 超过
    // 35.8 分钟（2^31 μs）即整截断回绕（14.14e9 μs → 1.255e9 μs），时间轴
    // 长度随机变小 → 滚动 range/缩放锚点钳制/绘制过滤全部基于错误内容域。
    // （"μs 进 int 即雷"铁律；clip->end() 是 qint64）
    qint64 max = 0;
    for(const auto& track : m_tracks){
        for(const auto& clip : track->clips){
            max = qMax(max, clip->end());
        }
    }
    if(max!=m_lengthFrame){
        m_lengthFrame = max;
        emit S_LengthChanged(m_lengthFrame);

    }
    return m_lengthFrame;
}

void BaseTimeLineModel::onDeleteTrack(int trackIndex) {
    /**
     * @brief 删除轨道
     * @details
     * 旧实现的问题：
     * - 通过 createIndex(0,0,clip) 传给 onDeleteClip，parent 无效，导致 onDeleteClip 直接 return，片段不会被释放。
     * - 结果是删除轨道时 clip 内存泄漏，并且视图/选择状态也可能出现悬挂指针。
     *
     * 新实现策略：
     * - 删除轨道行时（beginRemoveRows/endRemoveRows）由父行移除负责通知视图；
     * - 在移除轨道前，直接释放该轨道拥有的所有 clip 对象，确保内存与指针安全。
     */

    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        qDebug() << "Invalid track index";
        return;
    }

    beginRemoveRows(QModelIndex(), trackIndex, trackIndex);

    TrackData* track = m_tracks[trackIndex];
    if (track) {
        // 释放该轨道上的所有片段
        for (AbstractClipModel* clip : track->clips) {
            delete clip;
        }
        track->clips.clear();
    }

    // 从容器中移除轨道并释放轨道对象
    m_tracks.removeAt(trackIndex);
    invalidateRowPrefix(); // P4：行数变化，前缀和失效
    delete track;
    rebuildIndex();

    endRemoveRows();
}  
// 通过类型名创建轨道
void BaseTimeLineModel::onAddTrack(const QString& type) {
    if (!m_pluginLoader) return;
        
    beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size());
    auto track = new TrackData();
    track->type = type;
    track->name = type+" " + QString::number(m_tracks.size());
    m_tracks.push_back(std::move(track));
    invalidateRowPrefix(); // P4：行数变化，前缀和失效
    rebuildIndex();
    endInsertRows();
}

void BaseTimeLineModel::onMoveTrack(int sourceRow, int targetRow) {
    /**
     * @brief 移动轨道
     * @details
     * 使用 Qt 标准的 beginMoveRows/endMoveRows 通知视图更新，
     * 避免仅依赖自定义信号导致视图状态（选择、持久编辑器、滚动位置）不同步。
     */
    if (sourceRow < 0 || sourceRow >= m_tracks.size()) {
        return;
    }
    if (targetRow < 0 || targetRow >= m_tracks.size()) {
        return;
    }
    if (sourceRow == targetRow) {
        return;
    }

    // Qt 约定：当在同一 parent 内向下移动时，destinationRow 需要 +1（因为 source 行先被移除）
    const int destinationRow = (sourceRow < targetRow) ? (targetRow + 1) : targetRow;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow, QModelIndex(), destinationRow);
    m_tracks.move(sourceRow, targetRow);
    invalidateRowPrefix(); // P4：行序变化（展开行位置改变），前缀和失效
    rebuildIndex();
    endMoveRows();


}

void BaseTimeLineModel::onAddClip(int trackIndex, qint64 startUs) {
    /**
     * @brief 在指定轨道添加片段
     * @details
     * 本次重构的核心目标是“让 Qt Model/View 的绑定回到标准路径”：
     * - 插入子项必须使用 beginInsertRows/endInsertRows 通知视图，而不是仅依赖自定义信号全局刷新。
     * - index/createIndex 的 row 必须是“片段行号”，旧代码用 trackIndex 作为 row 创建 clip 索引，会造成索引语义错误。
     */

    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        qDebug() << "Invalid track index:" << trackIndex;
        return;
    }
    if (!getPluginLoader()) {
        qDebug() << "Plugin loader is not initialized";
        return;
    }

    TrackData* track = m_tracks[trackIndex];
    if (!track) {
        qDebug() << "Track pointer is null";
        return;
    }

    AbstractClipModel* newClip = getPluginLoader()->createModelForType(track->type, startUs);
    if (!newClip) {
        qDebug() << "Failed to create clip model";
        return;
    }

    newClip->setTimelineModel(this);
    newClip->setTimeCodeType(getTimeCodeType());

    watchClip(newClip);

    // 插入到 track->clips 的尾部
    const int insertRow = track->clips.size();
    const QModelIndex trackModelIndex = index(trackIndex, 0, QModelIndex());

    beginInsertRows(trackModelIndex, insertRow, insertRow);
    track->clips.push_back(newClip);
    rebuildIndex();
    endInsertRows();

    // 为新片段设置唯一 ID（使用标准 index(row,col,parent) 获取正确索引）
    const QModelIndex clipIndex = index(insertRow, 0, trackModelIndex);
    setData(clipIndex, QVariant::fromValue(m_clipNextId++), TimelineRoles::ClipIdRole);
    applyLengthChange(0, newClip->end());
}


void BaseTimeLineModel::onDeleteClip(QModelIndex clipIndex){
    if (!clipIndex.isValid() || !clipIndex.parent().isValid())
        return;

    // 获取轨道索引和轨道
    QModelIndex trackIndex = clipIndex.parent();
    TrackData* track = m_tracks[trackIndex.row()];
    if (!track)
        return;
        
    // 开始删除行
    beginRemoveRows(trackIndex, clipIndex.row(), clipIndex.row());

    // 获取要删除的片段
    if(clipIndex.row() < track->clips.size()) {
        AbstractClipModel* clip = track->clips[clipIndex.row()];
        // 从轨道中移除片段
        track->clips.erase(track->clips.begin() + clipIndex.row());
        m_clipLocation.erase(clip);
        delete clip;

    }

    rebuildIndex();
    endRemoveRows();
    //更新时间线长度
    onUpdateTimeLineLength();
}

/**
 * @brief 通过 clip 指针反查其在模型中的索引
 * @param clip 片段指针
 * @return clip 对应的 QModelIndex；若未找到返回无效索引
 */
QModelIndex BaseTimeLineModel::indexForClip(AbstractClipModel* clip) const
{
    if (!clip)
        return {};

    const auto it = m_clipLocation.find(clip);
    if (it == m_clipLocation.end())
        return {};

    const int trackRow = it->second.trackRow;
    const int clipRow = it->second.clipRow;
    if (trackRow < 0 || trackRow >= m_tracks.size())
        return {};

    const QModelIndex trackIndex = index(trackRow, 0, QModelIndex());
    return index(clipRow, 0, trackIndex);
}

/**
 * @brief 查找片段所在的轨道
 * @param clip 要查找的片段指针
 * @return 返回包含该片段的轨道的智能指针，如果未找到则返回nullptr
 */
void BaseTimeLineModel::rebuildIndex()
{
    m_clipLocation.clear();
    m_trackSet.clear();
    for (int trackRow = 0; trackRow < m_tracks.size(); ++trackRow) {
        TrackData* track = m_tracks[trackRow];
        if (!track)
            continue;
        m_trackSet.insert(track);
        for (int clipRow = 0; clipRow < track->clips.size(); ++clipRow) {
            AbstractClipModel* clip = track->clips[clipRow];
            if (!clip)
                continue;
            m_clipLocation[clip] = ClipLocation{trackRow, clipRow, clip->end()};
        }
        rebuildTrackClipOrder(track);
    }
}

void BaseTimeLineModel::rebuildTrackClipOrder(TrackData* track)
{
    if (!track)
        return;
    track->clipsByStart.clear();
    track->maxClipSpan = 0;
    for (int row = 0; row < track->clips.size(); ++row) {
        AbstractClipModel* clip = track->clips[row];
        if (!clip)
            continue;
        track->clipsByStart.push_back(row);
        track->maxClipSpan = qMax(track->maxClipSpan, clip->end() - clip->start());
    }
    std::sort(track->clipsByStart.begin(), track->clipsByStart.end(), [track](int a, int b) {
        const int sa = track->clips[a] ? track->clips[a]->start() : 0;
        const int sb = track->clips[b] ? track->clips[b]->start() : 0;
        if (sa != sb)
            return sa < sb;
        return a < b;
    });
}

void BaseTimeLineModel::beginInteractiveGeometryChange(AbstractClipModel* clip)
{
    if (!clip)
        return;
    if (!m_deferGeometryNotify)
        m_interactiveOriginalEnd = clip->end();
    m_interactiveClip = clip;
    m_deferGeometryNotify = true;
}

void BaseTimeLineModel::endInteractiveGeometryChange()
{
    AbstractClipModel* clip = m_interactiveClip;
    const int originalEnd = m_interactiveOriginalEnd;
    m_interactiveClip = nullptr;
    m_deferGeometryNotify = false;
    if (!clip)
        return;

    const auto located = m_clipLocation.find(clip);
    if (located != m_clipLocation.end())
        located->second.end = clip->end();
    noteClipGeometryChanged(clip);
    applyLengthChange(originalEnd, clip->end());

    const QModelIndex idx = indexForClip(clip);
    if (idx.isValid()) {
        emit dataChanged(idx, idx, {
            TimelineRoles::ClipInRole,
            TimelineRoles::ClipOutRole,
            TimelineRoles::ClipPosRole,
            TimelineRoles::ClipLengthRole
        });
    }
}

void BaseTimeLineModel::syncInteractiveClipOrder()
{
    if (m_interactiveClip)
        noteClipGeometryChanged(m_interactiveClip);
}

void BaseTimeLineModel::watchClip(AbstractClipModel* clip)
{
    if (!clip)
        return;
    connect(clip, &AbstractClipModel::lengthChanged, this, [this, clip](int) {
        queueClipGeometryNotify(clip);
    });
    connect(clip, &AbstractClipModel::timelinePositionChanged, this, [this, clip](int) {
        queueClipGeometryNotify(clip);
    });
    connect(clip, &AbstractClipModel::appearanceChanged, this, [this, clip]() {
        if (m_clipLocation.find(clip) == m_clipLocation.end())
            return;
        const QModelIndex idx = indexForClip(clip);
        if (!idx.isValid())
            return;
        emit dataChanged(idx, idx, {TimelineRoles::ClipColorRole, TimelineRoles::ClipNoteRole});
    });
}

void BaseTimeLineModel::queueClipGeometryNotify(AbstractClipModel* clip)
{
    if (!clip || m_deferGeometryNotify)
        return;
    if (m_clipLocation.find(clip) == m_clipLocation.end())
        return;
    if (!m_pendingGeometryClips.contains(clip))
        m_pendingGeometryClips.append(clip);
    if (m_geometryNotifyQueued)
        return;
    m_geometryNotifyQueued = true;
    // setStart 和 setEnd 会连着发两个信号。等当前调用返回后合并成一次通知。
    QTimer::singleShot(0, this, [this] { flushClipGeometryNotify(); });
}

void BaseTimeLineModel::flushClipGeometryNotify()
{
    m_geometryNotifyQueued = false;
    const QList<AbstractClipModel*> clips = m_pendingGeometryClips;
    m_pendingGeometryClips.clear();
    for (AbstractClipModel* clip : clips) {
        const auto located = m_clipLocation.find(clip);
        if (located == m_clipLocation.end())
            continue;
        const int oldEnd = located->second.end;
        const int newEnd = clip->end();
        located->second.end = newEnd;
        noteClipGeometryChanged(clip);
        applyLengthChange(oldEnd, newEnd);
        const QModelIndex idx = indexForClip(clip);
        if (idx.isValid()) {
            emit dataChanged(idx, idx, {
                TimelineRoles::ClipInRole,
                TimelineRoles::ClipOutRole,
                TimelineRoles::ClipPosRole,
                TimelineRoles::ClipLengthRole
            });
        }
    }
}

void BaseTimeLineModel::applyLengthChange(int oldEnd, int newEnd)
{
    if (newEnd > m_lengthFrame) {
        m_lengthFrame = newEnd;
        emit S_LengthChanged(m_lengthFrame);
        return;
    }
    if (oldEnd >= m_lengthFrame && newEnd < m_lengthFrame)
        onUpdateTimeLineLength();
}

void BaseTimeLineModel::noteClipGeometryChanged(AbstractClipModel* clip)
{
    if (!clip)
        return;
    const auto it = m_clipLocation.find(clip);
    if (it == m_clipLocation.end())
        return;
    const int trackRow = it->second.trackRow;
    if (trackRow < 0 || trackRow >= m_tracks.size())
        return;
    rebuildTrackClipOrder(m_tracks[trackRow]);
}

bool BaseTimeLineModel::isTrackPointer(void* ptr) const
{
    return ptr && m_trackSet.find(static_cast<TrackData*>(ptr)) != m_trackSet.end();
}

void BaseTimeLineModel::setPluginLoader(BasePluginLoader* loader) {
    if (m_pluginLoader) {
        delete m_pluginLoader;
    }
    m_pluginLoader = loader;
    m_pluginLoader->loadPlugins();
}

BasePluginLoader* BaseTimeLineModel::getPluginLoader() const {
    return m_pluginLoader;
}

// 在析构函数中清理
BaseTimeLineModel::~BaseTimeLineModel()
{
    /**
     * @brief 析构函数
     * @details
     * 当前数据结构使用裸指针（TrackData* / AbstractClipModel*），因此必须显式释放。
     * 旧代码仅 m_tracks.clear() 会导致轨道与片段全部泄漏。
     */
    m_clipLocation.clear();
    m_trackSet.clear();
    for (TrackData* track : m_tracks) {
        if (!track) continue;
        for (AbstractClipModel* clip : track->clips) {
            delete clip;
        }
        track->clips.clear();
        delete track;
    }
    m_tracks.clear();

    delete m_pluginLoader;
    m_pluginLoader = nullptr;
}

int BaseTimeLineModel::getTrackCount() const {
    return m_tracks.size();
}


