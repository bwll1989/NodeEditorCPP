#include "CurveViewport.hpp"

#include <QFont>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPainter>
#include <QSurfaceFormat>
#include <QToolTip>
#include <QWheelEvent>

#include <QtMath>

#include <algorithm>
#include <cmath>

namespace
{
const char* kVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uMvp;
out vec3 vColor;
void main() {
    gl_Position = uMvp * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* kFragmentShader = R"(#version 330 core
in vec3 vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 1.0);
}
)";

QVector3D rgb(int r, int g, int b)
{
    return QVector3D(r / 255.0f, g / 255.0f, b / 255.0f);
}

struct NavigationBall
{
    QVector3D direction;
    QPointF center;
    float facing = 0.0f;
    float radius = 0.0f;
    int index = 0;
    QString label;
    QColor color;
};

QString navigationName(int index)
{
    switch (index) {
    case 0: return QStringLiteral("右视图");
    case 1: return QStringLiteral("左视图");
    case 2: return QStringLiteral("前视图");
    case 3: return QStringLiteral("后视图");
    case 4: return QStringLiteral("顶视图");
    case 5: return QStringLiteral("底视图");
    default: return {};
    }
}

QList<NavigationBall> navigationBalls(const QPointF& center, const QVector3D& right, const QVector3D& up, const QVector3D& forward, int hover)
{
    static const QVector3D kDirections[6] = {
        QVector3D(1, 0, 0), QVector3D(-1, 0, 0),
        QVector3D(0, 1, 0), QVector3D(0, -1, 0),
        QVector3D(0, 0, 1), QVector3D(0, 0, -1),
    };
    static const QColor kColors[6] = {
        QColor(214, 64, 64), QColor(122, 48, 48),
        QColor(64, 186, 96), QColor(40, 108, 58),
        QColor(58, 138, 214), QColor(40, 74, 118),
    };
    static const char* kLabels[6] = {"X", "", "Y", "", "Z", ""};
    constexpr float kArm = 34.0f;
    QList<NavigationBall> balls;
    for (int index = 0; index < 6; ++index) {
        NavigationBall ball;
        ball.index = index;
        ball.direction = kDirections[index];
        ball.facing = QVector3D::dotProduct(kDirections[index], -forward);
        const float screenX = QVector3D::dotProduct(kDirections[index], right);
        const float screenY = QVector3D::dotProduct(kDirections[index], up);
        ball.center = QPointF(center.x() + screenX * kArm, center.y() - screenY * kArm);
        const bool positive = kLabels[index][0] != '\0';
        ball.radius = (positive ? 12.0f : 8.5f) + (hover == index ? 1.6f : 0.0f);
        QColor color = kColors[index];
        if (ball.facing < 0.0f) {
            color = color.darker(145);
        }
        if (hover == index) {
            color = color.lighter(125);
        }
        ball.color = color;
        ball.label = QString::fromLatin1(kLabels[index]);
        balls.push_back(ball);
    }
    std::sort(balls.begin(), balls.end(), [](const NavigationBall& a, const NavigationBall& b) {
        return a.facing < b.facing;
    });
    return balls;
}

float screenDistance(const QPointF& a, const QPointF& b)
{
    return std::hypot(float(a.x() - b.x()), float(a.y() - b.y()));
}

QVector3D axisVector(CurveViewport::AxisLock axis)
{
    switch (axis) {
    case CurveViewport::AxisLock::X: return QVector3D(1, 0, 0);
    case CurveViewport::AxisLock::Y: return QVector3D(0, 1, 0);
    case CurveViewport::AxisLock::Z: return QVector3D(0, 0, 1);
    default: return {};
    }
}
}

CurveViewport::CurveViewport(QWidget* parent)
    : QOpenGLWidget(parent)
{
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSamples(4);
    setFormat(format);
    setMinimumHeight(280);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setToolTip(QStringLiteral("左键拖动控制点，按住 X/Y/Z 锁定轴向；右键旋转，中键或 Shift+左键平移，滚轮缩放，双击地面或曲线添加点"));
}

CurveViewport::~CurveViewport()
{
    releaseResources();
}

void CurveViewport::releaseResources()
{
    QOpenGLContext* ctx = context();
    if (ctx) {
        disconnect(ctx, &QOpenGLContext::aboutToBeDestroyed, this, &CurveViewport::releaseResources);
    }
    if (!m_program && !m_vao.isCreated() && !m_vbo.isCreated()) {
        return;
    }
    if (ctx) {
        makeCurrent();
    }
    if (m_vbo.isCreated()) {
        m_vbo.destroy();
    }
    if (m_vao.isCreated()) {
        m_vao.destroy();
    }
    delete m_program;
    m_program = nullptr;
    m_glReady = false;
    m_mvpLocation = -1;
    if (ctx) {
        doneCurrent();
    }
}

