#ifndef TRACKMODEL_HPP
#define TRACKMODEL_HPP
#pragma once
#include <QObject>
#include "Widget/TimeLineWidget/TimelineAbstract/timelinetypes.h"
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include "Widget/TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include "pluginloader.hpp"
// 前向声明 TimelineModel 类
class TimelineModel;

// TrackModel类
class TrackModel : public QObject
{
Q_OBJECT
public:
    // 构造函数
    explicit TrackModel(int number, const QString& type, QObject* parent = nullptr)
        : QObject(parent),
        m_type(type),
        m_name(type),
        m_trackIndex(number)
    {
        connect(this,TrackModel::S_trackAddClip,this,TrackModel::onCalculateTrackLength);
        connect(this,TrackModel::S_trackDeleteClip,this,TrackModel::onCalculateTrackLength);
    }

    
    TrackModel() = default;


    /**
     * 移除剪辑
     * @param AbstractClipModel* clip 剪辑
     */
    void removeClip(AbstractClipModel* clip) {
        auto it = std::find(m_clips.begin(), m_clips.end(), clip);
        if(it != m_clips.end()) {
            // 关闭属性面板
            // if (QWidget* propertyWidget = clip->showPropertyWidget()) {
            //     propertyWidget->close();
            // }
            
            // 从列表中移除并删除片段
            m_clips.erase(it);
            delete clip;
            
            // 发出信号
            emit S_trackDeleteClip();
        }
    }

    /**
     * 获取剪辑
     * @return QVector<AbstractClipModel*>& 剪辑列表
     */
    QVector<AbstractClipModel*>& getClips(){
        return m_clips;
    }
    /**
     * 获取轨道类型
     * @return QString 轨道类型
     */
    QString type() const{
        return m_type;
    }
    /**
     * 获取轨道索引
     * @return int 轨道索引
     */
    int trackIndex() const{
        return m_trackIndex;
    }
    /**
     * 保存轨道
     * @return QJsonObject 轨道JSON
     */
    QJsonObject save() const
    {
        QJsonObject trackJson;
        trackJson["type"] = m_type;
        trackJson["trackIndex"] = m_trackIndex;
        trackJson["trackLength"] = m_trackLength;
        trackJson["trackName"] = m_name;
        return trackJson;
    }
    /**
     * 获取轨道长度
     * @return qint64 轨道长度
     */
    qint64 getTrackLength() const {
        return m_trackLength;
    }
    /**
     * 加载轨道
     * @param const QJsonObject &trackJson 轨道JSON
     * @param PluginLoader* pluginLoader 插件加载器
     */
    void setName(const QString& name){
        m_name = name;
    }
    QString getName() const{
        return m_name;
    }
    void load(const QJsonObject &trackJson, PluginLoader* pluginLoader) {
        if (!pluginLoader) return;
        
        m_type = trackJson["type"].toString();
        m_trackIndex = trackJson["trackIndex"].toInt();
        m_trackLength = trackJson["trackLength"].toInt();
        m_name = trackJson["trackName"].toString();
      
    }

public:
    Q_SIGNALS:
    /**
     * 轨道长度变化信号
     * @param qint64 length 轨道长度
     */
    void S_trackLengthChanged(qint64 length);

    void S_trackAddClip();

    void S_trackDeleteClip();

    void S_trackIndexChanged(int oldIndex, int newIndex );
public slots:
     /**
     * 计算轨道长度
     */
    void onCalculateTrackLength(){
        qint64 length = 0;
        for(AbstractClipModel* clip : m_clips){
            length = qMax(length, clip->end()+1);
        }
        if(length != m_trackLength){
            m_trackLength = length;
            emit S_trackLengthChanged(length);
        }
    }
    /**
     * 设置轨道索引
     * @param int index 轨道索引
     */
    void onSetTrackIndex(int index){
        int oldindex=m_trackIndex;
        m_trackIndex = index;
        emit S_trackIndexChanged(oldindex,index);
    }
    /**
     * 添加剪辑
     * @param int start 开始
     * @param PluginLoader* pluginLoader 插件加载器
     */
    void onAddClip(AbstractClipModel* newClip) {

        //片段长度变化时更新轨道长度
        connect(newClip, &AbstractClipModel::lengthChanged, this, &TrackModel::onCalculateTrackLength);
        // connect(newClip, &AbstractClipModel::timelinePositionChanged, this, &TrackModel::onTimelinePositionChanged);
        if (newClip) {
            newClip->setTrackIndex(m_trackIndex);
            m_clips.push_back(newClip);
            emit S_trackAddClip();
        }

    }
private:
     //下一个ID
    quint64 nextId = 1;
    //轨道索引
    int m_trackIndex; 
    //轨道类型
    QString m_type; 
    //剪辑列表
    QVector<AbstractClipModel*>  m_clips; 
    //轨道长度
    qint64 m_trackLength;
    QString m_name;
 
};

#endif // TRACKMODEL_HPP
