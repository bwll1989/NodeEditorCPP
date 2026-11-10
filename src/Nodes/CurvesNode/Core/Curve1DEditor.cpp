#include "Curve1DEditor.hpp"
#include "CurvePalette.hpp"

#include <QFont>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace
{
    QColor rgb(int r, int g, int b, int a = 255)
    {
        return QColor(r, g, b, a);
    }

    /** 1-2-5 序列取整步长（不小于 raw）。 */
    double niceStep(double raw)
    {
        if (!(raw > 0.0)) {
            return 1.0;
        }
        const double base = std::pow(10.0, std::floor(std::log10(raw)));
        for (double m : {1.0, 2.0, 5.0, 10.0}) {
            if (base * m >= raw - 1.0e-12) {
                return base * m;
            }
        }
        return base * 10.0;
    }

    /** 刻度标签小数位（与包络编辑器习惯一致，最少两位）。 */
    int decimalsFor(double step)
    {
        if (step >= 1.0) {
            return 2;
        }
        const int decimals = int(-std::floor(std::log10(step)));
        return std::clamp(decimals, 2, 6);
    }
}

Curve1DEditor::Curve1DEditor(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    setMinimumHeight(240);
    setToolTip(QStringLiteral(
        "左键拖动控制点（横轴=周期内时间，限制在相邻点之间）；单击曲线或图例切换编辑目标；"
        "双击插入点；中键或右键上下平移，滚轮缩放（仅 Y 轴）；Delete 删除选中点。"
        "横轴显示完整周期（在工具行修改周期后自动刷新刻度）"));
}

void Curve1DEditor::setPeriod(double seconds)
{
    seconds = std::clamp(seconds, 0.1, 3600.0);
    if (std::abs(m_period - seconds) < 1.0e-9) {
        return;
    }
    m_period = seconds;
    update();   // X 轴时间刻度随周期换算，立即重绘
}

CurvePath* Curve1DEditor::activePath() const
{
    return (m_activeIndex >= 0 && m_activeIndex < m_paths.size())
               ? m_paths.at(m_activeIndex).data() : nullptr;
}

void Curve1DEditor::setPaths(const QList<CurvePath*>& paths)
{
    for (const QPointer<CurvePath>& old : m_paths) {
        if (old) {
            disconnect(old, nullptr, this, nullptr);
        }
    }
    m_paths.clear();
    for (CurvePath* path : paths) {
        if (!path) {
            continue;
        }
        m_paths.append(path);
        connect(path, &CurvePath::curveChanged, this, qOverload<>(&QWidget::update));
        connect(path, &CurvePath::selectionChanged, this, qOverload<>(&QWidget::update));
    }
    if (m_activeIndex >= m_paths.size()) {
        m_activeIndex = m_paths.size() - 1;
    }
    m_hoverIndex = -1;
    update();
}

void Curve1DEditor::setActiveIndex(int index)
{
    index = (index >= 0 && index < m_paths.size()) ? index : -1;
    if (m_activeIndex == index) {
        return;
    }
    m_activeIndex = index;
    m_hoverIndex = -1;
    update();
    Q_EMIT activeIndexChanged(m_activeIndex);
}

void Curve1DEditor::setPlaybackFraction(double fraction)
{
    fraction = std::clamp(fraction, 0.0, 1.0);
    if (std::abs(m_fraction - fraction) < 1.0e-6) {
        return;
    }
    m_fraction = fraction;
    update();
}

void Curve1DEditor::fitY()
{
    bool has = false;
    double lo = 0.0;
    double hi = 0.0;
    for (const QPointer<CurvePath>& path : m_paths) {
        if (!path) {
            continue;
        }
        const QList<QVector3D>& points = path->points();
        for (const QVector3D& point : points) {
            if (!has) {
                lo = hi = double(point.y());
                has = true;
            } else {
                lo = std::min(lo, double(point.y()));
                hi = std::max(hi, double(point.y()));
            }
        }
    }
    if (!has) {
        lo = 0.0;
        hi = 1.0;
    }
    const double span = hi - lo;
    const double pad = span > 1.0e-9 ? span * 0.12 : 0.5;
    m_valueMin = lo - pad;
    m_valueMax = hi + pad;
    m_framed = true;
    update();
}