void CurveViewport::setPath(CurvePath* path)
{
    if (m_path == path) {
        return;
    }
    if (m_path) {
        disconnect(m_path, nullptr, this, nullptr);
    }
    m_path = path;
    if (m_path) {
        connect(m_path, &CurvePath::curveChanged, this, qOverload<>(&CurveViewport::update));
        connect(m_path, &CurvePath::selectionChanged, this, qOverload<>(&CurveViewport::update));
    }
    frameAll(true);
}

void CurveViewport::setPlaybackFraction(double fraction)
{
    fraction = std::clamp(fraction, 0.0, 1.0);
    if (std::abs(m_fraction - fraction) < 1.0e-6) {
        return;
    }
    m_fraction = fraction;
    update();
}

void CurveViewport::Mesh::addVertex(QList<float>* target, const QVector3D& position, const QVector3D& color)
{
    target->push_back(position.x());
    target->push_back(position.y());
    target->push_back(position.z());
    target->push_back(color.x());
    target->push_back(color.y());
    target->push_back(color.z());
}

void CurveViewport::Mesh::addLine(const QVector3D& a, const QVector3D& b, const QVector3D& color)
{
    addVertex(&lines, a, color);
    addVertex(&lines, b, color);
}

void CurveViewport::Mesh::addTriangle(const QVector3D& a, const QVector3D& b, const QVector3D& c, const QVector3D& color)
{
    addVertex(&triangles, a, color);
    addVertex(&triangles, b, color);
    addVertex(&triangles, c, color);
}

void CurveViewport::Mesh::addQuad(const QVector3D& center, const QVector3D& right, const QVector3D& up, const QVector3D& color)
{
    const QVector3D a = center - right - up;
    const QVector3D b = center + right - up;
    const QVector3D c = center + right + up;
    const QVector3D d = center - right + up;
    addTriangle(a, b, c, color);
    addTriangle(a, c, d, color);
}

void CurveViewport::initializeGL()
{
    if (!initializeOpenGLFunctions()) {
        return;
    }
    m_program = new QOpenGLShaderProgram();
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShader)
        || !m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentShader)
        || !m_program->link()) {
        delete m_program;
        m_program = nullptr;
        return;
    }
    m_mvpLocation = m_program->uniformLocation("uMvp");
    m_vao.create();
    m_vbo.create();
    m_vao.bind();
    m_vbo.bind();
    m_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    m_program->bind();
    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * int(sizeof(float)));
    m_program->enableAttributeArray(1);
    m_program->setAttributeBuffer(1, GL_FLOAT, 3 * int(sizeof(float)), 3, 6 * int(sizeof(float)));
    m_vao.release();
    m_vbo.release();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_CULL_FACE);
    m_glReady = true;
    if (QOpenGLContext* ctx = context()) {
        connect(ctx, &QOpenGLContext::aboutToBeDestroyed, this, &CurveViewport::releaseResources, Qt::DirectConnection);
    }
}

void CurveViewport::resizeGL(int width, int height)
{
    const qreal ratio = devicePixelRatioF();
    glViewport(0, 0, int(width * ratio), int(std::max(height, 1) * ratio));
}

void CurveViewport::paintGL()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.beginNativePainting();
    if (!m_glReady) {
        glClearColor(0.10f, 0.11f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        painter.endNativePainting();
        return;
    }

    updateCamera();
    const qreal ratio = devicePixelRatioF();
    glViewport(0, 0, int(width() * ratio), int(std::max(height(), 1) * ratio));
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(0.10f, 0.11f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mesh mesh;
    appendGrid(&mesh);
    appendCurve(&mesh);
    drawMesh(mesh, m_projection * m_view);
    painter.endNativePainting();
    paintNavigationGizmo(&painter);
    paintReadout(&painter);
}

void CurveViewport::updateCamera()
{
    const float yaw = qDegreesToRadians(m_yaw);
    const float pitch = qDegreesToRadians(m_pitch);
    const QVector3D offset(
        m_distance * std::cos(pitch) * std::cos(yaw),
        m_distance * std::cos(pitch) * std::sin(yaw),
        m_distance * std::sin(pitch));
    m_eye = m_target + offset;
    m_forward = (m_target - m_eye).normalized();
    QVector3D upHint = m_upHint;
    m_right = QVector3D::crossProduct(m_forward, upHint);
    if (m_right.lengthSquared() < 1.0e-8f) {
        upHint = std::abs(m_forward.z()) > 0.9f ? QVector3D(0, 1, 0) : QVector3D(0, 0, 1);
        m_right = QVector3D::crossProduct(m_forward, upHint);
    }
    m_right.normalize();
    m_up = QVector3D::crossProduct(m_right, m_forward).normalized();

    m_view.setToIdentity();
    m_view.lookAt(m_eye, m_target, m_up);
    const float aspect = float(std::max(width(), 1)) / float(std::max(height(), 1));
    const float halfHeight = m_distance * std::tan(qDegreesToRadians(22.5f));
    m_projection.setToIdentity();
    if (m_orthographic) {
        const float halfWidth = halfHeight * aspect;
        m_projection.ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.05f, 800.0f);
    } else {
        m_projection.perspective(45.0f, aspect, 0.05f, 800.0f);
    }
}

