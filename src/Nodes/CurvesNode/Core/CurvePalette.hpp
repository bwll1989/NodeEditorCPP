#pragma once

#include <QColor>

/**
 * 曲线家族统一调色板（Curve1D/2D/3D 共用）：
 * 图例色带、选中曲线、幽灵曲线（暗化）全部取自本表——保证
 * 「图例颜色 = 曲线颜色」一一对应。改色只改这里，三家自然同步。
 */
inline QColor curvePaletteColor(int index)
{
    static const QColor palette[] = {
        QColor(232, 48, 82),     // 红
        QColor(62, 178, 96),     // 绿
        QColor(70, 130, 230),    // 蓝
        QColor(244, 156, 48),    // 橙
        QColor(178, 96, 220),    // 紫
        QColor(52, 188, 188),    // 青
        QColor(226, 96, 152),    // 玫红
        QColor(168, 158, 60),    // 橄榄
    };
    constexpr int count = int(sizeof(palette) / sizeof(palette[0]));
    return palette[((index % count) + count) % count];
}

/** 幽灵曲线用：调色板色暗化压灰，保持可辨但弱于选中曲线。 */
inline QColor curveGhostColor(int index)
{
    const QColor base = curvePaletteColor(index);
    return QColor(int(base.red() * 0.45), int(base.green() * 0.45), int(base.blue() * 0.45));
}
