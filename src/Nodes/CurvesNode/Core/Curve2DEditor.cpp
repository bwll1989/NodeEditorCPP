#include "Curve2DEditor.hpp"
#include "CurvePalette.hpp"

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
}

Curve2DEditor::Curve2DEditor(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    setMinimumHeight(240);
    setToolTip(QStringLiteral("左键拖动控制点；单击曲线或图例切换编辑目标；双击曲线插入点、双击空白末尾加点；中键或右键平移，滚轮缩放，Delete 删除选中点"));
}

void Curve2DEditor::setPath(CurvePath* path, bool frame)
{
    if (m_path == path) {
        return;
    }
    if (m_path) {
        disconnect(m_path, nullptr, this, nullptr);
    }
    m_path = path;
    if (m_path) {
        connect(m_path, &CurvePath::curveChanged, this, qOverload<>(&QWidget::update));
        connect(m_path, &CurvePath::selectionChanged, this, qOverload<>(&QWidget::update));
    }
    if (frame) {
        frameAll();
    } else {
        update();
    }
}

void Curve2DEditor::setOverlayPaths(const QList<CurvePath*>& paths)
{
    for (const QPointer<CurvePath>& old : m_overlays) {
        if (old && old != m_path) {
            disconnect(old, nullptr, this, nullptr);
        }
    }
    m_overlays.clear();
    for (CurvePath* path : paths) {
        if (!path || path == m_path) {
            continue;   // 主路径自行高亮渲染，不进幽灵列表
        }
        m_overlays.append(path);
        connect(path, &CurvePath::curveChanged, this, qOverload<>(&QWidget::update));
    }
    update();
}

void Curve2DEditor::setPlaybackFraction(double fraction)
{
    fraction = std::clamp(fraction, 0.0, 1.0);
    if (std::abs(m_fraction - fraction) < 1.0e-6) {
        return;
    }
    m_fraction = fraction;
    update();
}

/** 把 m_activeIndex 应用为主路径 + 幽灵集（setPaths/setActiveIndex 共用）。 */
void Curve2DEditor::applyActiveSelection()
{
    CurvePath* active = (m_activeIndex >= 0 && m_activeIndex < m_paths.size())
                            ? m_paths.at(m_activeIndex).data() : nullptr;
    setPath(active, false);   // 保持当前视图
    QList<CurvePath*> overlays;
    for (int i = 0; i < m_paths.size(); ++i) {
        if (i != m_activeIndex && m_paths.at(i)) {
            overlays.append(m_paths.at(i).data());
        }
    }
    setOverlayPaths(overlays);
}

void Curve2DEditor::setPaths(const QList<CurvePath*>& paths)
{
    m_paths.clear();
    for (CurvePath* path : paths) {
        if (path) {
            m_paths.append(path);
        }
    }
    if (m_activeIndex >= m_paths.size() || (m_activeIndex < 0 && !m_paths.isEmpty())) {
        m_activeIndex = m_paths.isEmpty() ? -1 : 0;
    }
    m_hoverIndex = -1;
    applyActiveSelection();
    update();
}

void Curve2DEditor::setActiveIndex(int index)
{
    index = (index >= 0 && index < m_paths.size()) ? index : -1;
    if (m_activeIndex == index) {
        return;
    }
    m_activeIndex = index;
    m_hoverIndex = -1;
    applyActiveSelection();
    update();
    Q_EMIT activeIndexChanged(m_activeIndex);
}

void Curve2DEditor::frameAll()
{
    // 包围盒合并主路径与全部幽灵曲线，取景覆盖所有曲线
    QList<QVector3D> allPoints;
    if (m_path) {
        allPoints += m_path->points();
    }
    for (const QPointer<CurvePath>& overlay : m_overlays) {
        if (overlay) {
            allPoints += overlay->points();
        }
    }
    if (allPoints.isEmpty()) {
        m_center = QPointF(0.0, 0.0);
        m_zoom = 60.0;
        update();
        return;
    }
    QVector3D minPoint = allPoints.first();
    QVector3D maxPoint = minPoint;
    for (const QVector3D& position : allPoints) {
        minPoint.setX(std::min(minPoint.x(), position.x()));
        minPoint.setY(std::min(minPoint.y(), position.y()));
        maxPoint.setX(std::max(maxPoint.x(), position.x()));
        maxPoint.setY(std::max(maxPoint.y(), position.y()));
    }
    const QPointF span(std::max(1.0, double(maxPoint.x() - minPoint.x())),
                       std::max(1.0, double(maxPoint.y() - minPoint.y())));
    const double availW = std::max(1.0, double(width()) - 90.0);
    const double availH = std::max(1.0, double(height()) - 70.0);
    m_zoom = std::clamp(std::min(availW / span.x(), availH / span.y()), 4.0, 4000.0);
    m_center = QPointF((minPoint.x() + maxPoint.x()) * 0.5,
                       (minPoint.y() + maxPoint.y()) * 0.5);
    m_framed = true;
    update();
}