void CurveViewport::appendGrid(Mesh* mesh) const
{
    const QVector3D minor = rgb(46, 52, 64);
    const QVector3D major = rgb(68, 76, 92);
    constexpr int kExtent = 20;
    for (int i = -kExtent; i <= kExtent; ++i) {
        const QVector3D color = (i % 5 == 0) ? major : minor;
        mesh->addLine(QVector3D(float(-kExtent), float(i), 0), QVector3D(float(kExtent), float(i), 0), color);
        mesh->addLine(QVector3D(float(i), float(-kExtent), 0), QVector3D(float(i), float(kExtent), 0), color);
    }
    mesh->addLine(QVector3D(-kExtent, 0, 0), QVector3D(kExtent, 0, 0), rgb(196, 86, 86));
    mesh->addLine(QVector3D(0, -kExtent, 0), QVector3D(0, kExtent, 0), rgb(86, 168, 104));
    mesh->addLine(QVector3D(0, 0, 0), QVector3D(0, 0, 3), rgb(86, 146, 214));
}

void CurveViewport::appendPoint(Mesh* mesh, const QVector3D& position, const QVector3D& color, float pixelSize) const
{
    const float distance = std::max(0.05f, (position - m_eye).length());
    const float half = worldPerPixel(distance) * pixelSize * 0.5f;
    mesh->addQuad(position - m_forward * half, m_right * half, m_up * half, color);
}

void CurveViewport::appendAxes(Mesh* mesh, const QVector3D& origin, float length) const
{
    mesh->addLine(origin, origin + QVector3D(length, 0, 0), rgb(224, 96, 96));
    mesh->addLine(origin, origin + QVector3D(0, length, 0), rgb(110, 190, 120));
    mesh->addLine(origin, origin + QVector3D(0, 0, length), rgb(96, 160, 230));
}

void CurveViewport::appendCurve(Mesh* mesh) const
{
    if (!m_path) {
        return;
    }
    const QList<QVector3D>& points = m_path->points();
    const QVector3D polygonColor = rgb(70, 78, 92);
    for (int i = 1; i < points.size(); ++i) {
        mesh->addLine(points.at(i - 1), points.at(i), polygonColor);
    }
    if (m_path->isClosed() && points.size() >= 3) {
        mesh->addLine(points.last(), points.first(), polygonColor);
    }

    const QList<CurvePath::Sample>& samples = m_path->samples();
    if (samples.size() >= 2) {
        const double playDistance = std::clamp(m_fraction, 0.0, 1.0) * m_path->arcLength();
        QVector3D previous = samples.first().position;
        for (int i = 1; i < samples.size(); ++i) {
            const CurvePath::Sample& sample = samples.at(i);
            const QVector3D current = sample.position;
            if (sample.distance <= playDistance) {
                mesh->addLine(previous, current, rgb(240, 186, 78));
            } else if (samples.at(i - 1).distance >= playDistance) {
                mesh->addLine(previous, current, rgb(118, 156, 196));
            } else {
                const double span = sample.distance - samples.at(i - 1).distance;
                const float weight = span > 1.0e-8 ? float((playDistance - samples.at(i - 1).distance) / span) : 0.0f;
                const QVector3D tip = previous * (1.0f - weight) + current * weight;
                mesh->addLine(previous, tip, rgb(240, 186, 78));
                mesh->addLine(tip, current, rgb(118, 156, 196));
            }
            previous = current;
        }
    }

    if (m_draggingPoint && m_dragAxis != AxisLock::None && m_path->selectedIndex() >= 0) {
        const QVector3D anchor = points.at(m_path->selectedIndex());
        const QVector3D direction = axisVector(m_dragAxis);
        QVector3D color = rgb(224, 96, 96);
        if (m_dragAxis == AxisLock::Y) {
            color = rgb(110, 190, 120);
        } else if (m_dragAxis == AxisLock::Z) {
            color = rgb(96, 160, 230);
        }
        mesh->addLine(anchor - direction * 40.0f, anchor + direction * 40.0f, color);
    }

    for (int i = 0; i < points.size(); ++i) {
        QVector3D color = rgb(214, 218, 226);
        float size = 8.0f;
        if (i == m_hoverIndex) {
            color = rgb(255, 255, 255);
            size = 10.0f;
        }
        if (i == m_path->selectedIndex()) {
            color = rgb(255, 140, 54);
            size = 12.0f;
        }
        appendPoint(mesh, points.at(i), color, size);
    }

    const int selected = m_path->selectedIndex();
    if (selected >= 0 && selected < points.size()) {
        const float length = worldPerPixel(std::max(0.05f, (points.at(selected) - m_eye).length())) * 78.0f;
        appendAxes(mesh, points.at(selected), length);
    }

    if (!samples.isEmpty()) {
        appendPoint(mesh, m_path->positionAtFraction(m_fraction), rgb(64, 220, 150), 14.0f);
    }
}

