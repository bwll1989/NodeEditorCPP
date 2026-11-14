#ifndef CLIPPAINTSTYLE_H
#define CLIPPAINTSTYLE_H

#include <QColor>
#include "BaseClassExport.h"

// ============================================================================
//  ClipPaintStyle —— 片段绘制外观桥接量（[clip-sync] 协议的片段侧 half）
//
// 【2026-11-13 拆分迁移】原在 TimeLineStyle.h（随视图样式迁 TimeLineBase.dll
// 时拆出）。这些量是「视图 QSS ↔ 片段 paint」的桥接协议：
//   * 视图侧：BaseTimeLineView::applyStyleFromProperties() 在 QSS 生效/变化
//     时把视图成员颜色写到这里（片段模型不是 QWidget，无法被 QSS qproperty-
//     定向）——除该同步函数外任何代码都不应再写；
//   * 片段侧：AbstractClipModel 子类的 paint() 代码读取（片段模型不是
//     QWidget，颜色只能来自全局量）。
//
// 【为何在 BaseClass】片段绘制协议 = 片段作者实现时引用的接口，随片段基类
// 走；且 TimeLineBase（视图，写方）链 BaseClass（读方，片段）单向成立，
// 定义放本库才不构成循环依赖。全进程单实例：本头 extern 声明 +
// ClipPaintStyle.cpp 唯一定义（inline 变量跨 DLL 各持副本会折断同步，
// 教训见 TimeLineStyle.cpp 头注释）。
// ============================================================================

BASECLASS_EXPORT extern QColor ClipColor;        // 片段默认填充（qproperty-clipFillColor 同步）
BASECLASS_EXPORT extern QColor ClipBorderColour; // 片段边框 [clip-sync]（qproperty-clipBorderColor）
BASECLASS_EXPORT extern int ClipBorderWidth;     // clip 选中边框宽度
BASECLASS_EXPORT extern int clipround;           // clip 圆角

#endif // CLIPPAINTSTYLE_H