QPointF Curve2DEditor::worldToScreen(const QVector3D& world) const
{
    // Y 向上：屏幕 y 与世界 y 反向
    return QPointF(double(width()) * 0.5 + (double(world.x()) - m_center.x()) * m_zoom,
                   double(height()) * 0.5 - (double(world.y()) - m_center.y()) * m_zoom);
}

QVector3D Curve2DEditor::screenToWorld(const QPointF& screen) const
{
    const double wx = m_center.x() + (screen.x() - double(width()) * 0.5) / m_zoom;
    const double wy = m_center.y() - (screen.y() - double(height()) * 0.5) / m_zoom;
    return QVector3D(float(wx), float(wy), 0.0f);
}

int Curve2DEditor::pickPoint(const QPoint& position) const
{
    if (!m_path) {
        return -1;
    }
    const QList<QVector3D>& points = m_path->points();
    for (int i = 0; i < points.size(); ++i) {
        if ((worldToScreen(points.at(i)) - QPointF(position)).manhattanLength() < 12) {
            return i;
        }
    }
    return -1;
}

bool Curve2DEditor::pickCurve(const QPoint& position, int* insertIndex, QVector3D* hit) const
{
    if (!m_path || m_path->segmentCount() <= 0) {
        return false;
    }
    // 按控制点段（segment）拾取：命中后插入位置 = 段末控制点索引（segment + 1）。
    // 不能用 samples() 的扁平索引——那是采样点序号（每段 24 步），传给 insertPoint
    // 会被 clamp 到控制点末尾，表现为"双击任意位置都加到末尾"。
    const QPointF click(position);
    int bestSegment = -1;
    double bestDistance = 10.0;
    float bestU = 0.0f;
    constexpr int kSteps = 24;   // 与 CurvePath::rebuildSamples 的细分密度一致
    for (int segment = 0; segment < m_path->segmentCount(); ++segment) {
        QPointF previous = worldToScreen(m_path->segmentPoint(segment, 0.0f));
        for (int step = 1; step <= kSteps; ++step) {
            const QPointF current = worldToScreen(m_path->segmentPoint(segment, float(step) / float(kSteps)));
            const QPointF ab = current - previous;
            const double ab2 = QPointF::dotProduct(ab, ab);
            const double t = ab2 > 0.0 ? std::clamp(QPointF::dotProduct(click - previous, ab) / ab2, 0.0, 1.0) : 0.0;
            const double distance = (click - (previous + ab * t)).manhattanLength();
            if (distance < bestDistance) {
                bestDistance = distance;
                bestSegment = segment;
                bestU = float(step - 1 + t) / float(kSteps);
            }
            previous = current;
        }
    }
    if (bestSegment < 0) {
        return false;
    }
    if (hit) {
        *hit = m_path->segmentPoint(bestSegment, bestU);   // 样条上的精确命中点
    }
    if (insertIndex) {
        *insertIndex = bestSegment + 1;
    }
    return true;
}

/** 命中任意曲线（含幽灵线）：返回在全部曲线中的索引，未命中 -1（点选切换用）。 */
int Curve2DEditor::pickCurveIndex(const QPoint& position) const
{
    for (int i = 0; i < m_paths.size(); ++i) {
        CurvePath* path = m_paths.at(i).data();
        if (!path) {
            continue;
        }
        const QList<CurvePath::Sample>& samples = path->samples();
        for (int s = 1; s < samples.size(); ++s) {
            const QPointF a = worldToScreen(samples.at(s - 1).position);
            const QPointF b = worldToScreen(samples.at(s).position);
            const QPointF ab = b - a;
            const double ab2 = QPointF::dotProduct(ab, ab);
            const double t = ab2 > 0.0 ? std::clamp(QPointF::dotProduct(QPointF(position) - a, ab) / ab2, 0.0, 1.0) : 0.0;
            if ((QPointF(position) - (a + ab * t)).manhattanLength() <= 8.0) {
                return i;
            }
        }
    }
    return -1;
}