void CurveViewport::drawMesh(const Mesh& mesh, const QMatrix4x4& mvp)
{
    if (!m_program || (mesh.lines.isEmpty() && mesh.triangles.isEmpty())) {
        return;
    }
    QList<float> packed = mesh.lines;
    packed += mesh.triangles;
    m_program->bind();
    m_program->setUniformValue(m_mvpLocation, mvp);
    m_vao.bind();
    m_vbo.bind();
    m_vbo.allocate(packed.constData(), int(packed.size() * sizeof(float)));
    if (!mesh.lines.isEmpty()) {
        glDrawArrays(GL_LINES, 0, int(mesh.lines.size() / 6));
    }
    if (!mesh.triangles.isEmpty()) {
        glDrawArrays(GL_TRIANGLES, int(mesh.lines.size() / 6), int(mesh.triangles.size() / 6));
    }
    m_vbo.release();
    m_vao.release();
    m_program->release();
}

void CurveViewport::paintNavigationGizmo(QPainter* painter) const
{
    const QPointF center = navigationCenter();
    const QList<NavigationBall> balls = navigationBalls(center, m_right, m_up, m_forward, m_navHover);
    painter->setBrush(Qt::NoBrush);
    for (const NavigationBall& ball : balls) {
        QColor line = ball.color;
        line.setAlpha(ball.facing < 0.0f ? 120 : 220);
        painter->setPen(QPen(line, ball.facing < 0.0f ? 1.6 : 2.2, Qt::SolidLine, Qt::RoundCap));
        painter->drawLine(center, ball.center);
    }
    for (const NavigationBall& ball : balls) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(ball.color);
        painter->drawEllipse(ball.center, ball.radius, ball.radius);
        if (!ball.label.isEmpty()) {
            painter->setPen(QColor(255, 255, 255));
            QFont font = painter->font();
            font.setBold(true);
            font.setPixelSize(ball.facing > 0.15f ? 11 : 9);
            painter->setFont(font);
            painter->drawText(QRectF(ball.center.x() - ball.radius, ball.center.y() - ball.radius, ball.radius * 2.0, ball.radius * 2.0),
                Qt::AlignCenter, ball.label);
        }
    }
}

void CurveViewport::paintReadout(QPainter* painter) const
{
    if (!m_path) {
        return;
    }
    const QVector3D position = m_path->positionAtFraction(m_fraction);
    painter->setPen(QColor(210, 214, 222));
    QFont font = painter->font();
    font.setPixelSize(12);
    painter->setFont(font);
    painter->drawText(QRect(10, height() - 28, width() - 20, 20), Qt::AlignLeft | Qt::AlignVCenter,
        QStringLiteral("x %1   y %2   z %3")
            .arg(double(position.x()), 0, 'f', 3)
            .arg(double(position.y()), 0, 'f', 3)
            .arg(double(position.z()), 0, 'f', 3));
}

void CurveViewport::frameAll(bool resetAngles)
{
    if (resetAngles) {
        m_yaw = -55.0f;
        m_pitch = 28.0f;
        m_upHint = QVector3D(0, 0, 1);
    }
    if (!m_path || m_path->points().isEmpty()) {
        m_target = QVector3D(0, 0, 0);
        m_distance = 8.0f;
        update();
        return;
    }
    QVector3D minPoint = m_path->points().first();
    QVector3D maxPoint = minPoint;
    for (const QVector3D& position : m_path->points()) {
        minPoint.setX(std::min(minPoint.x(), position.x()));
        minPoint.setY(std::min(minPoint.y(), position.y()));
        minPoint.setZ(std::min(minPoint.z(), position.z()));
        maxPoint.setX(std::max(maxPoint.x(), position.x()));
        maxPoint.setY(std::max(maxPoint.y(), position.y()));
        maxPoint.setZ(std::max(maxPoint.z(), position.z()));
    }
    m_target = (minPoint + maxPoint) * 0.5f;
    const float radius = std::max(0.6f, (maxPoint - minPoint).length() * 0.5f);
    m_distance = std::clamp(radius / std::tan(qDegreesToRadians(22.5f)) * 1.55f, 0.4f, 300.0f);
    update();
}

