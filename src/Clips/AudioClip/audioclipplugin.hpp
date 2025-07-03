#ifndef AUDIOCLIPPLUGIN_HPP
#define AUDIOCLIPPLUGIN_HPP

#include <QObject>
#include "AbstractClipInterface.h"
#include "AbstractClipModel.hpp"
#include "Audioclipmodel.hpp"

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
        return new  Clips::AudioClipModel(start);
    }

    // AbstractClipDelegate* createDelegate() override {
    //     return new PlayerClipDelegate();
    // }
};

#endif // AUDIOCLIPPLUGIN_HPP