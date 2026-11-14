//
// ClipPaintStyle.cpp —— ClipPaintStyle.h 桥接量的唯一定义点。
// 协议说明见 ClipPaintStyle.h（[clip-sync]：视图 QSS 写、片段 paint 读，
// 全进程单实例——2026-11-13 自 TimeLineStyle 拆分，随片段基类留 BaseClass）。
//
#include "ClipPaintStyle.h"

QColor ClipColor        = QColor(126, 188, 105); // 片段默认填充（qproperty-clipFillColor 同步）
QColor ClipBorderColour = QColor(235, 154, 5);   // 片段边框 [clip-sync]（qproperty-clipBorderColor 同步）
int ClipBorderWidth = 2;  // clip 选中边框宽度
int clipround = 0;        // clip 圆角
