#ifndef ARTNETCLIPPLUGIN_HPP
#define ARTNETCLIPPLUGIN_HPP

#include <QObject>
#include "AbstractClipInterface.h"
#include "AbstractClipModel.hpp"
#include "artnetclipmodel.hpp"

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
        return new  Clips::ArtnetClipModel(start);
    }

    // AbstractClipDelegate* createDelegate() override {
    //     return new PlayerClipDelegate();
    // }
};

#endif // ARTNETCLIPPLUGIN_HPP