#include "CurvePath.hpp"

#include <QtMath>

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kCentripetalAlpha = 0.5f;

float knotSpan(const QVector3D& a, const QVector3D& b)
{
    const float distance = std::max((b - a).length(), 1.0e-4f);
    return std::pow(distance, kCentripetalAlpha);
}

QVector3D lerpKnot(const QVector3D& a, const QVector3D& b, float ta, float tb, float t)
{
    const float span = tb - ta;
    if (std::abs(span) < 1.0e-6f) {
        return a;
    }
    const float weight = (t - ta) / span;
    return a * (1.0f - weight) + b * weight;
}

QVector3D centripetalPoint(const QVector3D& p0, const QVector3D& p1, const QVector3D& p2, const QVector3D& p3, float u)
{
    const float t0 = 0.0f;
    const float t1 = t0 + knotSpan(p0, p1);
    const float t2 = t1 + knotSpan(p1, p2);
    const float t3 = t2 + knotSpan(p2, p3);
    const float t = t1 + (t2 - t1) * std::clamp(u, 0.0f, 1.0f);

    const QVector3D a1 = lerpKnot(p0, p1, t0, t1, t);
    const QVector3D a2 = lerpKnot(p1, p2, t1, t2, t);
    const QVector3D a3 = lerpKnot(p2, p3, t2, t3, t);
    const QVector3D b1 = lerpKnot(a1, a2, t0, t2, t);
    const QVector3D b2 = lerpKnot(a2, a3, t1, t3, t);
    return lerpKnot(b1, b2, t1, t2, t);
}
}

CurvePath::CurvePath(QObject* parent)
    : QObject(parent)
{
    resetToDefault();
}

void CurvePath::resetToDefault()
{
    m_points = {
        QVector3D(0.0f, 0.0f, 0.0f),
        QVector3D(1.2f, 0.3f, 0.6f),
        QVector3D(1.8f, 1.5f, 1.5f),
        QVector3D(0.4f, 2.5f, 0.7f),
        QVector3D(-1.0f, 1.8f, 1.6f),
        QVector3D(-1.3f, 0.4f, 0.4f),
    };
    m_closed = false;
    m_selected = 0;
    invalidateSamples();
    emit curveChanged();
    emit selectionChanged(m_selected);
}

void CurvePath::setSelectedIndex(int index)
{
    if (m_points.isEmpty()) {
        index = -1;
    } else {
        index = std::clamp(index, -1, int(m_points.size()) - 1);
    }
    if (m_selected == index) {
        return;
    }
    m_selected = index;
    emit selectionChanged(m_selected);
}

void CurvePath::setPoint(int index, const QVector3D& position)
{
    if (index < 0 || index >= m_points.size() || m_points.at(index) == position) {
        return;
    }
    m_points[index] = position;
    invalidateSamples();
    emit curveChanged();
}

void CurvePath::addPoint(const QVector3D& position)
{
    m_points.push_back(position);
    m_selected = int(m_points.size()) - 1;
    invalidateSamples();
    emit curveChanged();
    emit selectionChanged(m_selected);
}

void CurvePath::insertPoint(int index, const QVector3D& position)
{
    index = std::clamp(index, 0, int(m_points.size()));
    m_points.insert(index, position);
    m_selected = index;
    invalidateSamples();
    emit curveChanged();
    emit selectionChanged(m_selected);
}

void CurvePath::removePoint(int index)
{
    if (index < 0 || index >= m_points.size() || m_points.size() <= 1) {
        return;
    }
    m_points.removeAt(index);
    m_selected = std::min(index, int(m_points.size()) - 1);
    invalidateSamples();
    emit curveChanged();
    emit selectionChanged(m_selected);
}

void CurvePath::setClosed(bool closed)
{
    if (m_closed == closed) {
        return;
    }
    m_closed = closed;
    invalidateSamples();
    emit curveChanged();
}

int CurvePath::segmentCount() const
{
    const int count = int(m_points.size());
    if (m_closed && count >= 3) {
        return count;
    }
    return std::max(0, count - 1);
}

