//
// Created by pablo on 3/1/24.
//

#ifndef RECTDATA_H
#define RECTDATA_H

#include <QtNodes/NodeData>
#include <QRect>
#include <QRectF>
namespace NodeDataTypes
{
    class RectData final : public QtNodes::NodeData {
    public:
        RectData() {
        }
        explicit RectData(const QRectF& rect) : m_rect(rect) {
        }

        QRect rect() const {
            return m_rect.toRect();
        }

        QtNodes::NodeDataType type() const override {
            return QtNodes::NodeDataType{"rect", "rect"};
        }
        QRectF rectF() const {
            return m_rect;
        }
    private:
        QRectF m_rect;
    };
}
#endif //RECTDATA_H
