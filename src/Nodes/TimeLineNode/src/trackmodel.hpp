#ifndef TRACKMODEL_H
#define TRACKMODEL_H

#include <QObject>
#include "timelinetypes.h"

class TimelineModel;
class ClipModel;

class TrackModel
{

public:
    explicit TrackModel(int number, MediaType type = MediaType::CONTROL): m_number(number), m_type(type)
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

private:
    int m_number;
    int m_id;
    MediaType m_type;
    TimelineModel* m_parent;
    std::vector<ClipModel*> m_clips;

};

#endif // TRACKMODEL_H