int Curve2DEditor::legendAt(const QPoint& position) const
{
    for (int i = 0; i < m_legendRects.size() && i < m_paths.size(); ++i) {
        if (m_legendRects.at(i).contains(position) && m_paths.at(i)) {
            return i;
        }
    }
    return -1;
}

/** 左上角图例：色带 + 曲线名，点击行切换编辑目标（对齐 Curve1DEditor）。 */
void Curve2DEditor::drawLegend(QPainter* painter)
{
    m_legendRects.clear();
    if (m_paths.isEmpty()) {
        return;
    }
    QFont font = painter->font();
    font.setPointSizeF(8.5);
    painter->setFont(font);
    const QFontMetrics fm(painter->fontMetrics());

    int y = 8;
    for (int i = 0; i < m_paths.size(); ++i) {
        CurvePath* path = m_paths.at(i).data();
        if (!path) {
            m_legendRects.append(QRect());
            continue;
        }
        const bool active = (i == m_activeIndex);
        const QColor color = curvePaletteColor(i);
        const QString name = QStringLiteral("曲线 %1").arg(i + 1);
        const int nameW = fm.horizontalAdvance(name);
        const int rowH = fm.height() + 6;
        const int rowW = 4 + 6 + nameW + 8;
        const QRect rowRect(10, y, rowW, rowH);

        if (active) {
            painter->fillRect(rowRect, QColor(255, 255, 255, 24));
        }
        painter->fillRect(QRect(rowRect.left(), rowRect.top() + 3, 4, rowH - 6), color);
        painter->setPen(active ? rgb(238, 241, 247) : rgb(178, 184, 196));
        painter->drawText(QPoint(rowRect.left() + 10, rowRect.top() + fm.ascent() + 3), name);

        m_legendRects.append(rowRect);
        y += rowH + 2;
    }
}

void Curve2DEditor::drawWorldGrid(QPainter* painter) const
{
    // 自适应网格步长（1-2-5 序列），保证相邻网格线间距在 36~160px
    double gridStep = 1.0;
    bool found = false;
    for (int e = -3; e <= 6 && !found; ++e) {
        const double base = std::pow(10.0, e);
        for (int m : {1, 2, 5}) {
            const double s = base * m;
            if (s * m_zoom >= 36.0 && s * m_zoom <= 160.0) {
                gridStep = s;
                found = true;
                break;
            }
        }
    }
    if (!found) {
        gridStep = std::max(1.0, 50.0 / std::max(1.0, m_zoom));
    }

    const QVector3D topLeft = screenToWorld(QPointF(0, 0));
    const QVector3D bottomRight = screenToWorld(QPointF(width(), height()));
    const double x0 = std::floor(topLeft.x() / gridStep) * gridStep;
    const double y0 = std::floor(bottomRight.y() / gridStep) * gridStep;

    QPen minorPen(rgb(40, 45, 54));
    minorPen.setWidth(1);
    QPen majorPen(rgb(54, 60, 72));
    majorPen.setWidth(1);
    painter->setBrush(Qt::NoBrush);

    int index = int(std::round(x0 / gridStep));
    for (double x = x0; x <= bottomRight.x(); x += gridStep, ++index) {
        painter->setPen(std::abs(index) % 5 == 0 ? majorPen : minorPen);
        const QPointF from = worldToScreen(QVector3D(float(x), topLeft.y(), 0));
        const QPointF to = worldToScreen(QVector3D(float(x), bottomRight.y(), 0));
        painter->drawLine(from, to);
    }
    index = int(std::round(y0 / gridStep));
    for (double y = y0; y <= topLeft.y(); y += gridStep, ++index) {
        painter->setPen(std::abs(index) % 5 == 0 ? majorPen : minorPen);
        const QPointF from = worldToScreen(QVector3D(topLeft.x(), float(y), 0));
        const QPointF to = worldToScreen(QVector3D(bottomRight.x(), float(y), 0));
        painter->drawLine(from, to);
    }

    // 世界坐标轴（X 红 / Y 绿，与三维视口语义一致）
    QPen axisPen(rgb(150, 70, 70));
    axisPen.setWidth(2);
    painter->setPen(axisPen);
    painter->drawLine(worldToScreen(QVector3D(float(topLeft.x()), 0, 0)),
                      worldToScreen(QVector3D(float(bottomRight.x()), 0, 0)));
    axisPen.setColor(rgb(70, 130, 90));
    painter->setPen(axisPen);
    painter->drawLine(worldToScreen(QVector3D(0, float(bottomRight.y()), 0)),
                      worldToScreen(QVector3D(0, float(topLeft.y()), 0)));
}

