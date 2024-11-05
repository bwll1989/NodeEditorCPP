//
// Created by WuBin on 24-10-29.
//
#include <QtGui/QPixmap>
#include "QtNodes/NodeData"
#ifndef PIXMAPDATA_H
#define PIXMAPDATA_H
using QtNodes::NodeData;
using QtNodes::NodeDataType;
class PixmapData : public NodeData
{
public:
    PixmapData() {}

    PixmapData(QPixmap const &pixmap)
            : _pixmap(pixmap)
    {}

    NodeDataType type() const override
    {
        //       id      name
        return {"pixmap", "pixmap"};
    }

    QPixmap pixmap() const { return _pixmap; }

private:
    QPixmap _pixmap;
};

#endif //PIXMAPDATA_H
