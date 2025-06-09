#ifndef VIDEOCLIPPLUGIN_HPP
#define VIDEOCLIPPLUGIN_HPP

#include <QObject>
#include "AbstractClipInterface.h"
#include "AbstractClipModel.hpp"
#include "imageclipmodel.hpp"


class ImageClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.timeline.ClipPlugInterface")
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Image";
    }

    AbstractClipModel* createModel(int start) override {
        return new ImageClipModel(start);
    }

    // AbstractClipDelegate* createDelegate() override {
    //     return new PlayerClipDelegate();
    // }
};

#endif // VIDEOCLIPPLUGIN_HPP 