void CurveViewport::alignView(const QVector3D& fromDirection)
{
    const QVector3D direction = fromDirection.lengthSquared() > 1.0e-8f ? fromDirection.normalized() : QVector3D(0, -1, 0);
    if (std::abs(direction.z()) > 0.99f) {
        m_yaw = -90.0f;
        m_pitch = direction.z() > 0.0f ? 90.0f : -90.0f;
        m_upHint = QVector3D(0, 1, 0);
    } else {
        m_pitch = qRadiansToDegrees(std::asin(std::clamp(direction.z(), -1.0f, 1.0f)));
        m_yaw = qRadiansToDegrees(std::atan2(direction.y(), direction.x()));
        m_upHint = QVector3D(0, 0, 1);
    }
    update();
}

QPointF CurveViewport::navigationCenter() const
{
    return QPointF(std::max(70.0, double(width()) - 78.0), 74.0);
}

int CurveViewport::navigationAt(const QPoint& position) const
{
    const QList<NavigationBall> balls = navigationBalls(navigationCenter(), m_right, m_up, m_forward, m_navHover);
    int best = -1;
    float bestFacing = -2.0f;
    for (const NavigationBall& ball : balls) {
        if (screenDistance(position, ball.center) <= ball.radius + 3.0f && ball.facing > bestFacing) {
            best = ball.index;
            bestFacing = ball.facing;
        }
    }
    return best;
}

bool CurveViewport::projectToScreen(const QVector3D& world, QPointF* screen) const
{
    if (width() <= 0 || height() <= 0) {
        return false;
    }
    const QVector4D clip = (m_projection * m_view) * QVector4D(world, 1.0f);
    if (clip.w() <= 0.001f) {
        return false;
    }
    const float ndcX = clip.x() / clip.w();
    const float ndcY = clip.y() / clip.w();
    const float ndcZ = clip.z() / clip.w();
    if (ndcZ < -1.0f || ndcZ > 1.0f) {
        return false;
    }
    *screen = QPointF((ndcX * 0.5f + 0.5f) * float(width()), (1.0f - (ndcY * 0.5f + 0.5f)) * float(height()));
    return true;
}

