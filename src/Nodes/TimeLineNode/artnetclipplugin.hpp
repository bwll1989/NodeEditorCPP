#ifndef ARTNETCLIPPLUGIN_HPP
#define ARTNETCLIPPLUGIN_HPP

#include <QObject>
#include "TimelineEngine.h"
#include "artnetclipmodel.hpp"

class ArtnetClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Artnet";
    }

    TimeLineClipModel* createModel(int start) override {
        return new  Clips::ArtnetClipModel(start);
    }

    // AbstractClipDelegate* createDelegate() override {
    //     return new PlayerClipDelegate();
    // }
};

#endif // ARTNETCLIPPLUGIN_HPP
