#pragma once

#include "Curve1DCore.hpp"
#include "CurvePath.hpp"

#include <QList>
#include <QPointer>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QWidget>

// 核心采样算法（curve1DValueAt / curve1DSamplePolyline / normalizeCurve1D）
// 在 Curve1DCore.hpp（纯数据层，无 GUI 依赖）；本文件只定义编辑器 GUI。

/**
 * @brief 一维值曲线编辑器（函数图 / 包络编辑器，Curve1D 专用）。
 *
 * 与 Curve2DEditor（自由 XY 平面）相对：X 轴固定为完整周期（数据层进度 0~1，
 * 刻度按 进度 × 周期 显示实际时间秒数）、Y 轴为输出值，
 * 曲线即 y = f(progress) 函数，形态对齐 DAW 包络/自动化编辑器：
 * - 左上角图例：每条曲线一条色带 + 名称 + 实时输出值，点击行切换编辑目标；
 * - 左侧 Y 轴刻度、底部 X 轴时间刻度（1-2-5 自适应步长，单位秒）+ 细网格；
 * - 每条曲线独立配色，选中曲线加粗并显示控制点，其余曲线细线；
 * - 绿色竖直播放头（progress 处）+ 各曲线在播放头处的值圆点。
 *
 * 交互：左键拖动控制点（X 限制在相邻点之间且 0~1，Y 自由）、单击曲线/图例切换
 * 选中、双击按 X 排序插入点、中键或右键上下平移（仅 Y）、滚轮缩放（仅 Y，
 * 以鼠标为锚点）、Delete 删除选中点。「适应视图」= fitY()。
 * 数据仍为 CurvePath（z 恒 0，闭合强制开放——函数曲线无闭合语义）。
 */
class Curve1DEditor : public QWidget
{
    Q_OBJECT

public:
    explicit Curve1DEditor(QWidget* parent = nullptr);

    /** 设置全部曲线（按索引配色，重建信号连接）。 */
    void setPaths(const QList<CurvePath*>& paths);
    void setActiveIndex(int index);
    int activeIndex() const { return m_activeIndex; }
    void setPlaybackFraction(double fraction);
    /** 周期（秒）：X 轴刻度按 进度 × 周期 显示实际时间。 */
    void setPeriod(double seconds);
    /** Y 轴适配全部曲线的值范围（X 轴恒为完整周期）。 */
    void fitY();

signals:
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
    CurvePath* activePath() const;
    QRectF plotRect() const;
    QPointF valueToScreen(double progress, double value) const;
    QPointF screenToValue(const QPointF& screen) const;
    int pickPoint(const QPoint& position) const;
    int pickCurve(const QPoint& position) const;
    void drawGridAndAxes(QPainter* painter) const;
    void drawLegend(QPainter* painter);
    static QColor curveColor(int index);

    QList<QPointer<CurvePath>> m_paths;
    int m_activeIndex = -1;
    double m_fraction = 0.0;
    double m_period = 5.0;      // 周期（秒）：X 轴时间刻度的换算基准

    double m_valueMin = 0.0;    // Y 轴可视范围（X 轴固定完整周期，无缩放/平移）
    double m_valueMax = 1.0;
    bool m_framed = false;      // 首次显示前自动 fitY 一次
    bool m_panning = false;
    bool m_draggingPoint = false;
    int m_hoverIndex = -1;
    QPoint m_lastMouse;
    QList<QRect> m_legendRects; // 图例命中区（paint 时更新，坐标为控件坐标）
};
