#ifndef CURVECLIPPLUGIN_HPP
#define CURVECLIPPLUGIN_HPP

#include <QObject>

#include "AbstractClipInterface.h"
#include "AbstractClipModel.hpp"
#include "curveclipmodel.hpp"

class CurveClipPlugin : public QObject, public ClipPlugInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.timeline.ClipPlugInterface")
    Q_INTERFACES(ClipPlugInterface)

public:
    QString clipType() const override
    {
        return QStringLiteral("Curve");
    }

    AbstractClipModel* createModel(int start) override
    {
        return new Clips::CurveClipModel(start);
    }
};

#endif
