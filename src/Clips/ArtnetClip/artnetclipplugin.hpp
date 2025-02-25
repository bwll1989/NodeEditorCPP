#ifndef ARTNETCLIPPLUGIN_HPP
#define ARTNETCLIPPLUGIN_HPP

#include <QObject>
#include "TimeLineWidget/clippluginterface.hpp"
#include "TimeLineWidget/AbstractClipModel.hpp"
#include "TimeLineWidget/AbstractClipDelegate.hpp"
#include "artnetclipmodel.hpp"
#include "artnetclipdelegate.hpp"

class ArtnetClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.timeline.ClipPlugInterface")
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Artnet";
    }

    AbstractClipModel* createModel(int start) override {
        return new ArtnetClipModel(start, start+100);
    }

    AbstractClipDelegate* createDelegate() override {
        return new ArtnetClipDelegate();
    }
};

#endif // ARTNETCLIPPLUGIN_HPP 