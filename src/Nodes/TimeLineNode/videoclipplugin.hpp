#pragma once
#include <QObject>
#include "TimelineEngine.h"
#include "videoclipmodel.hpp"

class VideoClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override {
        return "Video";
    }

    TimeLineClipModel* createModel(int start) override {
        return new  Clips::VideoClipModel(start);
    }

};
