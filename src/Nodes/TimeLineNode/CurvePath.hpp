#pragma once

#include <QJsonArray>
#include <QList>
#include <QObject>
#include <QVector3D>

// 三维曲线：控制点 + 向心 Catmull-Rom，按弧长比例取样。算法与 CurveEditor 的位置曲线一致。
class CurvePath : public QObject
{
    Q_OBJECT

public:
    struct Sample
    {
        QVector3D position;
        double distance = 0.0;
    };

    explicit CurvePath(QObject* parent = nullptr);

    const QList<QVector3D>& points() const { return m_points; }
    bool isClosed() const { return m_closed; }
    int selectedIndex() const { return m_selected; }

    void setSelectedIndex(int index);
    void setPoint(int index, const QVector3D& position);
    void addPoint(const QVector3D& position);
    void insertPoint(int index, const QVector3D& position);
    void removePoint(int index);
    void setClosed(bool closed);
    void resetToDefault();

    int segmentCount() const;
    QVector3D segmentPoint(int segment, float u) const;
    const QList<Sample>& samples() const;
    double arcLength() const;
    QVector3D positionAtFraction(double fraction) const;

    QJsonArray toJson() const;
    void fromJson(const QJsonArray& points, bool closed);

signals:
    void curveChanged();
    void selectionChanged(int index);

private:
    QVector3D pointAt(int index) const;
    void invalidateSamples();
    void rebuildSamples() const;

    QList<QVector3D> m_points;
    bool m_closed = false;
    int m_selected = 0;
    mutable bool m_samplesDirty = true;
    mutable QList<Sample> m_samples;
    mutable double m_arcLength = 0.0;
};
