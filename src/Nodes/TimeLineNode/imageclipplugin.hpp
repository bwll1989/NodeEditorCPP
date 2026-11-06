#ifndef IMAGECLIPPLUGIN_HPP
#define IMAGECLIPPLUGIN_HPP

#include <QObject>
#include "TimelineEngine.h"
#include "imageclipmodel.hpp"


class ImageClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Image";
    }

    AbstractClipModel* createModel(int start) override {
        return new Clips::ImageClipModel(start);
    }

    // AbstractClipDelegate* createDelegate() override {
    //     return new PlayerClipDelegate();
    // }
};

#endif // VIDEOCLIPPLUGIN_HPP 
