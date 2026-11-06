#pragma once

#include "CurvePath.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

// 三维曲线视口。交互对齐 CurveEditor：右键旋转、中键平移、滚轮缩放、拖动控制点、轴向约束、视角球。
class CurveViewport : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit CurveViewport(QWidget* parent = nullptr);
    ~CurveViewport() override;

    void setPath(CurvePath* path);
    void setPlaybackFraction(double fraction);
    void frameAll(bool resetAngles);
    void releaseResources();

    enum class AxisLock
    {
        None,
        X,
        Y,
        Z
    };

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    struct Mesh
    {
        QList<float> lines;
        QList<float> triangles;
        void addVertex(QList<float>* target, const QVector3D& position, const QVector3D& color);
        void addLine(const QVector3D& a, const QVector3D& b, const QVector3D& color);
        void addTriangle(const QVector3D& a, const QVector3D& b, const QVector3D& c, const QVector3D& color);
        void addQuad(const QVector3D& center, const QVector3D& right, const QVector3D& up, const QVector3D& color);
    };

    void updateCamera();
    void appendGrid(Mesh* mesh) const;
    void appendCurve(Mesh* mesh) const;
    void appendPoint(Mesh* mesh, const QVector3D& position, const QVector3D& color, float pixelSize) const;
    void appendAxes(Mesh* mesh, const QVector3D& origin, float length) const;
    void drawMesh(const Mesh& mesh, const QMatrix4x4& mvp);
    void paintNavigationGizmo(QPainter* painter) const;
    void paintReadout(QPainter* painter) const;

    bool projectToScreen(const QVector3D& world, QPointF* screen) const;
    int pickPoint(const QPoint& position) const;
    AxisLock pickAxis(const QPoint& position) const;
    bool pickCurve(const QPoint& position, int* insertIndex, QVector3D* point) const;
    bool rayFrom(const QPoint& position, QVector3D* origin, QVector3D* direction) const;
    bool intersectPlane(const QVector3D& origin, const QVector3D& direction, const QVector3D& planePoint, const QVector3D& planeNormal, QVector3D* hit) const;
    bool dragPosition(const QPoint& position, QVector3D* result) const;
    float worldPerPixel(float distance) const;
    void applyAxisLock(QVector3D* position) const;
    void alignView(const QVector3D& fromDirection);
    void showContextMenu(const QPoint& position);
    void addPointOnGround(const QPoint& position);
    QPointF navigationCenter() const;
    int navigationAt(const QPoint& position) const;

    CurvePath* m_path = nullptr;
    double m_fraction = 0.0;

    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
    int m_mvpLocation = -1;
    bool m_glReady = false;

    QVector3D m_target{0.2f, 1.0f, 0.7f};
    QVector3D m_eye;
    QVector3D m_forward{0, 1, 0};
    QVector3D m_right{1, 0, 0};
    QVector3D m_up{0, 0, 1};
    QMatrix4x4 m_view;
    QMatrix4x4 m_projection;
    float m_yaw = -55.0f;
    float m_pitch = 28.0f;
    float m_distance = 8.0f;
    QVector3D m_upHint{0, 0, 1};
    bool m_orthographic = false;
    int m_navHover = -1;

    bool m_draggingPoint = false;
    bool m_orbiting = false;
    bool m_panning = false;
    bool m_rightMoved = false;
    int m_rightDistance = 0;
    QPoint m_lastMouse;
    QVector3D m_pressPosition;
    QVector3D m_dragOffset;
    float m_pressT = 0.0f;
    int m_hoverIndex = -1;
    AxisLock m_axis = AxisLock::None;
    AxisLock m_dragAxis = AxisLock::None;
};
