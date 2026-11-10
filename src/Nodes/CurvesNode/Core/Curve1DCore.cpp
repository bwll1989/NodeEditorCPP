#include "Curve1DCore.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    /**
     * 单调三次插值（Fritsch–Carlson）的节点切线表。
     * 包络语义的关键：以 X 为参数轴，曲线在关键点之间严格单调、不过冲、不回弯——
     * 参数化 Catmull-Rom 的 X 分量在长短段相邻时会过冲回弯（视觉"钩子"），故弃用。
     */
    struct MonoCurve
    {
        QVector<double> xs;
        QVector<double> ys;
        QVector<double> ms;   // 节点切线 dy/dx
    };

    MonoCurve buildMonoCurve(QList<QVector3D> points)
    {
        // 保险：按 X 升序（normalizeCurve1D 已保证，双击插入/存档路径兜底）
        std::sort(points.begin(), points.end(),
                  [](const QVector3D& a, const QVector3D& b) { return a.x() < b.x(); });
        MonoCurve c;
        const int n = points.size();
        c.xs.reserve(n);
        c.ys.reserve(n);
        for (const QVector3D& p : points) {
            c.xs.append(double(p.x()));
            c.ys.append(double(p.y()));
        }
        c.ms.resize(n);
        if (n == 1) {
            c.ms[0] = 0.0;
            return c;
        }
        // 各段平均斜率（X 重合段记 0 → 垂直跳变）
        QVector<double> delta(std::max(1, n - 1), 0.0);
        for (int i = 0; i + 1 < n; ++i) {
            const double h = c.xs.at(i + 1) - c.xs.at(i);
            delta[i] = h > 1.0e-9 ? (c.ys.at(i + 1) - c.ys.at(i)) / h : 0.0;
        }
        // 端点切线取最外侧段斜率；内点异号/零斜率 → 水平（局部极值），否则调和平均
        c.ms[0] = delta.at(0);
        c.ms[n - 1] = delta.at(n - 2);
        for (int i = 1; i + 1 < n; ++i) {
            if (delta.at(i - 1) * delta.at(i) <= 0.0) {
                c.ms[i] = 0.0;
            } else {
                const double h0 = c.xs.at(i) - c.xs.at(i - 1);
                const double h1 = c.xs.at(i + 1) - c.xs.at(i);
                const double w0 = 2.0 * h1 + h0;
                const double w1 = h1 + 2.0 * h0;
                c.ms[i] = (w0 + w1) / (w0 / delta.at(i - 1) + w1 / delta.at(i));
            }
        }
        // Fritsch–Carlson 限幅：每段两端切线 |m| ≤ 3|δ|，保证段内单调无过冲
        for (int i = 0; i + 1 < n; ++i) {
            const double limit = 3.0 * std::abs(delta.at(i));
            c.ms[i] = std::clamp(c.ms.at(i), -limit, limit);
            c.ms[i + 1] = std::clamp(c.ms.at(i + 1), -limit, limit);
        }
        return c;
    }

    /** Hermite 基函数求值（单调切线保证段内不过端点值）。 */
    double monoEval(const MonoCurve& c, double x)
    {
        const int n = c.xs.size();
        if (n == 0) {
            return 0.0;
        }
        if (n == 1 || x <= c.xs.first()) {
            return c.ys.first();   // 首点之前 hold
        }
        if (x >= c.xs.last()) {
            return c.ys.last();    // 末点之后 hold
        }
        int lo = 0;
        int hi = n - 1;
        while (lo + 1 < hi) {      // 二分找 xs[lo] <= x < xs[lo+1]
            const int mid = (lo + hi) / 2;
            if (c.xs.at(mid) <= x) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        const double h = c.xs.at(lo + 1) - c.xs.at(lo);
        if (h <= 1.0e-9) {
            return c.ys.at(lo + 1);   // 垂直跳变：取后值
        }
        const double t = (x - c.xs.at(lo)) / h;
        const double t2 = t * t;
        const double t3 = t2 * t;
        const double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
        const double h10 = t3 - 2.0 * t2 + t;
        const double h01 = -2.0 * t3 + 3.0 * t2;
        const double h11 = t3 - t2;
        return h00 * c.ys.at(lo) + h10 * h * c.ms.at(lo)
             + h01 * c.ys.at(lo + 1) + h11 * h * c.ms.at(lo + 1);
    }
}

double curve1DValueAt(CurvePath* path, double progress)
{
    if (!path) {
        return 0.0;
    }
    if (path->points().isEmpty()) {
        return 0.0;
    }
    const MonoCurve mono = buildMonoCurve(path->points());
    return monoEval(mono, std::clamp(progress, 0.0, 1.0));
}

QVector<QPointF> curve1DSamplePolyline(CurvePath* path, int segments)
{
    QVector<QPointF> line;
    if (!path || path->points().isEmpty()) {
        return line;
    }
    const MonoCurve mono = buildMonoCurve(path->points());
    segments = std::max(8, segments);
    line.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        const double p = double(i) / double(segments);
        line.append(QPointF(p, monoEval(mono, p)));
    }
    return line;
}

void normalizeCurve1D(CurvePath* path)
{
    if (!path) {
        return;
    }
    QList<QVector3D> points = path->points();
    if (points.isEmpty()) {
        return;
    }
    std::sort(points.begin(), points.end(),
              [](const QVector3D& a, const QVector3D& b) { return a.x() < b.x(); });
    for (QVector3D& point : points) {
        point.setX(float(std::clamp(double(point.x()), 0.0, 1.0)));
    }
    if (path->isClosed()) {
        path->setClosed(false);   // 函数曲线无闭合语义
    }
    for (int i = 0; i < points.size(); ++i) {
        path->setPoint(i, points.at(i));
    }
}
