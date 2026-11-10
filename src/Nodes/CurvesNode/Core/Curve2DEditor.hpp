#pragma once

#include "CurvePath.hpp"

#include <QList>
#include <QPointer>
#include <QPoint>
#include <QRect>
#include <QWidget>

/**
 * @brief 二维平面曲线编辑器（QPainter 正交视图，Curve2D 专用；1D 用 Curve1DEditor）。
 *
 * 与三维 CurveViewport 相对：无旋转、无透视，直接以 XY 平面俯视呈现曲线，
 * 世界网格 + 坐标轴 + 进度染色曲线 + 控制点 + 播放头竖直参考线，所见即所得。
 *
 * 多曲线（对齐 Curve1DEditor）：setPaths 一次给全部曲线，左上角图例
 * （色带 + 曲线名）点击切换编辑目标，单击其它曲线同样切换；选中曲线高亮
 * （3px 进度染色），其余为各自调色板暗色幽灵线。activeIndexChanged 通知
 * 模型同步（编辑器 → 模型；模型经 setActiveIndex 回推同值时 no-emit）。
 *
 * 交互：左键拖动控制点、双击选中曲线插入点（双击其它曲线先切换选中）、
 * 双击空白末尾追加、中键或右键平移、滚轮以鼠标为中心缩放、Delete/Backspace
 * 删除选中点。
 * 数据仍为 CurvePath（z 分量不参与显示与输出，新增点 z=0）。
 */
class Curve2DEditor : public QWidget
{
    Q_OBJECT

public:
    explicit Curve2DEditor(QWidget* parent = nullptr);

    /** frame=false 时不重置视图（多曲线切换保持当前平移/缩放）。 */
    void setPath(CurvePath* path, bool frame = true);
    void setOverlayPaths(const QList<CurvePath*>& paths);

    /** 多曲线模式：一次设置全部曲线与选中项（保持当前视图，不重置取景）。 */
    void setPaths(const QList<CurvePath*>& paths);
    void setActiveIndex(int index);
    int activeIndex() const { return m_activeIndex; }

    void setPlaybackFraction(double fraction);
    void frameAll();

signals:
    /** 编辑器内切换了编辑目标曲线（图例/曲线点选），模型据此同步。 */
    void activeIndexChanged(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QPointF worldToScreen(const QVector3D& world) const;
    QVector3D screenToWorld(const QPointF& screen) const;
    int pickPoint(const QPoint& position) const;
    bool pickCurve(const QPoint& position, int* insertIndex, QVector3D* hit) const;
    int pickCurveIndex(const QPoint& position) const;   // 命中任意曲线（点选切换用）
    int legendAt(const QPoint& position) const;
    void drawWorldGrid(QPainter* painter) const;
    void drawLegend(QPainter* painter);
    void applyActiveSelection();                        // 把 m_activeIndex 应用为主路径 + 幽灵集

    CurvePath* m_path = nullptr;
    QList<QPointer<CurvePath>> m_overlays;   // 幽灵曲线（暗色，不参与编辑）
    QList<QPointer<CurvePath>> m_paths;      // 全部曲线（图例与点选切换用）
    int m_activeIndex = -1;
    QList<QRect> m_legendRects;              // 图例命中区（paint 时更新，控件坐标）
    double m_fraction = 0.0;

    QPointF m_center{0.0, 0.0};              // 视图中心（世界坐标）
    double m_zoom = 60.0;                    // 每世界单位的像素数
    bool m_framed = false;                   // 首次显示/设置路径后自动取景一次
    bool m_panning = false;
    bool m_draggingPoint = false;
    int m_hoverIndex = -1;
    QPoint m_lastMouse;
};