int CurveViewport::pickPoint(const QPoint& position) const
{
    if (!m_path) {
        return -1;
    }
    int best = -1;
    float bestDistance = 14.0f;
    const QList<QVector3D>& points = m_path->points();
    for (int i = 0; i < points.size(); ++i) {
        QPointF screen;
        if (!projectToScreen(points.at(i), &screen)) {
            continue;
        }
        const float distance = screenDistance(screen, QPointF(position));
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

CurveViewport::AxisLock CurveViewport::pickAxis(const QPoint& position) const
{
    if (!m_path || m_path->selectedIndex() < 0 || m_path->selectedIndex() >= m_path->points().size()) {
        return AxisLock::None;
    }
    const QVector3D origin = m_path->points().at(m_path->selectedIndex());
    const float length = worldPerPixel(std::max(0.05f, (origin - m_eye).length())) * 78.0f;
    QPointF start;
    if (!projectToScreen(origin, &start)) {
        return AxisLock::None;
    }
    const QPointF click(position);
    if (screenDistance(click, start) <= 14.0f) {
        return AxisLock::None;
    }
    AxisLock best = AxisLock::None;
    float bestDistance = 10.0f;
    const AxisLock axes[3] = {AxisLock::X, AxisLock::Y, AxisLock::Z};
    for (AxisLock axis : axes) {
        QPointF tip;
        if (!projectToScreen(origin + axisVector(axis) * length, &tip)) {
            continue;
        }
        const QPointF delta = tip - start;
        const float lengthSquared = float(QPointF::dotProduct(delta, delta));
        float along = 0.0f;
        if (lengthSquared > 1.0e-4f) {
            along = std::clamp(float(QPointF::dotProduct(click - start, delta) / lengthSquared), 0.0f, 1.0f);
        }
        const float distance = screenDistance(click, start + delta * along);
        if (along > 0.2f && distance < bestDistance) {
            bestDistance = distance;
            best = axis;
        }
    }
    return best;
}

bool CurveViewport::pickCurve(const QPoint& position, int* insertIndex, QVector3D* point) const
{
    if (!m_path || m_path->segmentCount() <= 0) {
        return false;
    }
    const QPointF click(position);
    int bestSegment = -1;
    float bestDistance = 12.0f;
    float bestU = 0.0f;
    constexpr int kSteps = 16;
    for (int segment = 0; segment < m_path->segmentCount(); ++segment) {
        QPointF previous;
        float previousU = 0.0f;
        bool hasPrevious = false;
        for (int step = 0; step <= kSteps; ++step) {
            const float u = float(step) / float(kSteps);
            QPointF screen;
            if (!projectToScreen(m_path->segmentPoint(segment, u), &screen)) {
                hasPrevious = false;
                continue;
            }
            if (hasPrevious) {
                const QPointF delta = screen - previous;
                const float lengthSquared = float(QPointF::dotProduct(delta, delta));
                float along = 0.0f;
                if (lengthSquared > 1.0e-4f) {
                    along = std::clamp(float(QPointF::dotProduct(click - previous, delta) / lengthSquared), 0.0f, 1.0f);
                }
                const float distance = screenDistance(click, previous + delta * along);
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestSegment = segment;
                    bestU = previousU + (u - previousU) * along;
                }
            }
            previous = screen;
            previousU = u;
            hasPrevious = true;
        }
    }
    if (bestSegment < 0) {
        return false;
    }
    *insertIndex = bestSegment + 1;
    *point = m_path->segmentPoint(bestSegment, bestU);
    return true;
}

bool CurveViewport::rayFrom(const QPoint& position, QVector3D* origin, QVector3D* direction) const
{
    if (width() <= 0 || height() <= 0) {
        return false;
    }
    const float x = (2.0f * float(position.x()) / float(width())) - 1.0f;
    const float y = 1.0f - (2.0f * float(position.y()) / float(height()));
    const QMatrix4x4 inverse = (m_projection * m_view).inverted();
    QVector4D nearPoint = inverse * QVector4D(x, y, -1.0f, 1.0f);
    QVector4D farPoint = inverse * QVector4D(x, y, 1.0f, 1.0f);
    if (std::abs(nearPoint.w()) < 1.0e-6f || std::abs(farPoint.w()) < 1.0e-6f) {
        return false;
    }
    nearPoint /= nearPoint.w();
    farPoint /= farPoint.w();
    *origin = nearPoint.toVector3D();
    *direction = (farPoint.toVector3D() - *origin).normalized();
    return true;
}

bool CurveViewport::intersectPlane(const QVector3D& origin, const QVector3D& direction, const QVector3D& planePoint, const QVector3D& planeNormal, QVector3D* hit) const
{
    const float denominator = QVector3D::dotProduct(direction, planeNormal);
    if (std::abs(denominator) < 1.0e-6f) {
        return false;
    }
    const float t = QVector3D::dotProduct(planePoint - origin, planeNormal) / denominator;
    if (t < 0.0f) {
        return false;
    }
    *hit = origin + direction * t;
    return true;
}

float CurveViewport::worldPerPixel(float distance) const
{
    const float pixels = float(std::max(height(), 1));
    const float depth = m_orthographic ? m_distance : std::max(distance, 0.05f);
    return 2.0f * depth * std::tan(qDegreesToRadians(22.5f)) / pixels;
}

void CurveViewport::applyAxisLock(QVector3D* position) const
{
    if (m_axis == AxisLock::X) {
        position->setY(m_pressPosition.y());
        position->setZ(m_pressPosition.z());
    } else if (m_axis == AxisLock::Y) {
        position->setX(m_pressPosition.x());
        position->setZ(m_pressPosition.z());
    } else if (m_axis == AxisLock::Z) {
        position->setX(m_pressPosition.x());
        position->setY(m_pressPosition.y());
    }
}

bool CurveViewport::dragPosition(const QPoint& position, QVector3D* result) const
{
    QVector3D origin;
    QVector3D direction;
    QVector3D hit;
    if (!rayFrom(position, &origin, &direction)) {
        return false;
    }
    const AxisLock axis = m_dragAxis != AxisLock::None ? m_dragAxis : m_axis;
    if (axis != AxisLock::None) {
        const QVector3D axisDir = axisVector(axis);
        QVector3D planeNormal = QVector3D::crossProduct(axisDir, QVector3D::crossProduct(axisDir, m_forward));
        if (planeNormal.lengthSquared() < 1.0e-8f) {
            planeNormal = QVector3D::crossProduct(axisDir, m_right);
        }
        planeNormal.normalize();
        if (!intersectPlane(origin, direction, m_pressPosition, planeNormal, &hit)) {
            return false;
        }
        const float travel = QVector3D::dotProduct(hit - m_pressPosition, axisDir);
        *result = m_pressPosition + axisDir * (travel - m_pressT);
        return true;
    }
    if (!intersectPlane(origin, direction, m_pressPosition, m_forward, &hit)) {
        return false;
    }
    *result = hit + m_dragOffset;
    return true;
}

void CurveViewport::mousePressEvent(QMouseEvent* event)
{
    updateCamera();
    m_lastMouse = event->pos();
    setFocus(Qt::MouseFocusReason);

    if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ShiftModifier))) {
        m_panning = true;
        setCursor(Qt::SizeAllCursor);
        event->accept();
        return;
    }
    if (event->button() == Qt::RightButton || (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier))) {
        m_orbiting = event->button() != Qt::RightButton;
        m_rightMoved = false;
        m_rightDistance = 0;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && navigationAt(event->pos()) >= 0) {
        static const QVector3D kDirections[6] = {
            QVector3D(1, 0, 0), QVector3D(-1, 0, 0),
            QVector3D(0, 1, 0), QVector3D(0, -1, 0),
            QVector3D(0, 0, 1), QVector3D(0, 0, -1),
        };
        alignView(kDirections[navigationAt(event->pos())]);
        event->accept();
        return;
    }
    if (event->button() != Qt::LeftButton || !m_path) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }

    const AxisLock axis = pickAxis(event->pos());
    const int index = axis == AxisLock::None ? pickPoint(event->pos()) : m_path->selectedIndex();
    if (index < 0) {
        m_path->setSelectedIndex(-1);
        return;
    }
    m_path->setSelectedIndex(index);
    m_draggingPoint = true;
    m_dragAxis = axis;
    m_pressPosition = m_path->points().at(index);
    QVector3D origin;
    QVector3D direction;
    QVector3D hit;
    m_dragOffset = {};
    m_pressT = 0.0f;
    if (rayFrom(event->pos(), &origin, &direction)) {
        if (axis == AxisLock::None && intersectPlane(origin, direction, m_pressPosition, m_forward, &hit)) {
            m_dragOffset = m_pressPosition - hit;
        } else if (axis != AxisLock::None) {
            const QVector3D axisDir = axisVector(axis);
            QVector3D planeNormal = QVector3D::crossProduct(axisDir, QVector3D::crossProduct(axisDir, m_forward));
            if (planeNormal.lengthSquared() < 1.0e-8f) {
                planeNormal = QVector3D::crossProduct(axisDir, m_right);
            }
            planeNormal.normalize();
            if (intersectPlane(origin, direction, m_pressPosition, planeNormal, &hit)) {
                m_pressT = QVector3D::dotProduct(hit - m_pressPosition, axisDir);
            }
        }
    }
    grabMouse();
    setCursor(Qt::SizeAllCursor);
    event->accept();
}

