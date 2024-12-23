#ifndef TRACKMODEL_H
#define TRACKMODEL_H
#pragma once
#include <QObject>
#include "timelinetypes.h"
#include <QJsonObject>
#include <QJsonArray>
#include "clipmodel.hpp"
class ClipModel;

class TrackModel
{

public:
    explicit TrackModel(int number, MediaType type = MediaType::CONTROL): m_id(number), m_type(type)
    {

    }

    TrackModel() = default;

    void addClip(ClipModel* clip){
        m_clips.push_back(clip);
    }

    void removeClip(ClipModel* clip){
        auto it = std::find(m_clips.begin(), m_clips.end(), clip);
        if(it!=m_clips.end())
            m_clips.erase(it);
    }

    const std::vector<ClipModel*>& getClips(){
        return m_clips;
    }

    MediaType type() const{
        return m_type;
    }

    QJsonObject save() const
    {
        QJsonObject sceneJson;
        sceneJson["id"]=m_id;
        QString typeString;
        switch (m_type) {
            case MediaType::VIDEO:
                typeString = "VIDEO";
                break;
            case MediaType::AUDIO:
                typeString = "AUDIO";
                break;
            case MediaType::CONTROL:
                typeString = "CONTROL";
                break;
        }
        sceneJson["type"] = typeString;
        QJsonArray clipArray;
        for(auto const &index:m_clips){
            clipArray.append(index->save());
        }
        sceneJson["clips"] = clipArray;
        return sceneJson;
    }

private:

    int m_id;
    MediaType m_type;
    std::vector<ClipModel*> m_clips;

};

#endif // TRACKMODEL_H