void Curve2DEditor::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), rgb(26, 28, 33));
    drawWorldGrid(&painter);
    if (!m_path) {
        drawLegend(&painter);
        return;
    }

    // 幽灵曲线：各自调色板暗色细线（与图例色带对应），仅形状提示
    for (const QPointer<CurvePath>& overlay : m_overlays) {
        if (!overlay || overlay == m_path) {
            continue;
        }
        const int globalIndex = m_paths.indexOf(overlay);
        QPen ghostPen(globalIndex >= 0 ? curveGhostColor(globalIndex) : QColor(76, 82, 96));
        ghostPen.setWidth(1);
        painter.setPen(ghostPen);
        const QList<CurvePath::Sample>& samples = overlay->samples();
        if (samples.size() >= 2) {
            for (int i = 1; i < samples.size(); ++i) {
                painter.drawLine(worldToScreen(samples.at(i - 1).position),
                                 worldToScreen(samples.at(i).position));
            }
        }
        if (overlay->isClosed() && overlay->points().size() >= 3) {
            painter.drawLine(worldToScreen(overlay->points().last()),
                             worldToScreen(overlay->points().first()));
        }
    }

    // 控制点折线辅助（暗灰）
    const QList<QVector3D>& points = m_path->points();
    QPen polygonPen(rgb(70, 78, 92));
    polygonPen.setWidth(1);
    painter.setPen(polygonPen);
    for (int i = 1; i < points.size(); ++i) {
        painter.drawLine(worldToScreen(points.at(i - 1)), worldToScreen(points.at(i)));
    }
    if (m_path->isClosed() && points.size() >= 3) {
        painter.drawLine(worldToScreen(points.last()), worldToScreen(points.first()));
    }

    // 主曲线：按播放进度染色（已播橙 / 未播蓝），3px 突出
    const QList<CurvePath::Sample>& samples = m_path->samples();
    if (samples.size() >= 2) {
        const QColor playedColor = rgb(255, 184, 40);
        const QColor pendingColor = rgb(96, 186, 255);
        const double playDistance = m_fraction * m_path->arcLength();
        QPen curvePen(pendingColor);
        curvePen.setWidth(3);
        curvePen.setCapStyle(Qt::RoundCap);
        painter.setPen(curvePen);
        QVector3D previous = samples.first().position;
        for (int i = 1; i < samples.size(); ++i) {
            const CurvePath::Sample& sample = samples.at(i);
            const QVector3D current = sample.position;
            if (sample.distance <= playDistance) {
                curvePen.setColor(playedColor);
            } else if (samples.at(i - 1).distance >= playDistance) {
                curvePen.setColor(pendingColor);
            } else {
                const double span = sample.distance - samples.at(i - 1).distance;
                const float weight = span > 1.0e-8 ? float((playDistance - samples.at(i - 1).distance) / span) : 0.0f;
                const QVector3D tip = previous * (1.0f - weight) + current * weight;
                curvePen.setColor(playedColor);
                painter.drawLine(worldToScreen(previous), worldToScreen(tip));
                curvePen.setColor(pendingColor);
                painter.drawLine(worldToScreen(tip), worldToScreen(current));
                previous = current;
                continue;
            }
            painter.drawLine(worldToScreen(previous), worldToScreen(current));
            previous = current;
        }
    }

    // 播放头：全高竖直参考线 + 亮绿点
    if (!samples.isEmpty()) {
        const QVector3D playhead = m_path->positionAtFraction(m_fraction);
        const QPointF headScreen = worldToScreen(playhead);
        QPen headPen(rgb(64, 220, 150, 140));
        headPen.setWidth(1);
        painter.setPen(headPen);
        painter.drawLine(QPointF(headScreen.x(), 0), QPointF(headScreen.x(), double(height())));
        painter.setPen(Qt::NoPen);
        painter.setBrush(rgb(64, 220, 150));
        painter.drawEllipse(headScreen, 5.0, 5.0);
    }

    // 控制点（白 / hover 亮 / 选中橙）
    for (int i = 0; i < points.size(); ++i) {
        const QPointF screen = worldToScreen(points.at(i));
        QColor color = rgb(214, 218, 226);
        double radius = 4.0;
        if (i == m_hoverIndex) {
            color = rgb(255, 255, 255);
            radius = 5.5;
        }
        if (i == m_path->selectedIndex()) {
            color = rgb(255, 140, 54);
            radius = 6.5;
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(screen, radius, radius);
    }

    drawLegend(&painter);
}

