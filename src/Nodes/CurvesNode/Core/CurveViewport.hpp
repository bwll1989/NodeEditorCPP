#pragma once

#include "CurvePath.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPointer>

// 三维曲线视口。交互对齐 CurveEditor：右键旋转、中键平移、滚轮缩放、拖动控制点、轴向约束、视角球。
class CurveViewport : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit CurveViewport(QWidget* parent = nullptr);
    ~CurveViewport() override;

    /** frame=false 时不重置相机（多曲线切换保持当前视角）。 */
    void setPath(CurvePath* path, bool frame = true);
    void setPlaybackFraction(double fraction);
    void frameAll(bool resetAngles);
    void releaseResources();

    /**
     * 幽灵曲线叠加：以暗色线绘制这些路径（不参与拾取/编辑/播放头），
     * 用于多曲线编辑时同时显示全部曲线。主路径仍由 setPath 指定并高亮。
     * 列表存 QPointer，路径销毁后自动跳过。
     */
    void setOverlayPaths(const QList<CurvePath*>& paths);
    QList<CurvePath*> overlayPaths() const;

    /** 多曲线模式：一次设置全部曲线与选中项（保持当前相机视角，对齐 Curve1DEditor）。 */
    void setPaths(const QList<CurvePath*>& paths);
    void setActiveIndex(int index);
    int activeIndex() const { return m_activeIndex; }

    /** 只读预览模式：屏蔽所有鼠标/键盘交互（平移、旋转、缩放、拖点、加点、删除），仅渲染。 */
    void setReadOnly(bool readOnly);
    bool isReadOnly() const { return m_readOnly; }

    enum class AxisLock
    {
        None,
        X,
        Y,
        Z
    };

signals:
    /** 编辑器内切换了编辑目标曲线（图例/曲线点选），模型据此同步。 */
    void activeIndexChanged(int index);

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
    void appendOverlayCurves(Mesh* mesh) const;
    void appendCurve(Mesh* mesh) const;
    void appendPoint(Mesh* mesh, const QVector3D& position, const QVector3D& color, float pixelSize) const;
    void appendAxes(Mesh* mesh, const QVector3D& origin, float length) const;
    void drawMesh(const Mesh& mesh, const QMatrix4x4& mvp, float lineWidth = 1.0f);
    void paintNavigationGizmo(QPainter* painter) const;
    void paintReadout(QPainter* painter) const;

    bool projectToScreen(const QVector3D& world, QPointF* screen) const;
    int pickPoint(const QPoint& position) const;
    AxisLock pickAxis(const QPoint& position) const;
    bool pickCurve(const QPoint& position, int* insertIndex, QVector3D* point) const;
    int pickCurveIndex(const QPoint& position) const;   // 命中任意曲线（点选切换用）
    int legendAt(const QPoint& position) const;
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
    void paintLegend(QPainter* painter);
    void applyActiveSelection();                        // 把 m_activeIndex 应用为主路径 + 幽灵集

    CurvePath* m_path = nullptr;
    QList<QPointer<CurvePath>> m_overlayPaths;   // 幽灵曲线（暗色叠加，不参与交互）
    QList<QPointer<CurvePath>> m_paths;          // 全部曲线（图例与点选切换用）
    int m_activeIndex = -1;
    QList<QRect> m_legendRects;                  // 图例命中区（paint 时更新，控件坐标）
    double m_fraction = 0.0;
    bool m_readOnly = false;

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
