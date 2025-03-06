#include "AbstractClipModel.hpp"
// 实现虚析构函数
AbstractClipModel::~AbstractClipModel() = default;
// Getters
int AbstractClipModel::start() const { return m_start; }
int AbstractClipModel::end() const { return m_end; }
QString AbstractClipModel::type() const { return m_type; }
int AbstractClipModel::length() const { return m_end - m_start; }
int AbstractClipModel::trackIndex() const { return m_trackIndex; }
bool AbstractClipModel::isResizable() const { return RESIZEABLE; }
bool AbstractClipModel::isEmbedWidget() const { return EMBEDWIDGET; }
bool AbstractClipModel::isShowBorder() const { return SHOWBORDER; }
Timecode AbstractClipModel::getStartTimeCode() const {return m_startTimeCode;}
Timecode AbstractClipModel::getEndTimeCode() const {return m_endTimeCode;}
void AbstractClipModel::setTimecodeType(TimecodeType timecodeType) 
{
    m_timecodeType = timecodeType;
    m_startTimeCode = Timecode::fromFrames(m_start, m_timecodeType);
    m_endTimeCode = Timecode::fromFrames(m_end, m_timecodeType);
}
// Setters
void AbstractClipModel::setStart(int start) { 
    if (m_start != start) {
        m_start = start; 
        m_startTimeCode = Timecode::fromFrames(m_start, m_timecodeType);
        emit timelinePositionChanged(m_start);

    }
}
void AbstractClipModel::setEnd(int end) { 
    if (m_end != end) {
        m_end = end;
        m_endTimeCode = Timecode::fromFrames(m_end, m_timecodeType);
        emit lengthChanged();

    }
}
void AbstractClipModel::setTrackIndex(int index) { m_trackIndex = index; }
void AbstractClipModel::setResizable(bool resizable) { RESIZEABLE = resizable; }
void AbstractClipModel::setEmbedWidget(bool embedWidget) { 
    if (EMBEDWIDGET != embedWidget) {
        EMBEDWIDGET = embedWidget;
    }
}
void AbstractClipModel::setShowBorder(bool showBorder) { SHOWBORDER = showBorder; }
// Save/Load
QJsonObject AbstractClipModel::save() const {
    QJsonObject clipJson;
    clipJson["start"] = m_start;
    clipJson["end"] = m_end;
    clipJson["type"] = m_type;
    clipJson["track"] = m_trackIndex;

    return clipJson;
}

void AbstractClipModel::load(const QJsonObject& json) {
    m_start = json["start"].toInt();
    m_end = json["end"].toInt();
    m_type = json["type"].toString();
    m_trackIndex = json["track"].toInt();
}



QVariantMap AbstractClipModel::currentVideoData(int currentFrame) const {

    return QVariantMap();
}

QVariantMap AbstractClipModel::currentAudioData(int currentFrame) const {
    emit audioDataUpdate();
    return QVariantMap();
}

QVariantMap AbstractClipModel::currentControlData(int currentFrame) const {
    emit controlDataUpdate();
    return QVariantMap();
}

// 如果有任何其他非内联成员函数，也可以在这里实现 