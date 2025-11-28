//
// Created by pablo on 3/1/24.
//

#pragma once
#include "DataTypesExport.h"
#include <QtNodes/NodeData>
#include <QRect>
#include <QList>

typedef QList<QRect> Rects;
namespace NodeDataTypes
{
    class DATATYPES_EXPORT RectsData final : public QtNodes::NodeData {
    public:
        RectsData() {
        }
        explicit RectsData(const QRect& rect) : m_rects({rect}) {
        }
        explicit RectsData(const QList<QRect>& rects) : m_rects(rects) {
        }

        QList<QRect> rects() {
            return m_rects;
        }

        QtNodes::NodeDataType type() const override {
            return QtNodes::NodeDataType{"rects", "rects"};
        }
    private:
        QList<QRect> m_rects;
    };
}

