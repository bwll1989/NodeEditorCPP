#pragma once

#include <QColor>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QtGlobal>
#include <cmath>

namespace NodeDataTypes
{
/** 将载荷解析为 float 列表；不足 size 的分量补 0，超出则截断。size<=0 时保留自然长度。 */
inline QVector<float> floatVectorFromVariant(const QVariant &val, int size = 0)
{
    QVector<float> values;
    if (val.isValid()) {
        const int typeId = val.typeId();
        if (typeId == QMetaType::QVariantList) {
            const QVariantList list = val.toList();
            values.reserve(list.size());
            for (const QVariant &item : list) {
                values.push_back(float(item.toDouble()));
            }
        } else if (typeId == QMetaType::QVector3D) {
            const QVector3D v = val.value<QVector3D>();
            values = {v.x(), v.y(), v.z()};
        } else if (typeId == QMetaType::QVector4D) {
            const QVector4D v = val.value<QVector4D>();
            values = {v.x(), v.y(), v.z(), v.w()};
        } else if (typeId == QMetaType::QSize) {
            const QSize s = val.toSize();
            values = {float(s.width()), float(s.height())};
        } else if (typeId == QMetaType::QSizeF) {
            const QSizeF s = val.toSizeF();
            values = {float(s.width()), float(s.height())};
        } else if (typeId == QMetaType::QRect) {
            const QRect r = val.toRect();
            values = {float(r.x()), float(r.y()), float(r.width()), float(r.height())};
        } else if (typeId == QMetaType::QRectF) {
            const QRectF r = val.toRectF();
            values = {float(r.x()), float(r.y()), float(r.width()), float(r.height())};
        } else if (typeId == QMetaType::QPoint) {
            const QPoint p = val.toPoint();
            values = {float(p.x()), float(p.y())};
        } else if (typeId == QMetaType::QPointF) {
            const QPointF p = val.toPointF();
            values = {float(p.x()), float(p.y())};
        } else if (typeId == QMetaType::QVariantMap) {
            return floatVectorFromVariant(val.toMap().value(QStringLiteral("default")), size);
        } else if (typeId != QMetaType::QString && typeId != QMetaType::QByteArray
                   && val.canConvert<double>()) {
            values = {float(val.toDouble())};
        }
    }
    if (size > 0) {
        if (values.size() < size) {
            values.resize(size);
        } else if (values.size() > size) {
            values.resize(size);
        }
    }
    return values;
}

inline QVariantList floatVectorToList(const QVector<float> &values)
{
    QVariantList list;
    list.reserve(values.size());
    for (float v : values) {
        list.append(double(v));
    }
    return list;
}

inline bool floatVectorFuzzyEqual(const QVector<float> &a, const QVector<float> &b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (int i = 0; i < a.size(); ++i) {
        if (qAbs(double(a[i]) - double(b[i])) > 1e-6) {
            return false;
        }
    }
    return true;
}

/** 将色相折到 [0, 1) */
inline double wrapHueF(double h)
{
    if (!std::isfinite(h)) {
        return 0.0;
    }
    h -= std::floor(h);
    if (h < 0.0) {
        h += 1.0;
    }
    if (h >= 1.0) {
        h = 0.0;
    }
    return h;
}

/** RGB 0–1 → HSV 0–1 */
inline void rgbToHsv(double r, double g, double b, double &h, double &s, double &v)
{
    const double maxc = qMax(r, qMax(g, b));
    const double minc = qMin(r, qMin(g, b));
    v = maxc;
    const double d = maxc - minc;
    s = maxc < 1e-12 ? 0.0 : d / maxc;
    if (d < 1e-12) {
        h = 0.0;
        return;
    }
    if (maxc == r) {
        h = (g - b) / d;
        if (h < 0.0) {
            h += 6.0;
        }
    } else if (maxc == g) {
        h = (b - r) / d + 2.0;
    } else {
        h = (r - g) / d + 4.0;
    }
    h /= 6.0;
}

/** HSV 0–1 → RGB 0–1 */
inline void hsvToRgb(double h, double s, double v, double &r, double &g, double &b)
{
    h = wrapHueF(h);
    s = qBound(0.0, s, 1.0);
    v = qBound(0.0, v, 1.0);
    if (s < 1e-12) {
        r = g = b = v;
        return;
    }
    const double hh = h * 6.0;
    const int i = int(std::floor(hh)) % 6;
    const double f = hh - std::floor(hh);
    const double p = v * (1.0 - s);
    const double q = v * (1.0 - f * s);
    const double t = v * (1.0 - (1.0 - f) * s);
    switch (i) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
}

inline QVector<float> hsvFromRgb(float r, float g, float b)
{
    double h, s, v;
    rgbToHsv(r, g, b, h, s, v);
    return {float(h), float(s), float(v)};
}

inline QVector<float> rgbFromHsv(float h, float s, float v)
{
    double r, g, b;
    hsvToRgb(h, s, v, r, g, b);
    return {float(r), float(g), float(b)};
}

/**
 * 颜色在 VariableData.default 中的约定：
 * - 主形态：RGBA 浮点列表 [r, g, b, a]，范围 0–1（3 元则 a=1）
 * - 兼容：0–255 整数列表（任一分量 > 1 则按字节解释）、QColor、#hex / 颜色名
 */
inline QVariantList colorToList(const QColor &c)
{
    if (!c.isValid()) {
        return {0.0, 0.0, 0.0, 1.0};
    }
    return {c.redF(), c.greenF(), c.blueF(), c.alphaF()};
}

inline QColor colorFromList(const QVariantList &list)
{
    if (list.isEmpty()) {
        return {};
    }

    auto at = [&](int i, double fallback) {
        return i < list.size() ? list.at(i).toDouble() : fallback;
    };

    const double r = at(0, 0.0);
    const double g = list.size() >= 3 ? at(1, 0.0) : r;
    const double b = list.size() >= 3 ? at(2, 0.0) : r;
    const bool hasAlpha = list.size() >= 4;
    const double a = hasAlpha ? at(3, 1.0) : 1.0;

    const bool asByte = r > 1.0 || g > 1.0 || b > 1.0 || a > 1.0;
    QColor c;
    if (asByte) {
        c.setRgb(qBound(0, int(std::lround(r)), 255),
                 qBound(0, int(std::lround(g)), 255),
                 qBound(0, int(std::lround(b)), 255),
                 hasAlpha ? qBound(0, int(std::lround(a)), 255) : 255);
    } else {
        c.setRgbF(qBound(0.0, r, 1.0),
                  qBound(0.0, g, 1.0),
                  qBound(0.0, b, 1.0),
                  hasAlpha ? qBound(0.0, a, 1.0) : 1.0);
    }
    return c;
}

inline bool isColorPayload(const QVariant &val)
{
    if (!val.isValid()) {
        return false;
    }
    const int typeId = val.typeId();
    if (typeId == QMetaType::QColor) {
        return val.value<QColor>().isValid();
    }
    if (typeId == QMetaType::QVector3D || typeId == QMetaType::QVector4D) {
        return true;
    }
    if (typeId == QMetaType::QVariantList) {
        return val.toList().size() >= 3;
    }
    if (typeId == QMetaType::QVariantMap) {
        const QVariantMap map = val.toMap();
        return map.contains(QStringLiteral("default"))
            || map.contains(QStringLiteral("r")) || map.contains(QStringLiteral("R"))
            || map.contains(QStringLiteral("x"));
    }
    if (typeId == QMetaType::QString || typeId == QMetaType::QByteArray) {
        return QColor(val.toString()).isValid();
    }
    return false;
}

inline QColor colorFromVariant(const QVariant &val)
{
    if (!val.isValid()) {
        return {};
    }

    const int typeId = val.typeId();
    if (typeId == QMetaType::QColor) {
        return val.value<QColor>();
    }
    if (typeId == QMetaType::QVector3D) {
        const QVector3D v = val.value<QVector3D>();
        QColor c;
        c.setRgbF(v.x(), v.y(), v.z(), 1.0);
        return c;
    }
    if (typeId == QMetaType::QVector4D) {
        const QVector4D v = val.value<QVector4D>();
        QColor c;
        c.setRgbF(v.x(), v.y(), v.z(), v.w());
        return c;
    }
    if (typeId == QMetaType::QVariantList) {
        return colorFromList(val.toList());
    }
    if (typeId == QMetaType::QVariantMap) {
        const QVariantMap map = val.toMap();
        if (map.contains(QStringLiteral("default"))) {
            return colorFromVariant(map.value(QStringLiteral("default")));
        }
        const QVariant r = map.value(QStringLiteral("r"),
            map.value(QStringLiteral("R"), map.value(QStringLiteral("x"))));
        const QVariant g = map.value(QStringLiteral("g"),
            map.value(QStringLiteral("G"), map.value(QStringLiteral("y"))));
        const QVariant b = map.value(QStringLiteral("b"),
            map.value(QStringLiteral("B"), map.value(QStringLiteral("z"))));
        QVariantList list{r, g, b};
        if (map.contains(QStringLiteral("a")) || map.contains(QStringLiteral("A"))
            || map.contains(QStringLiteral("w"))) {
            list.append(map.value(QStringLiteral("a"),
                map.value(QStringLiteral("A"), map.value(QStringLiteral("w")))));
        }
        return colorFromList(list);
    }
    if (typeId == QMetaType::QString || typeId == QMetaType::QByteArray) {
        return QColor(val.toString());
    }
    return {};
}

/** 消费端 RGBA：0–1 长度 4。向量不足补 0，缺 alpha 则为 1，超出截断；#hex / QColor 仍兼容。 */
inline QVector<float> rgbaVectorFromVariant(const QVariant &val)
{
    QVector<float> v = floatVectorFromVariant(val, 0);
    if (!v.isEmpty()) {
        bool asByte = false;
        for (float x : v) {
            if (x > 1.0f) {
                asByte = true;
                break;
            }
        }
        if (v.size() > 4) {
            v.resize(4);
        }
        const bool hadAlpha = v.size() >= 4;
        if (v.size() < 4) {
            v.resize(4);
        }
        if (!hadAlpha) {
            v[3] = asByte ? 255.0f : 1.0f;
        }
        if (asByte) {
            for (float &x : v) {
                x = float(qBound(0.0, double(x) / 255.0, 1.0));
            }
        }
        return v;
    }

    const QColor c = colorFromVariant(val);
    if (c.isValid()) {
        return {float(c.redF()), float(c.greenF()), float(c.blueF()), float(c.alphaF())};
    }
    return {0.0f, 0.0f, 0.0f, 1.0f};
}

inline QColor qcolorFromRgba(const QVector<float> &rgba)
{
    auto at = [&](int i, double fallback) {
        return i < rgba.size() ? qBound(0.0, double(rgba[i]), 1.0) : fallback;
    };
    return QColor::fromRgbF(at(0, 0.0), at(1, 0.0), at(2, 0.0), at(3, 1.0));
}

inline QVector<float> rgbaFromQColor(const QColor &c)
{
    if (!c.isValid()) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    return {float(c.redF()), float(c.greenF()), float(c.blueF()), float(c.alphaF())};
}

} // namespace NodeDataTypes