QVector3D CurvePath::pointAt(int index) const
{
    const int count = int(m_points.size());
    if (count == 0) {
        return {};
    }
    if (m_closed && count >= 3) {
        int wrapped = index % count;
        if (wrapped < 0) {
            wrapped += count;
        }
        return m_points.at(wrapped);
    }
    if (index < 0) {
        return m_points.first();
    }
    if (index >= count) {
        return m_points.last();
    }
    return m_points.at(index);
}

QVector3D CurvePath::segmentPoint(int segment, float u) const
{
    if (segment < 0 || segment >= segmentCount()) {
        return m_points.isEmpty() ? QVector3D() : m_points.first();
    }
    return centripetalPoint(pointAt(segment - 1), pointAt(segment), pointAt(segment + 1), pointAt(segment + 2), u);
}

const QList<CurvePath::Sample>& CurvePath::samples() const
{
    if (m_samplesDirty) {
        rebuildSamples();
    }
    return m_samples;
}

double CurvePath::arcLength() const
{
    if (m_samplesDirty) {
        rebuildSamples();
    }
    return m_arcLength;
}

QVector3D CurvePath::positionAtFraction(double fraction) const
{
    const QList<Sample>& curve = samples();
    if (curve.isEmpty()) {
        return {};
    }
    if (curve.size() == 1 || m_arcLength <= 1.0e-8) {
        return curve.first().position;
    }

    const double distance = std::clamp(fraction, 0.0, 1.0) * m_arcLength;
    if (distance <= 0.0) {
        return curve.first().position;
    }
    if (distance >= m_arcLength) {
        return curve.last().position;
    }

    int low = 0;
    int high = int(curve.size()) - 1;
    while (low < high) {
        const int mid = (low + high) / 2;
        if (curve.at(mid).distance < distance) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    if (low <= 0) {
        return curve.first().position;
    }

    const Sample& previous = curve.at(low - 1);
    const Sample& next = curve.at(low);
    const double span = next.distance - previous.distance;
    const float weight = span > 1.0e-8 ? float((distance - previous.distance) / span) : 0.0f;
    return previous.position * (1.0f - weight) + next.position * weight;
}

QJsonArray CurvePath::toJson() const
{
    QJsonArray array;
    for (const QVector3D& point : m_points) {
        QJsonArray item;
        item.append(double(point.x()));
        item.append(double(point.y()));
        item.append(double(point.z()));
        array.append(item);
    }
    return array;
}

void CurvePath::fromJson(const QJsonArray& points, bool closed)
{
    QList<QVector3D> loaded;
    for (const QJsonValue& value : points) {
        const QJsonArray item = value.toArray();
        if (item.size() < 3) {
            continue;
        }
        loaded.push_back(QVector3D(float(item.at(0).toDouble()), float(item.at(1).toDouble()), float(item.at(2).toDouble())));
    }
    if (loaded.isEmpty()) {
        resetToDefault();
        return;
    }
    m_points = loaded;
    m_closed = closed;
    m_selected = 0;
    invalidateSamples();
    emit curveChanged();
    emit selectionChanged(m_selected);
}

void CurvePath::invalidateSamples()
{
    m_samplesDirty = true;
}

void CurvePath::rebuildSamples() const
{
    m_samples.clear();
    m_arcLength = 0.0;
    m_samplesDirty = false;
    if (m_points.isEmpty()) {
        return;
    }
    if (segmentCount() == 0) {
        m_samples.push_back(Sample{m_points.first(), 0.0});
        return;
    }

    constexpr int kSteps = 24;
    QVector3D previous;
    bool hasPrevious = false;
    const int segments = segmentCount();
    for (int segment = 0; segment < segments; ++segment) {
        for (int step = 0; step <= kSteps; ++step) {
            if (segment > 0 && step == 0) {
                continue;
            }
            const QVector3D position = segmentPoint(segment, float(step) / float(kSteps));
            if (hasPrevious) {
                m_arcLength += double((position - previous).length());
            }
            m_samples.push_back(Sample{position, m_arcLength});
            previous = position;
            hasPrevious = true;
        }
    }
}
