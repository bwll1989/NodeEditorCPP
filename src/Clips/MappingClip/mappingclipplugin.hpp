#ifndef MAPPINGCLIPPLUGIN_HPP
#define MAPPINGCLIPPLUGIN_HPP

#include <QObject>
#include "TimeLineWidget/clippluginterface.hpp"
#include "TimeLineWidget/AbstractClipModel.hpp"
#include "TimeLineWidget/AbstractClipDelegate.hpp"
#include "mappingclipmodel.hpp"
#include "mappingclipdelegate.hpp"

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
        return new MappingClipModel(start, start+100);
    }

    AbstractClipDelegate* createDelegate() override {
        return new MappingClipDelegate();
    }
};

#endif // MAPPINGCLIPPLUGIN_HPP 