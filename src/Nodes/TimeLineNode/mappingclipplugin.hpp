#ifndef MAPPINGCLIPPLUGIN_HPP
#define MAPPINGCLIPPLUGIN_HPP

#include <QObject>
#include "TimelineEngine.h"
#include "mappingclipmodel.hpp"

class MappingClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Mapping";
    }

    TimeLineClipModel* createModel(int start) override {
        return new Clips::MappingClipModel(start);
    }
};

#endif // TRIGGERCLIPPLUGIN_HPP 
