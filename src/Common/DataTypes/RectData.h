//
// Created by pablo on 3/1/24.
//

#pragma once

#include <QtNodes/NodeData>
#include <QRect>
#include <QRectF>
#include "DataTypesExport.h"
namespace NodeDataTypes
{
    class DATATYPES_EXPORT RectData final : public QtNodes::NodeData {
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

