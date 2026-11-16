//
// Created by bwll1 on 2024/5/30.
//
// TimeLineStyle —— QtTimeLine 视图样式默认值（QSS 回退）
//
// ⚠️ 样式来源约定（重要）：
// 1. 运行时的外观颜色一律通过 QSS 配置：
//      BaseTimelineView  { qproperty-timelineBgColor: ...; qproperty-... }
//      BaseTracklistView { qproperty-trackColor: ...; qproperty-... }
//      ClipAppearancePanel { qproperty-swatchBorderColor: ...; qproperty-... }
// 2. 本文件中的颜色仅作为「QSS 未生效时的回退默认值」：
//      - 视图成员 m_xxxColor 在构造时从这里取默认值；
//      - QSS 通过 qproperty- 覆盖成员值（见 BaseTimeLineView.h / BaseTrackListView.h）。
// 3. 【clip-sync 桥接量已拆出】片段绘制读取的 ClipColor/ClipBorderColour/
//      ClipBorderWidth/clipround 已拆至 BaseClass.dll 的 ClipPaintStyle.h
//      （2026-11-13 拆分迁移）——它们是片段绘制协议（视图 QSS 写、片段
//      paint 读，跨 DLL 单实例），随片段基类走；本文件只保留【纯视图侧】
//      样式量（QSS 回退默认 + 布局度量 + 缩放参数），仅 TimeLineBase 读写。
//      BaseTimeLineView::applyStyleFromProperties() 写桥接量时 include
//      "ClipPaintStyle.h"（TimeLineBase 链 BaseClass 取符号）。
//
// 【2026-11-13 迁移】原在 BaseClass（BASECLASS_EXPORT）——职责重定位：
// 视图样式非节点/片段实现时引用的基类，随唯一消费者（本库视图类）迁入
// TimeLineBase.dll；导出宏换 NODE_TIMELINE_PUBLIC。
//
// 【2026-11-12 单实例化】本头全局量原为 inline 变量（静态库时代随消费方
// exe 单实例）。DLL 化后 inline 变量会跨 DLL 各持一份副本，折断样式桥接
// ——改为【extern 声明 + TimeLineStyle.cpp 唯一定义】，全进程单实例。
//
#ifndef TIMELINESTYLE_H
#define TIMELINESTYLE_H

#include <QColor>
#include "TimelineExports.hpp"

// ===================== 布局度量（结构参数，非颜色） =====================
// 说明：这些参数参与滚动/命中/绘制几何计算，随 QSS 热切换会导致缓存错位，
// 因此保持为编译期常量，不通过 QSS 配置。
NODE_TIMELINE_PUBLIC extern int toolbarButtonWidth;  // 工具栏按钮宽度
NODE_TIMELINE_PUBLIC extern int toolbarHeight;       // 工具栏高度
NODE_TIMELINE_PUBLIC extern int trackHeight;         // 轨道高度（2026-11-05 40→52 为内嵌 tween 曲线带
                                                     // 让位；2026-11-15 52→40：panel 已退役回归紧凑行高）
// 轨道展开附加高度 trackExpandedExtra 已随轨道折叠功能退役（2026-11-15）——
// 行高恒 = trackHeight。
NODE_TIMELINE_PUBLIC extern int rulerHeight;         // 刻度高度
NODE_TIMELINE_PUBLIC extern int clipoffset;          // clip 偏移
NODE_TIMELINE_PUBLIC extern int textoffset;          // 文本偏移
NODE_TIMELINE_PUBLIC extern int fontSize;            // 显示时间字体大小
NODE_TIMELINE_PUBLIC extern int playheadwidth;       // 播放头宽度
NODE_TIMELINE_PUBLIC extern int playheadheight;      // 播放头高度
NODE_TIMELINE_PUBLIC extern int playheadCornerHeight; // 播放头圆角高度

// ===================== 时间刻度（缩放参数，μs 原生） =====================
/**
 * 参数说明：timescale初始值（像素/微秒）
 * - 语义：当前时间轴上“每微秒占据的像素宽度”，单位 px/μs。
 * - 作用：被用于 pointToFrame/frameToPoint、刻度绘制、剪辑位置与宽度计算等所有横向度量相关的逻辑
 *   （pointToFrame 返回 μs，与库内时间基同域）。
 * - 缩放行为：timescale 越大，时间轴越“放大”（每 μs 越宽）；timescale 越小，时间轴越“缩小”。
 * - 取值范围：通常应在 [minTimescale, baseTimeScale] 之间；UI 层通过 currentScale（0~1）
 *   对数映射到此范围（2026-11-16，原为线性）。
 *   映射：timescale = minTimescale × (baseTimeScale/minTimescale)^currentScale
 *   ——线性映射跨数量级时低端被 minTimescale 垫底稀释成亚像素死区（缩到
 *   最小附近回滚长时间无可见变化）；对数映射每步缩放视觉变化恒定、无死区。
 * - 初始化建议：若修改 timescale 的默认值，建议同步更新 BaseTimeLineView.h 中 currentScale 的初始值为
 *   ln(timescale/minTimescale) / ln(baseTimeScale/minTimescale)，避免程序第一次滚轮缩放出现“跳变”。
 * - 换算基准：kFrameUs = 40'000（PAL 25fps），旧 px/frame 值 ÷ 40000 = px/μs 值。
 */
