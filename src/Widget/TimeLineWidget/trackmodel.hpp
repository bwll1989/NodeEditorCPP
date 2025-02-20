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

    // 添加剪辑
    // void addClip(ClipModel* clip){
    //     if (clip) {
    //         int row = m_clips.size();
    //         m_clips.push_back(clip);
    //         nextId++;
    void addClip(int start, PluginLoader* pluginLoader) {
        if (!pluginLoader) return;
        
        AbstractClipModel* newClip = pluginLoader->createModelForType(m_type, start);
        if (newClip) {
            newClip->setTrackIndex(m_trackIndex);
            m_clips.push_back(newClip);
        }
    }

    // 移除剪辑
    void removeClip(AbstractClipModel* clip){
        auto it = std::find(m_clips.begin(), m_clips.end(), clip);
        if(it!=m_clips.end())
            m_clips.erase(it);
    }

    // 获取剪辑
    QVector<AbstractClipModel*>& getClips(){
        return m_clips;
    }

    // 获取轨道类型
    QString type() const{
        return m_type;
    }

    // 设置轨道索引
    void setTrackIndex(int index){
        m_trackIndex = index;
    }   
    // 获取轨道索引
    int trackIndex() const{
        return m_trackIndex;
    }
    // 保存轨道
    QJsonObject save() const
    {
        QJsonObject trackJson;
        trackJson["type"] = m_type;
        trackJson["trackIndex"] = m_trackIndex;

        QJsonArray clipArray;
        for (const AbstractClipModel* clip : m_clips) {
            clipArray.append(clip->save());
        }
        trackJson["clips"] = clipArray;

        return trackJson;
    }
    // 下一个ID
    quint64 nextId = 1;

    int m_trackIndex; // 轨道索引
    QString m_type; // 轨道类型
    QVector<AbstractClipModel*>  m_clips; // 剪辑列表

    void load(const QJsonObject &trackJson, PluginLoader* pluginLoader) {
        if (!pluginLoader) return;
        
        m_type = trackJson["type"].toString();
        m_trackIndex = trackJson["trackIndex"].toInt();

        QJsonArray clipArray = trackJson["clips"].toArray();
        for (const QJsonValue &clipValue : clipArray) {
            QJsonObject clipObject = clipValue.toObject();
            QString clipType = clipObject["type"].toString();
            int start = clipObject["start"].toInt();
            int end = clipObject["end"].toInt();
            
            AbstractClipModel* clip = pluginLoader->createModelForType(clipType, start);
            if (clip) {
                clip->setTrackIndex(m_trackIndex);
                clip->load(clipObject);
                m_clips.push_back(clip);
            }
        }
    }
};

#endif // TRACKMODEL_H