void Curve2DEditor::mousePressEvent(QMouseEvent* event)
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
        const int legend = legendAt(event->pos());
        if (legend >= 0) {
            setActiveIndex(legend);
            event->accept();
            return;
        }
        if (!m_path) {
            event->ignore();
            return;
        }
        const int index = pickPoint(event->pos());
        if (index >= 0) {
            m_path->setSelectedIndex(index);
            m_draggingPoint = true;
            event->accept();
            return;
        }
        // 单击其它曲线 → 切换编辑目标（对齐 Curve1D）
        const int curveHit = pickCurveIndex(event->pos());
        if (curveHit >= 0 && curveHit != m_activeIndex) {
            setActiveIndex(curveHit);
            event->accept();
            return;
        }
        // 点击空白：取消选中（保留当前视图）
        if (m_path->selectedIndex() >= 0) {
            m_path->setSelectedIndex(-1);
        }
    }
    event->ignore();
}

void Curve2DEditor::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning) {
        const QPointF delta = QPointF(event->pos() - m_lastMouse);
        m_center -= QPointF(delta.x() / m_zoom, -delta.y() / m_zoom);
        m_lastMouse = event->pos();
        update();
        event->accept();
        return;
    }
    if (m_draggingPoint && m_path) {
        const int index = m_path->selectedIndex();
        if (index >= 0 && index < m_path->points().size()) {
            const QVector3D world = screenToWorld(event->pos());
            const QVector3D original = m_path->points().at(index);
            m_path->setPoint(index, QVector3D(float(world.x()), float(world.y()), original.z()));
        }
        event->accept();
        return;
    }
    m_hoverIndex = pickPoint(event->pos());
    setCursor(m_hoverIndex >= 0 ? Qt::CrossCursor : Qt::ArrowCursor);
    update();
    event->ignore();
}

void Curve2DEditor::mouseReleaseEvent(QMouseEvent* event)
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

void Curve2DEditor::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    if (!m_path) {
        // 尚无编辑目标：双击某条曲线直接切换到它
        const int curveHit = pickCurveIndex(event->pos());
        if (curveHit >= 0) {
            setActiveIndex(curveHit);
            event->accept();
        }
        return;
    }
    int insertIndex = -1;
    QVector3D hit;
    if (pickCurve(event->pos(), &insertIndex, &hit)) {
        m_path->insertPoint(insertIndex, hit);
    } else {
        const int curveHit = pickCurveIndex(event->pos());
        if (curveHit >= 0 && curveHit != m_activeIndex) {
            setActiveIndex(curveHit);   // 双击其它曲线：先切换选中，再次双击插点
        } else {
            // 空白双击：末尾加点（z=0）
            const QVector3D world = screenToWorld(event->pos());
            m_path->addPoint(world);
        }
    }
    event->accept();
}

void Curve2DEditor::wheelEvent(QWheelEvent* event)
{
    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    const QVector3D anchor = screenToWorld(event->position());
    m_zoom = std::clamp(m_zoom * factor, 0.5, 8000.0);
    // 保持鼠标下的世界点不动
    const QVector3D after = screenToWorld(event->position());
    m_center += QPointF(double(anchor.x() - after.x()), double(anchor.y() - after.y()));
    update();
    event->accept();
}

void Curve2DEditor::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && m_path) {
        const int index = m_path->selectedIndex();
        if (index >= 0 && index < m_path->points().size()) {
            m_path->removePoint(index);
            event->accept();
            return;
        }
    }
    event->ignore();
}

void Curve2DEditor::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    if (!m_framed) {
        frameAll();
    }
}

void Curve2DEditor::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    if (!m_framed) {
        frameAll();
    }
    update();
}