void CurveViewport::mouseMoveEvent(QMouseEvent* event)
{
    updateCamera();
    const QPoint delta = event->pos() - m_lastMouse;
    m_lastMouse = event->pos();

    if (m_panning) {
        const float scale = worldPerPixel(m_distance);
        m_target -= m_right * float(delta.x()) * scale;
        m_target += m_up * float(delta.y()) * scale;
        update();
        return;
    }
    if ((event->buttons() & Qt::RightButton) || m_orbiting) {
        if (event->buttons() & Qt::RightButton) {
            m_rightDistance += std::abs(delta.x()) + std::abs(delta.y());
            m_rightMoved = m_rightDistance >= 4;
        }
        m_upHint = QVector3D(0, 0, 1);
        m_yaw -= float(delta.x()) * 0.35f;
        m_pitch = std::clamp(m_pitch + float(delta.y()) * 0.35f, -89.0f, 89.0f);
        setCursor(Qt::ClosedHandCursor);
        update();
        return;
    }
    if (m_draggingPoint && m_path && m_path->selectedIndex() >= 0) {
        QVector3D position;
        if (dragPosition(event->pos(), &position)) {
            if (m_dragAxis == AxisLock::None) {
                applyAxisLock(&position);
            }
            m_path->setPoint(m_path->selectedIndex(), position);
        }
        return;
    }

    const int navigation = navigationAt(event->pos());
    if (navigation != m_navHover) {
        m_navHover = navigation;
        if (navigation >= 0) {
            QToolTip::showText(event->globalPosition().toPoint(), navigationName(navigation), this);
        } else {
            QToolTip::hideText();
        }
        update();
    }
    if (navigation >= 0) {
        setCursor(Qt::PointingHandCursor);
        return;
    }
    const int hover = pickPoint(event->pos());
    const AxisLock axis = hover < 0 ? pickAxis(event->pos()) : AxisLock::None;
    if (hover != m_hoverIndex) {
        m_hoverIndex = hover;
        update();
    }
    setCursor(hover >= 0 || axis != AxisLock::None ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void CurveViewport::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_draggingPoint && event->button() == Qt::LeftButton) {
        m_draggingPoint = false;
        m_dragAxis = AxisLock::None;
        releaseMouse();
        setCursor(Qt::ArrowCursor);
        update();
    }
    if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && m_panning)) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
    }
    if (event->button() == Qt::RightButton) {
        const bool showMenu = !m_rightMoved && !m_orbiting;
        m_orbiting = false;
        setCursor(Qt::ArrowCursor);
        if (showMenu) {
            showContextMenu(event->pos());
        }
    }
    if (event->button() == Qt::LeftButton) {
        m_orbiting = false;
        m_panning = false;
    }
}

