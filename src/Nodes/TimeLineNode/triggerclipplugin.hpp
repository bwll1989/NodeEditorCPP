#ifndef TRIGGERCLIPPLUGIN_HPP
#define TRIGGERCLIPPLUGIN_HPP

#include <QObject>
#include "TimelineEngine.h"
#include "triggerclipmodel.hpp"

class TriggerClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Trigger";
    }
    TimeLineClipModel* createModel(int start) override {
        return new Clips::TriggerClipModel(start);
    }
};

#endif // TRIGGERCLIPPLUGIN_HPP 
