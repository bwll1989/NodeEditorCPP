#ifndef TRIGGERCLIPPLUGIN_HPP
#define TRIGGERCLIPPLUGIN_HPP

#include <QObject>
#include "AbstractClipInterface.h"
#include "AbstractClipModel.hpp"
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
        return new Clips::TriggerClipModel(start);
    }
};

#endif // TRIGGERCLIPPLUGIN_HPP 