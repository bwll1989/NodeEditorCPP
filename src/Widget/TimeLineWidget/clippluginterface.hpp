#ifndef CLIPPLUGINTERFACE_HPP
#define CLIPPLUGINTERFACE_HPP

#include <QtPlugin>
#include "AbstractClipModel.hpp"
#include "AbstractClipDelegate.hpp"

class ClipPlugInterface
{
public:
    virtual ~ClipPlugInterface() {}
    virtual QString clipType() const = 0;
    virtual AbstractClipModel* createModel(int start) = 0;
    virtual AbstractClipDelegate* createDelegate() = 0;
};

#define ClipPlugInterface_iid "com.timeline.ClipPlugInterface"
Q_DECLARE_INTERFACE(ClipPlugInterface, ClipPlugInterface_iid)

#endif // CLIPPLUGINTERFACE_HPP