QRectF Curve1DEditor::plotRect() const
{
    // 左侧留 Y 轴刻度、底部留 X 轴刻度
    return QRectF(54.0, 10.0,
                  std::max(20.0, double(width()) - 54.0 - 14.0),
                  std::max(20.0, double(height()) - 10.0 - 28.0));
}

QPointF Curve1DEditor::valueToScreen(double progress, double value) const
{
    const QRectF r = plotRect();
    const double span = std::max(1.0e-9, m_valueMax - m_valueMin);
    const double p = std::clamp(progress, 0.0, 1.0);
    return QPointF(r.left() + p * r.width(),
                   r.bottom() - (value - m_valueMin) / span * r.height());
}

QPointF Curve1DEditor::screenToValue(const QPointF& screen) const
{
    const QRectF r = plotRect();
    const double span = std::max(1.0e-9, m_valueMax - m_valueMin);
    const double p = (screen.x() - r.left()) / std::max(1.0, r.width());
    const double v = m_valueMin + (r.bottom() - screen.y()) / std::max(1.0, r.height()) * span;
    return QPointF(p, v);
}

int Curve1DEditor::pickPoint(const QPoint& position) const
{
    CurvePath* path = activePath();
    if (!path) {
        return -1;
    }
    const QList<QVector3D>& points = path->points();
    for (int i = 0; i < points.size(); ++i) {
        const QPointF screen = valueToScreen(double(points.at(i).x()), double(points.at(i).y()));
        if ((screen - QPointF(position)).manhattanLength() < 12) {
            return i;
        }
    }
    return -1;
}

int Curve1DEditor::pickCurve(const QPoint& position) const
{
    if (m_paths.isEmpty()) {
        return -1;
    }
    // 命中检测：优先选中曲线，再按索引遍历其余曲线
    QList<int> order;
    if (m_activeIndex >= 0 && m_activeIndex < m_paths.size()) {
        order.append(m_activeIndex);
    }
    for (int i = 0; i < m_paths.size(); ++i) {
        if (i != m_activeIndex) {
            order.append(i);
        }
    }
    for (int index : order) {
        CurvePath* path = m_paths.at(index).data();
        if (!path) {
            continue;
        }
        // 命中检测用与绘制相同的全跨度折线（较粗采样即可）
        const QVector<QPointF> line = curve1DSamplePolyline(path, int(plotRect().width() / 4.0));
        if (line.size() < 2) {
            continue;
        }
        for (int i = 1; i < line.size(); ++i) {
            const QPointF a = valueToScreen(line.at(i - 1).x(), line.at(i - 1).y());
            const QPointF b = valueToScreen(line.at(i).x(), line.at(i).y());
            const QPointF ab = b - a;
            const double ab2 = QPointF::dotProduct(ab, ab);
            const double t = ab2 > 0.0
                ? std::clamp(QPointF::dotProduct(QPointF(position) - a, ab) / ab2, 0.0, 1.0)
                : 0.0;
            if ((QPointF(position) - (a + ab * t)).manhattanLength() <= 8.0) {
                return index;
            }
        }
    }
    return -1;
}

