#ifndef TRIGGERCLIPPLUGIN_HPP
#define TRIGGERCLIPPLUGIN_HPP

#include <QObject>
#include "AbstractClipInterface.h"
#include "AbstractClipModel.hpp"
#include "mappingclipmodel.hpp"

class MappingClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.timeline.ClipPlugInterface")
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Mapping";
    }

    AbstractClipModel* createModel(int start) override {
        return new Clips::MappingClipModel(start);
    }
};

#endif // TRIGGERCLIPPLUGIN_HPP 