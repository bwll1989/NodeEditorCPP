#ifndef CURVECLIPPLUGIN_HPP
#define CURVECLIPPLUGIN_HPP

#include <QObject>

#include "TimelineEngine.h"
#include "curveclipmodel.hpp"

class CurveClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override
    {
        return QStringLiteral("Curve");
    }

    TimeLineClipModel* createModel(int start) override
    {
        return new Clips::CurveClipModel(start);
    }
};

#endif