void Curve1DEditor::drawGridAndAxes(QPainter* painter) const
{
    const QRectF r = plotRect();
    painter->setBrush(Qt::NoBrush);

    // ---- Y 轴（值）：1-2-5 步长，目标约 50px 一格 ----
    const double valueSpan = std::max(1.0e-9, m_valueMax - m_valueMin);
    const int yTargetLines = std::max(2, int(r.height() / 50.0));
    const double yStep = niceStep(valueSpan / double(yTargetLines));
    const int yDecimals = decimalsFor(yStep);
    const double yStart = std::ceil(m_valueMin / yStep) * yStep;
    for (double v = yStart; v <= m_valueMax + 1.0e-9; v += yStep) {
        const double y = valueToScreen(0.0, v).y();
        if (y < r.top() - 0.5 || y > r.bottom() + 0.5) {
            continue;
        }
        QPen gridPen(rgb(255, 255, 255, 16));
        gridPen.setWidth(1);
        painter->setPen(gridPen);
        painter->drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
        // 轴上小刻度 + 左侧标签
        QPen tickPen(rgb(255, 255, 255, 46));
        painter->setPen(tickPen);
        painter->drawLine(QPointF(r.left() - 4.0, y), QPointF(r.left(), y));
        painter->setPen(rgb(190, 196, 206));
        const QFontMetrics fm(painter->fontMetrics());
        const QString label = QString::number(v, 'f', yDecimals);
        painter->drawText(QPointF(r.left() - 8.0 - fm.horizontalAdvance(label),
                                  y + fm.ascent() * 0.5 - 1.0), label);
    }

    // ---- X 轴（时间 = 进度 × 周期，单位秒）：1-2-5 步长，目标约 80px 一格 ----
    // 索引驱动防浮点累加漂移，保证末端 t=周期 的刻度必画
    const double period = std::max(0.1, m_period);
    const double xStep = niceStep(period / std::max(1.0, std::floor(r.width() / 80.0)));
    const int xDecimals = decimalsFor(xStep);
    const double minorStep = xStep / 5.0;
    const int minorCount = int(std::ceil(period / minorStep));
    for (int i = 0; i <= minorCount; ++i) {
        const double t = std::min(double(i) * minorStep, period);
        const double x = valueToScreen(t / period, 0.0).x();
        const bool major = std::abs(std::round(t / xStep) * xStep - t) < 1.0e-6;
        QPen tickPen(rgb(255, 255, 255, major ? 46 : 26));
        painter->setPen(tickPen);
        painter->drawLine(QPointF(x, r.bottom()), QPointF(x, r.bottom() + (major ? 6.0 : 3.0)));
        if (major) {
            QPen gridPen(rgb(255, 255, 255, 16));
            painter->setPen(gridPen);
            painter->drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
            painter->setPen(rgb(190, 196, 206));
            const QFontMetrics fm(painter->fontMetrics());
            const QString label = QString::number(t, 'f', xDecimals);
            painter->drawText(QPointF(x - fm.horizontalAdvance(label) * 0.5,
                                      r.bottom() + 8.0 + fm.ascent()), label);
        }
    }
    // X 轴单位标记（秒）
    painter->setPen(rgb(150, 156, 166));
    const QFontMetrics unitFm(painter->fontMetrics());
    painter->drawText(QPointF(r.right() - unitFm.horizontalAdvance(QStringLiteral("s")),
                              r.bottom() + 8.0 + unitFm.ascent()), QStringLiteral("s"));

    // 坐标轴边线
    QPen axisPen(rgb(255, 255, 255, 40));
    painter->setPen(axisPen);
    painter->drawLine(QPointF(r.left(), r.top()), QPointF(r.left(), r.bottom()));
    painter->drawLine(QPointF(r.left(), r.bottom()), QPointF(r.right(), r.bottom()));
}

void Curve1DEditor::drawLegend(QPainter* painter)
{
    m_legendRects.clear();
    if (m_paths.isEmpty()) {
        return;
    }
    QFont font = painter->font();
    font.setPointSizeF(8.5);
    painter->setFont(font);
    const QFontMetrics fm(painter->fontMetrics());

    const QRectF r = plotRect();
    int y = int(r.top()) + 8;
    for (int i = 0; i < m_paths.size(); ++i) {
        CurvePath* path = m_paths.at(i).data();
        if (!path) {
            m_legendRects.append(QRect());
            continue;
        }
        const bool active = (i == m_activeIndex);
        const QColor color = curveColor(i);
        const QString name = QStringLiteral("曲线 %1").arg(i + 1);
        const QString valueText = QString::number(curve1DValueAt(path, m_fraction), 'f', 3);
        const int nameW = fm.horizontalAdvance(name);
        const int rowH = fm.height() + 6;
        const int rowW = 4 + 6 + nameW + 6 + fm.horizontalAdvance(valueText) + 8;
        QRect rowRect(int(r.left()) + 10, y, rowW, rowH);

        if (active) {
            painter->fillRect(rowRect, QColor(255, 255, 255, 24));
        }
        painter->fillRect(QRect(rowRect.left(), rowRect.top() + 3, 4, rowH - 6), color);
        painter->setPen(active ? rgb(238, 241, 247) : rgb(178, 184, 196));
        painter->drawText(QPoint(rowRect.left() + 10, rowRect.top() + fm.ascent() + 3), name);
        painter->setPen(rgb(color.red(), color.green(), color.blue(), active ? 235 : 170));
        painter->drawText(QPoint(rowRect.left() + 10 + nameW + 6,
                                 rowRect.top() + fm.ascent() + 3), valueText);

        m_legendRects.append(rowRect);
        y += rowH + 2;
    }
}

