#include "timelinemodel.hpp"

// 获取播放头位置
int TimelineModel::getPlayheadPos() const{
    return m_timecodeGenerator->getCurrentFrame();
}

// 设置播放头位置
void TimelineModel::setPlayheadPos(int newPlayheadPos)
{
//    if(m_timecodeGenerator->getCurrentFrame()>=0)
//        m_timecodeGenerator->setCurrentFrame(m_timecodeGenerator->getCurrentFrame()+newPlayheadPos);
//    if(m_timecodeGenerator->getCurrentFrame()>m_timecodeGenerator->getMaxFrames())
//        m_timecodeGenerator->setCurrentFrame(m_timecodeGenerator->getCurrentFrame()-newPlayheadPos);
//    if(m_timecodeGenerator->getCurrentFrame()<0)
//        m_timecodeGenerator->setCurrentFrame(0);

//    emit S_playheadMoved(m_timecodeGenerator->getCurrentFrame());
    emit S_timelineUpdated();
}

QModelIndex TimelineModel::index(int row, int column, const QModelIndex &parent) const
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
        TrackModel* track = static_cast<TrackModel*>(parent.internalPointer());
        if (track && row < track->getClips().size()) {
            return createIndex(row, column, track->getClips()[row]);
        }
    }
    return QModelIndex();
}

QModelIndex TimelineModel::parent(const QModelIndex &child) const
{
    // 如果子索引无效，返回空索引
    if (!child.isValid()) {
        return QModelIndex();
    }

    // Check if the child is a ClipModel
    AbstractClipModel* clip = static_cast<AbstractClipModel*>(child.internalPointer());
    if (clip) {
        // 获取剪辑所在轨道
        TrackModel* track = findParentTrackOfClip(clip);
        if (track) {
            // 获取轨道行
            int trackRow = findTrackRow(track);
            // 创建索引
            return createIndex(trackRow, 0, track);
        }
    }

    return QModelIndex();
}


int TimelineModel::rowCount(const QModelIndex &parent) const
{
    // 如果父索引无效，返回轨道数量
    if (!parent.isValid()) {
        // Root level: number of tracks
        return m_tracks.size();
    }
    // 获取轨道
    TrackModel* track = static_cast<TrackModel*>(parent.internalPointer());
    if (track) {
        // 返回剪辑数量
        return track->getClips().size();
    }

    return 0;
}

int TimelineModel::columnCount(const QModelIndex &parent) const
{
    // 如果父索引无效，返回1
    Q_UNUSED(parent);
    return 1; // Only one column for both tracks and clips
}


QVariant TimelineModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        switch (role) {
            case TimelineLengthRole:
                return QVariant::fromValue(m_timecodeGenerator->getMaxFrames());
            default:
                return QVariant();
        }
    }

    if (index.parent().isValid()) {
        // Clip level
        AbstractClipModel* clip = static_cast<AbstractClipModel*>(index.internalPointer());
        if (!clip) {
            qDebug() << "Clip pointer is null in data()";
            return QVariant();
        }

        // qDebug() << "Processing role:" << role << "for clip at" << index.row();
        
        switch (role) {
            case ClipModelRole:
                qDebug() << "Returning clip model from TimelineModel";
                return QVariant::fromValue(clip);
            case ClipInRole:
                return QVariant::fromValue(clip->start());
            //获取剪辑结束时间
            case ClipOutRole:
                return QVariant::fromValue(clip->end());
            //获取剪辑长度
            case ClipLengthRole:
                return QVariant::fromValue(clip->length());
            //获取剪辑类型
            case ClipTypeRole:
                return QVariant::fromValue(clip->type());
            //获取剪辑是否显示小部件
            case ClipShowWidgetRole:
                return QVariant::fromValue(clip->isEmbedWidget());
            //获取剪辑是否可调整大小
            case ClipResizableRole:
                return QVariant::fromValue(clip->isResizable());
            //获取剪辑是否显示边框
            case ClipShowBorderRole:
                return QVariant::fromValue(clip->isShowBorder());
            // //获取剪辑时间开始
            // case ClipTimeInRole:
            //     return QVariant::fromValue(FramesToTimeString(clip->start(),m_timecodeGenerator->getFrameRate()));
            // //获取剪辑时间结束
            // case ClipTimeOutRole:
            //     return QVariant::fromValue(FramesToTimeString(clip->end(),m_frameRate));
            default:
                return clip->data(role);
        }
    } else {
        // Track level
        TrackModel* track = static_cast<TrackModel*>(index.internalPointer());
            switch (role){
                //获取轨道工具提示
            case Qt::ToolTipRole:
                return QVariant::fromValue("track " + QString::number(index.row()));
            //获取轨道编号
            case TrackNumberRole:
                return QVariant::fromValue(index.row());
            //获取轨道类型
            case TrackTypeRole:
                return QVariant::fromValue(TrackTypeString(track->type()));
            default:
                return QVariant();
        }
    }
    return QVariant();
}

