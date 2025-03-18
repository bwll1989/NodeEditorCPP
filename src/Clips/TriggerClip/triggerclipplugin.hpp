#ifndef TRIGGERCLIPPLUGIN_HPP
#define TRIGGERCLIPPLUGIN_HPP

#include <QObject>
#include "TimeLineWidget/TimelineAbstract/AbstractClipInterface.hpp"
#include "TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include "triggerclipmodel.hpp"

class TriggerClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.timeline.ClipPlugInterface")
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Trigger";
    }

    AbstractClipModel* createModel(int start) override {
        return new TriggerClipModel(start, start);
    }
};

#endif // TRIGGERCLIPPLUGIN_HPP 