QColor Curve1DEditor::curveColor(int index)
{
    // 曲线家族统一调色板（CurvePalette.hpp），与 2D/3D 图例/幽灵线一致
    return curvePaletteColor(index);
}

void Curve1DEditor::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), rgb(26, 28, 33));
    drawGridAndAxes(&painter);

    // 播放头：全高竖直参考线（画在曲线下层）
    const QRectF r = plotRect();
    const double headX = valueToScreen(m_fraction, 0.0).x();
    if (headX >= r.left() - 0.5 && headX <= r.right() + 0.5) {
        QPen headPen(rgb(64, 220, 150, 150));
        headPen.setWidth(1);
        painter.setPen(headPen);
        painter.drawLine(QPointF(headX, r.top()), QPointF(headX, r.bottom()));
    }

    // 曲线：先画非选中，选中曲线最后画在最上层；每条曲线独立配色
    QList<int> order;
    for (int i = 0; i < m_paths.size(); ++i) {
        if (i != m_activeIndex && m_paths.at(i)) {
            order.append(i);
        }
    }
    if (m_activeIndex >= 0 && m_activeIndex < m_paths.size() && m_paths.at(m_activeIndex)) {
        order.append(m_activeIndex);
    }

    for (int index : order) {
        CurvePath* path = m_paths.at(index).data();
        const QColor color = curveColor(index);
        const bool active = (index == m_activeIndex);

        // 全跨度单调插值折线（含首末点之外的 hold 平段）——绘制与输出严格一致
        const QVector<QPointF> line = curve1DSamplePolyline(path, int(plotRect().width() / 2.0));
        if (line.size() >= 2) {
            QPen pen(color);
            pen.setWidth(active ? 3 : 2);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);
            for (int i = 1; i < line.size(); ++i) {
                painter.drawLine(valueToScreen(line.at(i - 1).x(), line.at(i - 1).y()),
                                 valueToScreen(line.at(i).x(), line.at(i).y()));
            }
        }

        // 各曲线在播放头处的当前值圆点
        const double value = curve1DValueAt(path, m_fraction);
        const QPointF head = valueToScreen(m_fraction, value);
        painter.setPen(active ? QPen(rgb(255, 255, 255, 190)) : Qt::NoPen);
        painter.setBrush(color);
        const double dotRadius = active ? 4.5 : 3.0;
        painter.drawEllipse(head, dotRadius, dotRadius);

        // 选中曲线的控制点（白 / hover 亮 / 选中橙），控制点在曲线上（插值样条）
        if (active) {
            const QList<QVector3D>& points = path->points();
            for (int i = 0; i < points.size(); ++i) {
                const QPointF screen = valueToScreen(double(points.at(i).x()),
                                                     double(points.at(i).y()));
                QColor dotColor = rgb(214, 218, 226);
                double radius = 4.0;
                if (i == m_hoverIndex) {
                    dotColor = rgb(255, 255, 255);
                    radius = 5.5;
                }
                if (i == path->selectedIndex()) {
                    dotColor = rgb(255, 140, 54);
                    radius = 6.5;
                }
                painter.setPen(Qt::NoPen);
                painter.setBrush(dotColor);
                painter.drawEllipse(screen, radius, radius);
            }
        }
    }

    drawLegend(&painter);
}

void Curve1DEditor::mousePressEvent(QMouseEvent* event)
{
    m_lastMouse = event->pos();
    if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        m_panning = true;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        // 图例行点击 → 切换编辑目标曲线
        for (int i = 0; i < m_legendRects.size() && i < m_paths.size(); ++i) {
            if (m_legendRects.at(i).contains(event->pos()) && m_paths.at(i)) {
                setActiveIndex(i);
                event->accept();
                return;
            }
        }
        const int pointIndex = pickPoint(event->pos());
        if (pointIndex >= 0) {
            if (CurvePath* path = activePath()) {
                path->setSelectedIndex(pointIndex);
            }
            m_draggingPoint = true;
            event->accept();
            return;
        }
        const int curveIndex = pickCurve(event->pos());
        if (curveIndex >= 0 && curveIndex != m_activeIndex) {
            setActiveIndex(curveIndex);
            event->accept();
            return;
        }
        // 空白点击：取消选中点
        if (CurvePath* path = activePath()) {
            if (path->selectedIndex() >= 0) {
                path->setSelectedIndex(-1);
            }
        }
        event->accept();
        return;
    }
    event->ignore();
}