void TimelineModel::onStartPlay(){
    m_timecodeGenerator->onStart();
}
void TimelineModel::onPausePlay(){
    m_timecodeGenerator->onPause();
}
void TimelineModel::onStopPlay(){
    m_timecodeGenerator->onStop();
}


// 设置数据
bool TimelineModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !index.parent().isValid())
        return false;
    AbstractClipModel* clip = static_cast<AbstractClipModel*>(index.internalPointer());

    if (!clip)
        return false;

    switch (role) {
        // 设置剪辑开始时间
        case TimelineRoles::ClipInRole: {
            int newStart = value.toInt();
            int clipLength = clip->length();
            clip->setStart(newStart);
            clip->setEnd(newStart + clipLength);
            emit dataChanged(index, index);
            onTimelineLengthChanged();
            return true;
        }
        // 设置剪辑是否显示小部件
        case TimelineRoles::ClipShowWidgetRole: {
            bool newShowWidget = value.toBool();
            clip->setEmbedWidget(newShowWidget);
            emit dataChanged(index, index);
            return true;
        }
        // 设置剪辑是否可调整大小
        case TimelineRoles::ClipResizableRole: {
            bool newResizable = value.toBool();
            clip->setResizable(newResizable);
            emit dataChanged(index, index);
            return true;
        }
        // 设置剪辑结束时间
        case TimelineRoles::ClipOutRole: {
            int newEnd = value.toInt();
            clip->setEnd(newEnd);
            emit dataChanged(index, index);
            onTimelineLengthChanged();  
            return true;
        }
        // 设置剪辑长度
        case TimelineRoles::ClipLengthRole: {
            int newLength = value.toInt();
            clip->setEnd(clip->start()+newLength);
            emit dataChanged(index, index);
            onTimelineLengthChanged();
            return true;
        }
        // 设置剪辑是否显示边框
        case TimelineRoles::ClipShowBorderRole: {
            bool newShowBorder = value.toBool();
            clip->setShowBorder(newShowBorder);
            emit dataChanged(index, index);
            return true;
        }
        // 可以添加其他角色的处理
        default:
            return false;
    }

}

// 获取支持的拖放操作
Qt::DropActions TimelineModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

// 获取项目标志
Qt::ItemFlags TimelineModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled; 
}


void TimelineModel::clear(){
    while(rowCount()>0){
        onDeleteTrack(0);
    }
    
}

void TimelineModel::onAddTrack(const QString& type) {
    addTrack(type);
}
// 保存模型
QJsonObject TimelineModel::save() const {
    QJsonObject modelJson;
    QJsonArray trackArray;
    auto timecodeType = m_timecodeGenerator->getTimecodeType();
    modelJson["timecodeType"] = static_cast<int>(timecodeType);
    modelJson["displayFormat"] = static_cast<int>(m_timeDisplayFormat);
    modelJson["isLooping"] = m_timecodeGenerator->isLooping();
    for (const TrackModel* track : m_tracks) {
        trackArray.append(track->save());
    }
    
    modelJson["tracks"] = trackArray;
    modelJson["length"] = m_timecodeGenerator->getMaxFrames();

    // 保存舞台信息
    if (m_stage) {
        modelJson["stage"] = m_stage->save();
    }

    return modelJson;
}