void CurveViewport::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || (event->modifiers() & (Qt::AltModifier | Qt::ShiftModifier)) || !m_path) {
        return;
    }
    updateCamera();
    if (navigationAt(event->pos()) >= 0 || pickPoint(event->pos()) >= 0 || pickAxis(event->pos()) != AxisLock::None) {
        return;
    }
    int insertIndex = 0;
    QVector3D point;
    if (pickCurve(event->pos(), &insertIndex, &point)) {
        m_path->insertPoint(insertIndex, point);
        return;
    }
    addPointOnGround(event->pos());
}

void CurveViewport::wheelEvent(QWheelEvent* event)
{
    const float steps = event->angleDelta().y() / 120.0f;
    m_distance = std::clamp(m_distance * std::pow(0.85f, steps), 0.4f, 300.0f);
    update();
}

void CurveViewport::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_X || event->key() == Qt::Key_Y || event->key() == Qt::Key_Z) {
        m_axis = event->key() == Qt::Key_X ? AxisLock::X : event->key() == Qt::Key_Y ? AxisLock::Y : AxisLock::Z;
        if (m_draggingPoint && m_dragAxis == AxisLock::None) {
            m_dragAxis = m_axis;
        }
        update();
        return;
    }
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && m_path) {
        m_path->removePoint(m_path->selectedIndex());
        return;
    }
    if (event->key() == Qt::Key_F) {
        frameAll(false);
        return;
    }
    QOpenGLWidget::keyPressEvent(event);
}

void CurveViewport::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_X || event->key() == Qt::Key_Y || event->key() == Qt::Key_Z) {
        m_axis = AxisLock::None;
        if (m_draggingPoint) {
            m_dragAxis = AxisLock::None;
        }
        update();
        return;
    }
    QOpenGLWidget::keyReleaseEvent(event);
}

void CurveViewport::addPointOnGround(const QPoint& position)
{
    if (!m_path || pickPoint(position) >= 0) {
        return;
    }
    updateCamera();
    QVector3D origin;
    QVector3D direction;
    QVector3D hit;
    const float planeZ = m_path->selectedIndex() >= 0 ? m_path->points().at(m_path->selectedIndex()).z() : 0.0f;
    if (!rayFrom(position, &origin, &direction) || !intersectPlane(origin, direction, QVector3D(0, 0, planeZ), QVector3D(0, 0, 1), &hit)) {
        return;
    }
    if (hit.length() > 200.0f) {
        return;
    }
    m_path->addPoint(hit);
}

void CurveViewport::showContextMenu(const QPoint& position)
{
    QMenu menu(this);
    QAction* addAction = menu.addAction(QStringLiteral("在地面添加控制点"));
    QAction* deleteAction = menu.addAction(QStringLiteral("删除选中控制点"));
    deleteAction->setEnabled(m_path && m_path->selectedIndex() >= 0 && m_path->points().size() > 1);
    menu.addSeparator();
    QMenu* views = menu.addMenu(QStringLiteral("视角"));
    const auto addView = [this, views](const QString& text, const QVector3D& direction) {
        connect(views->addAction(text), &QAction::triggered, this, [this, direction] { alignView(direction); });
    };
    addView(QStringLiteral("顶视图"), QVector3D(0, 0, 1));
    addView(QStringLiteral("底视图"), QVector3D(0, 0, -1));
    addView(QStringLiteral("前视图"), QVector3D(0, 1, 0));
    addView(QStringLiteral("后视图"), QVector3D(0, -1, 0));
    addView(QStringLiteral("右视图"), QVector3D(1, 0, 0));
    addView(QStringLiteral("左视图"), QVector3D(-1, 0, 0));
    views->addSeparator();
    QAction* projectionAction = views->addAction(m_orthographic ? QStringLiteral("透视") : QStringLiteral("正交"));
    QAction* frameAction = menu.addAction(QStringLiteral("适应视图"));
    QAction* chosen = menu.exec(mapToGlobal(position));
    if (chosen == addAction) {
        addPointOnGround(position);
    } else if (chosen == deleteAction && m_path) {
        m_path->removePoint(m_path->selectedIndex());
    } else if (chosen == projectionAction) {
        m_orthographic = !m_orthographic;
        update();
    } else if (chosen == frameAction) {
        frameAll(false);
    }
}
