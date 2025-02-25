#ifndef VIDEOCLIPPLUGIN_HPP
#define VIDEOCLIPPLUGIN_HPP

#include <QObject>
#include "TimeLineWidget/clippluginterface.hpp"
#include "TimeLineWidget/AbstractClipModel.hpp"
#include "TimeLineWidget/AbstractClipDelegate.hpp"
#include "videoclipmodel.hpp"
#include "videoclipdelegate.hpp"

class VideoClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.timeline.ClipPlugInterface")
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Video";
    }

    AbstractClipModel* createModel(int start) override {
        return new VideoClipModel(start, start+100);
    }

    AbstractClipDelegate* createDelegate() override {
        return new VideoClipDelegate();
    }
};

#endif // VIDEOCLIPPLUGIN_HPP 