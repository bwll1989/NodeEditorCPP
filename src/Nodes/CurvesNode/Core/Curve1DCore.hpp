#pragma once

#include "CurvePath.hpp"

#include <QList>
#include <QPointF>
#include <QVector>

// 一维值曲线核心算法层（Curve1D 专用）。
// 仅依赖 CurvePath 与 Qt 基础类型，不含任何 GUI（QWidget）依赖：
// 模型（数据生成/outData）与编辑器（GUI 绘制/拾取）共用同一份实现，
// 保证采样语义只有一份定义——界面只消费算法，不参与数据行为的定义。

/**
 * 一维值曲线取样：progress(0~1) → 曲线在 X=progress 处的 Y 值。
 * 单调三次插值（Fritsch–Carlson）：曲线在关键点之间严格单调、不过冲、不回弯
 * （参数化 Catmull-Rom 的 X 分量在长短段相邻时会过冲回弯成"钩子"，不适合包络）；
 * 目标 X 在首末点范围之外时保持端点值（hold）。
 */
double curve1DValueAt(CurvePath* path, double progress);

/**
 * 采样曲线的全跨度折线（值域坐标：x=进度 0~1，含首末点之外的 hold 平段；y=值）。
 * 与 curve1DValueAt 同一插值算法——绘制与输出严格一致；segments 为采样段数。
 */
QVector<QPointF> curve1DSamplePolyline(CurvePath* path, int segments);

/** 一维曲线归一化：控制点按 X 升序排序、X 钳制到 0~1、强制开放（载入存档后调用）。 */
void normalizeCurve1D(CurvePath* path);
