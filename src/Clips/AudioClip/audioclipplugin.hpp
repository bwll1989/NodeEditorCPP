#ifndef AUDIOCLIPPLUGIN_HPP
#define AUDIOCLIPPLUGIN_HPP

#include <QObject>
#include "TimeLineWidget/clippluginterface.hpp"
#include "TimeLineWidget/AbstractClipModel.hpp"
#include "TimeLineWidget/AbstractClipDelegate.hpp"
#include "audioclipmodel.hpp"
#include "audioclipdelegate.hpp"

class AudioClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.timeline.ClipPlugInterface")
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Audio";
    }

    AbstractClipModel* createModel(int start) override {
        return new AudioClipModel(start,start+100);
    }

    AbstractClipDelegate* createDelegate() override {
        return new AudioClipDelegate();
    }
};

#endif // AUDIOCLIPPLUGIN_HPP