NODE_TIMELINE_PUBLIC extern double timescale; // 时间刻度(像素/μs) = 旧 0.5 px/帧

/**
 * 参数说明：minTimescale初始值（最小像素/μs）
 * - 语义：允许的最小缩放下限，即每微秒占用的最小像素宽度，限制过度缩小导致文本/控件不可见或精度问题。
 * - 调参建议（2026-11-11 用户反馈：最小缩放毫无意义地小——旧 1.25e-7 下
 *   1 分钟工程只有 7.5px 宽、1 小时工程也只有 450px，且 pointToFrame 反算
 *   值超出 int 域引发截断）：
 *   1e-5 px/μs = 旧 0.4 px/帧 —— 60s 工程缩到最小约 600px（可整体入画），
 *   10min 工程约 6000px（一次滚动可达），1h 工程 36000px（分段滚动浏览）。
 * - 调参（2026-11-16 用户反馈：还要能缩更小）：1e-5→1e-6 px/μs（1s=1px）——
 *   视口 1200px ≈ 20min 全入画，1h 工程 3600px（一次滚动可达）；
 *   放大上限同轮回调 2.5e-3→1e-3（25fps 1帧 100px→40px，1ms=1px）。
 * - 与 currentScale 的关系：当 currentScale=0 时应映射到 minTimescale，以保证最小缩放可达。
 */
NODE_TIMELINE_PUBLIC extern double minTimescale; // 最小缩放(像素/μs)（1.25e-7→1e-5→1e-6，1s=1px）

NODE_TIMELINE_PUBLIC extern double baseTimeScale; // 缩放上限(像素/μs)（2.5e-4→2.5e-3→1e-3，25fps 1帧=40px）

// ===================== 颜色默认值（均可被 QSS qproperty- 覆盖） =====================
// 颜色取值遵循 docs/brand/Flow-Brand-Visual-Identity-Manual.html（流动绿 #79AE31）：
// 选中/强调 = #79AE31；悬停 = 深一档绿 #39571E(800)；片段悬停 = #9CC455(400)。
// 【2026-11-13 拆分】片段绘制读取的 ClipColor/ClipBorderColour 已迁
// BaseClass.dll 的 ClipPaintStyle.h（片段绘制协议随片段基类走）。

// ---- 时间轴视图 BaseTimelineView 的可配置颜色（QSS 属性名见 BaseTimeLineView.h）----
NODE_TIMELINE_PUBLIC extern QColor bgColour;            // 背景（qproperty-timelineBgColor）
NODE_TIMELINE_PUBLIC extern QColor seperatorColour;     // 轨道分隔线（qproperty-timelineSeparatorColor）
NODE_TIMELINE_PUBLIC extern QColor rulerColour;         // 刻度/标尺（qproperty-timelineRulerColor）
NODE_TIMELINE_PUBLIC extern QColor playheadColour;      // 播放头（qproperty-timelinePlayheadColor）
NODE_TIMELINE_PUBLIC extern QColor trackSelectedColour; // 轨道选中·品牌流动绿（qproperty-trackSelectedColor）

// ---- 轨道列表视图 BaseTracklistView 的可配置颜色（QSS 属性名见 BaseTrackListView.h）----
NODE_TIMELINE_PUBLIC extern QColor trackColour;      // 轨道行背景（qproperty-trackColor）
NODE_TIMELINE_PUBLIC extern QColor trackTitleColor;  // 轨道名文字（qproperty-trackTitleColor）
NODE_TIMELINE_PUBLIC extern QColor trackHoverColour; // 轨道悬停·绿 800（qproperty-trackHoverColor）

// ---- 标尺时间文字颜色（qproperty-timelinePlayheadColor on BaseTracklistView）----
// 与播放头同色系，标尺中央显示的当前时间文字使用。

#endif //TIMELINESTYLE_HPP
