#ifndef IMAGECLIPPLUGIN_HPP
#define IMAGECLIPPLUGIN_HPP

#include <QObject>
#include "TimeLineWidget/clippluginterface.hpp"
#include "TimeLineWidget/AbstractClipModel.hpp"
#include "TimeLineWidget/AbstractClipDelegate.hpp"
#include "imageclipmodel.hpp"
#include "imageclipdelegate.hpp"

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
        return new ImageClipModel(start, start+100);
    }

    AbstractClipDelegate* createDelegate() override {
        return new ImageClipDelegate();
    }
};

#endif // IMAGECLIPPLUGIN_HPP 