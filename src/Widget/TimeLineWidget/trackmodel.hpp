#ifndef TRACKMODEL_HPP
#define TRACKMODEL_HPP
#pragma once
#include <QObject>
#include "timelinetypes.h"
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include "AbstractClipModel.hpp"
// #include "clips/VideoClip/videoclipmodel.hpp"
// #include "clips/AudioClip/audioclipmodel.hpp"
// #include "clips/MappingClip/mappingclipmodel.hpp"
// #include "clips/TriggerClip/triggerclipmodel.hpp"
#include "PluginLoader.hpp"

// TrackModel类
class TrackModel : public QObject
{
Q_OBJECT
public:
    // 构造函数
    explicit TrackModel(int number, const QString& type, QObject* parent = nullptr)
        : m_type(type), m_trackIndex(number), QObject(parent)
    {}

    TrackModel() = default;
    /**
     * 添加剪辑
     * @param int start 开始
     * @param PluginLoader* pluginLoader 插件加载器
     */
    void addClip(int start, PluginLoader* pluginLoader,TimecodeType timecodeType) {
        if (!pluginLoader) return;
        
        AbstractClipModel* newClip = pluginLoader->createModelForType(m_type, start);
        //片段长度变化时更新轨道长度
        connect(newClip, &AbstractClipModel::lengthChanged, this, &TrackModel::calculateTrackLength);
        newClip->setTimecodeType(timecodeType);
        if (newClip) {
            newClip->setTrackIndex(m_trackIndex);
            m_clips.push_back(newClip);
            emit S_trackAddClip();
        }
        calculateTrackLength();
    }

    /**
     * 移除剪辑
     * @param AbstractClipModel* clip 剪辑
     */
    void removeClip(AbstractClipModel* clip){
        auto it = std::find(m_clips.begin(), m_clips.end(), clip);
        if(it!=m_clips.end())
            m_clips.erase(it);
        emit S_trackDeleteClip();
        calculateTrackLength();
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
     * 设置轨道索引
     * @param int index 轨道索引
     */
    void setTrackIndex(int index){
        m_trackIndex = index;
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
        QJsonArray clipArray;
        for (const AbstractClipModel* clip : m_clips) {
            clipArray.append(clip->save());
        }
        trackJson["clips"] = clipArray;

        return trackJson;
    }
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
    void load(const QJsonObject &trackJson, PluginLoader* pluginLoader) {
        if (!pluginLoader) return;
        
        m_type = trackJson["type"].toString();
        m_trackIndex = trackJson["trackIndex"].toInt();
        m_trackLength = trackJson["trackLength"].toInt();
        QJsonArray clipArray = trackJson["clips"].toArray();
        for (const QJsonValue &clipValue : clipArray) {
            QJsonObject clipObject = clipValue.toObject();
            QString clipType = clipObject["type"].toString();
            int start = clipObject["start"].toInt();
            int end = clipObject["end"].toInt();

            AbstractClipModel* clip = pluginLoader->createModelForType(clipType, start);
            //片段长度变化时更新轨道长度
            connect(clip, &AbstractClipModel::lengthChanged, this, &TrackModel::calculateTrackLength);
            clip->setEnd(end);
            if (clip) {

                clip->setTrackIndex(m_trackIndex);
                clip->load(clipObject);
                m_clips.push_back(clip);
                emit S_trackAddClip();
            }
        }
        calculateTrackLength();
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

public slots:
     /**
     * 计算轨道长度
     */
    void calculateTrackLength(){
        qint64 length = 0;
        for(AbstractClipModel* clip : m_clips){
            length = qMax(length, clip->end());
        }
        if(length != m_trackLength){
            m_trackLength = length;
            emit S_trackLengthChanged(length);
        }
    }
};

#endif // TRACKMODEL_H