void TimelineModel::load(const QJsonObject &modelJson) {
    clear();
    m_timecodeGenerator->setMaxFrames(modelJson["length"].toInt());
    
    // 先设置时间码类型
    TimecodeType type = static_cast<TimecodeType>(modelJson["timecodeType"].toInt());
    m_timecodeGenerator->setTimecodeType(type);
    m_timecodeGenerator->setLooping(modelJson["isLooping"].toBool());
    m_timeDisplayFormat = static_cast<TimedisplayFormat>(modelJson["displayFormat"].toInt());
    // 加载轨道
    QJsonArray trackArray = modelJson["tracks"].toArray();
    for (const QJsonValue &trackValue : trackArray) {
        QJsonObject trackJson = trackValue.toObject();
        QString type = trackJson["type"].toString();
        TrackModel* track = new TrackModel(trackJson["trackIndex"].toInt(), type);
        track->load(trackJson, m_pluginLoader);
        m_tracks.push_back(track);
    }

    // 加载舞台信息
    if (modelJson.contains("stage")) {
        if (!m_stage) {
            m_stage = new TimelineStage();
        }
        m_stage->load(modelJson["stage"].toObject());
    }

    emit S_timelineUpdated();
    emit S_trackChanged();
    emit S_stageChanged();
}
// 计算时间线长度
void TimelineModel::onTimelineLengthChanged()
{
    // qDebug()<<"onTimelineLengthChanged";
    int max = 0;
    for(TrackModel* track : m_tracks){
        max = qMax(max,track->getTrackLength());
    }
   
    m_timecodeGenerator->setMaxFrames(max);
}

void TimelineModel::onDeleteTrack(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        qDebug() << "Invalid track index";
        return;
    }

    beginRemoveRows(QModelIndex(), trackIndex, trackIndex); // 开始移除行
   
    delete m_tracks[trackIndex]; // 删除轨道对象
    m_tracks.erase(m_tracks.begin() + trackIndex); // 从列表中移除轨道
    // 重新计算时间线长度
    for (int i = trackIndex; i < m_tracks.size(); ++i) {
        m_tracks[i]->setTrackIndex(i);
    }

    onTimelineLengthChanged();
    endRemoveRows(); // 结束移除行

    emit S_trackChanged();
    emit S_timelineUpdated();
} 
// 创建轨道
void TimelineModel::addTrack(const QString& type) {
    if (!m_pluginLoader) return;
        
        beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size());
        TrackModel* track = new TrackModel(m_tracks.size(), type);
        connect(track, &TrackModel::S_trackLengthChanged, this, &TimelineModel::onTimelineLengthChanged);
        connect(track, &TrackModel::S_trackLengthChanged, this, &TimelineModel::S_timelineUpdated);
        m_tracks.push_back(track);
        endInsertRows();
        emit S_trackChanged();
        emit S_timelineUpdated();
    }

void TimelineModel::deleteClip(QModelIndex clipIndex){
     if (!clipIndex.isValid() || !clipIndex.parent().isValid())
        return;

    // 获取轨道索引和轨道
    QModelIndex trackIndex = clipIndex.parent();
    TrackModel* track = m_tracks[trackIndex.row()];
    if (!track)
        return;

    // 开始删除行
    beginRemoveRows(trackIndex, clipIndex.row(), clipIndex.row());

    // 获取要删除的片段
    AbstractClipModel* clip = track->getClips()[clipIndex.row()];

    // 从轨道中移除片段
    track->removeClip(clip);
    // 删除片段对象
    delete clip;

    // 结束删除行
    endRemoveRows();

    // 重新计算时间线长度
    onTimelineLengthChanged();

    // 发出信号通知视图更新
    emit S_timelineUpdated();
}


TrackModel* TimelineModel::findParentTrackOfClip(AbstractClipModel* clip) const {
    for (TrackModel* track : m_tracks) {
        QVector<AbstractClipModel*>& clips = track->getClips();
        if (std::find(clips.begin(), clips.end(), clip) != clips.end()) {
            return track;
        }
    }
    return nullptr; // Return nullptr if no parent track is found
}
// 查找轨道行
int TimelineModel::findTrackRow(TrackModel* track) const {
    auto it = std::find(m_tracks.begin(), m_tracks.end(), track);
    if (it != m_tracks.end()) {
        return std::distance(m_tracks.begin(), it);
    }
    return -1; // Return -1 if the track is not found
}

void TimelineModel::setPluginLoader(PluginLoader* loader) {
    m_pluginLoader = loader;
}

PluginLoader* TimelineModel::getPluginLoader() const {
    return m_pluginLoader;
}



// 在析构函数中清理
TimelineModel::~TimelineModel()
{
    if (m_stage) {
        delete m_stage;
        m_stage = nullptr;
    }
}

void TimelineModel::setStage(TimelineStage* stage)
{
    if (m_stage != stage) {
        if (m_stage) {
            delete m_stage;
        }
        m_stage = stage;
        emit S_stageChanged();
    }
}



// 修改时间码类型设置
void TimelineModel::onTimecodeTypeChanged(TimecodeType type)
{
    m_timecodeGenerator->setTimecodeType(type);
}