void Curve1DEditor::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning) {
        // 仅 Y 方向平移（X 轴固定 0~1）
        const double dy = double(event->pos().y() - m_lastMouse.y());
        const double scale = (m_valueMax - m_valueMin) / std::max(1.0, plotRect().height());
        m_valueMin += dy * scale;
        m_valueMax += dy * scale;
        m_lastMouse = event->pos();
        update();
        event->accept();
        return;
    }
    if (m_draggingPoint) {
        CurvePath* path = activePath();
        const int index = path ? path->selectedIndex() : -1;
        if (path && index >= 0 && index < path->points().size()) {
            const QPointF value = screenToValue(QPointF(event->pos()));
            const QList<QVector3D>& points = path->points();
            // X = 进度：钳制到 [0,1] 且不越过相邻点（保持 X 升序）
            float x = float(std::clamp(value.x(), 0.0, 1.0));
            const float eps = 0.001f;
            if (index > 0) {
                x = std::max(x, points.at(index - 1).x() + eps);
            }
            if (index < points.size() - 1) {
                x = std::min(x, points.at(index + 1).x() - eps);
            }
            x = std::clamp(x, 0.0f, 1.0f);
            path->setPoint(index, QVector3D(x, float(value.y()), 0.0f));
        }
        event->accept();
        return;
    }
    m_hoverIndex = pickPoint(event->pos());
    setCursor(m_hoverIndex >= 0 ? Qt::CrossCursor : Qt::ArrowCursor);
    update();
    event->ignore();
}

void Curve1DEditor::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_panning && (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton)) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    if (m_draggingPoint && event->button() == Qt::LeftButton) {
        m_draggingPoint = false;
        event->accept();
        return;
    }
    event->ignore();
}

void Curve1DEditor::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    // 双击其它曲线：先切换选中
    if (!activePath()) {
        const int curveIndex = pickCurve(event->pos());
        if (curveIndex >= 0) {
            setActiveIndex(curveIndex);
        }
        event->accept();
        return;
    }
    CurvePath* path = activePath();
    // 已有点上双击 → 选中而非新增
    const int hitExisting = pickPoint(event->pos());
    if (hitExisting >= 0) {
        path->setSelectedIndex(hitExisting);
        event->accept();
        return;
    }
    const QPointF value = screenToValue(QPointF(event->pos()));
    const QList<QVector3D>& points = path->points();
    // 按 X 升序找插入位置（函数语义：任意位置双击即插入该进度的关键点）
    int insertIndex = points.size();
    for (int i = 0; i < points.size(); ++i) {
        if (points.at(i).x() > float(value.x())) {
            insertIndex = i;
            break;
        }
    }
    path->insertPoint(insertIndex,
                      QVector3D(float(std::clamp(value.x(), 0.0, 1.0)),
                                float(value.y()), 0.0f));
    event->accept();
}

void Curve1DEditor::wheelEvent(QWheelEvent* event)
{
    // 仅缩放 Y 轴（X 轴恒为 0~1），以鼠标下的值为锚点
    const double factor = event->angleDelta().y() > 0 ? 1.0 / 1.15 : 1.15;
    const double anchor = screenToValue(event->position()).y();
    m_valueMin = anchor + (m_valueMin - anchor) * factor;
    m_valueMax = anchor + (m_valueMax - anchor) * factor;
    if (m_valueMax - m_valueMin < 1.0e-3) {
        m_valueMin = anchor - 5.0e-4;
        m_valueMax = anchor + 5.0e-4;
    }
    update();
    event->accept();
}

void Curve1DEditor::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (CurvePath* path = activePath()) {
            const int index = path->selectedIndex();
            if (index >= 0 && index < path->points().size()) {
                path->removePoint(index);
                event->accept();
                return;
            }
        }
    }
    event->ignore();
}

void Curve1DEditor::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    if (!m_framed) {
        fitY();
    }
}

void Curve1DEditor::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    if (!m_framed) {
        fitY();
    }
    update();
}
