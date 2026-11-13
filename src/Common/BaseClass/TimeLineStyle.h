//
// Created by bwll1 on 2024/5/30.
//
// TimeLineStyle —— QtTimeLine 样式默认值（QSS 回退）
//
// ⚠️ 样式来源约定（重要）：
// 1. 运行时的外观颜色一律通过 QSS 配置：
//      BaseTimelineView  { qproperty-timelineBgColor: ...; qproperty-... }
//      BaseTracklistView { qproperty-trackColor: ...; qproperty-... }
//      ClipAppearancePanel { qproperty-swatchBorderColor: ...; qproperty-... }
// 2. 本文件中的颜色仅作为「QSS 未生效时的回退默认值」：
//      - 视图成员 m_xxxColor 在构造时从这里取默认值；
//      - QSS 通过 qproperty- 覆盖成员值（见 BaseTimeLineView.h / BaseTrackListView.h）。
// 3. 标注 [clip-sync] 的变量是绘制桥接缓存：
//      BaseTimelineView::applyStyleFromProperties() 会在 QSS 生效/变化时，
//      把视图成员颜色同步到这些全局量，供片段(AbstractClipModel 子类)的
//      paint() 代码读取——片段模型不是 QWidget，无法直接被 QSS qproperty- 定向。
//      除该同步函数外，任何代码都不应再写这些全局量。
//
// 【2026-11-12 收编 BaseClass.dll】本头原为 inline 变量（静态库时代随消费
// 方 exe 单实例）。随 AbstractClipModel 收编 BaseClass.dll 后，inline 变量
// 会跨 DLL 各持一份副本（exe 份/ dll 份），折断 [clip-sync]（视图在 exe
// 写、片段绘制在 dll 读）——改为【BASECLASS_EXPORT extern 声明 +
// TimeLineStyle.cpp 唯一定义】，全进程单实例。调用方代码零改动。
//
#ifndef TIMELINESTYLE_H
#define TIMELINESTYLE_H

#include <QColor>
#include "BaseClassExport.h"

// ===================== 布局度量（结构参数，非颜色） =====================
// 说明：这些参数参与滚动/命中/绘制几何计算，随 QSS 热切换会导致缓存错位，
// 因此保持为编译期常量，不通过 QSS 配置。
BASECLASS_EXPORT extern int toolbarButtonWidth;  // 工具栏按钮宽度
BASECLASS_EXPORT extern int toolbarHeight;       // 工具栏高度
BASECLASS_EXPORT extern int trackHeight;         // 轨道高度（2026-11-05 40→52：片段底部让位
                                                 // 给 30px tween 曲线编辑带 + 18px 标题区，
                                                 // WATCHOUT 式 timeline 内曲线拖值需要竖向空间）
// 轨道展开附加高度（2026-11-06 轨道展开 + clip 内嵌 tween panel）：
// 展开行高 = trackHeight + trackExpandedExtra，附加区域按 clip x/宽摆放
// 宿主注入的 clip 面板（TweenPanel：通道 chips 行 ~22 + 行头 ~20 + 曲线区 ~120）。
// 结构参数（参与滚动/命中/绘制几何），与 trackHeight 同纪律：编译期常量，不走 QSS。
BASECLASS_EXPORT extern int trackExpandedExtra;
BASECLASS_EXPORT extern int rulerHeight;         // 刻度高度
BASECLASS_EXPORT extern int clipoffset;          // clip 偏移
BASECLASS_EXPORT extern int clipround;           // clip 圆角
BASECLASS_EXPORT extern int textoffset;          // 文本偏移
BASECLASS_EXPORT extern int fontSize;            // 显示时间字体大小
BASECLASS_EXPORT extern int playheadwidth;       // 播放头宽度
BASECLASS_EXPORT extern int playheadheight;      // 播放头高度
BASECLASS_EXPORT extern int playheadCornerHeight; // 播放头圆角高度
BASECLASS_EXPORT extern int ClipBorderWidth;     // clip 选中边框宽度

// ===================== 时间刻度（缩放参数，μs 原生） =====================
/**
 * 参数说明：timescale初始值（像素/微秒）
 * - 语义：当前时间轴上“每微秒占据的像素宽度”，单位 px/μs。
 * - 作用：被用于 pointToFrame/frameToPoint、刻度绘制、剪辑位置与宽度计算等所有横向度量相关的逻辑
 *   （pointToFrame 返回 μs，与库内时间基同域）。
 * - 缩放行为：timescale 越大，时间轴越“放大”（每 μs 越宽）；timescale 越小，时间轴越“缩小”。
 * - 取值范围：通常应在 [minTimescale, baseTimeScale] 之间；UI 层通过 currentScale（0~1）线性映射到此范围。
 *   映射示例：timescale = minTimescale + currentScale * (baseTimeScale - minTimescale)
 * - 初始化建议：若修改 timescale 的默认值，建议同步更新 BaseTimeLineView.h 中 currentScale 的初始值为
 *   (timescale - minTimescale) / (baseTimeScale - minTimescale)，避免程序第一次滚轮缩放出现“跳变”。
 * - 换算基准：kFrameUs = 40'000（PAL 25fps），旧 px/frame 值 ÷ 40000 = px/μs 值。
 */
BASECLASS_EXPORT extern double timescale; // 时间刻度(像素/μs) = 旧 0.5 px/帧

/**
 * 参数说明：minTimescale初始值（最小像素/μs）
 * - 语义：允许的最小缩放下限，即每微秒占用的最小像素宽度，限制过度缩小导致文本/控件不可见或精度问题。
 * - 调参建议（2026-11-11 用户反馈：最小缩放毫无意义地小——旧 1.25e-7 下
 *   1 分钟工程只有 7.5px 宽、1 小时工程也只有 450px，且 pointToFrame 反算
 *   值超出 int 域引发截断）：
 *   1e-5 px/μs = 旧 0.4 px/帧 —— 60s 工程缩到最小约 600px（可整体入画），
 *   10min 工程约 6000px（一次滚动可达），1h 工程 36000px（分段滚动浏览）。
 * - 与 currentScale 的关系：当 currentScale=0 时应映射到 minTimescale，以保证最小缩放可达。
 */
BASECLASS_EXPORT extern double minTimescale; // 最小缩放(像素/μs) = 旧 0.4 px/帧（1.25e-7→1e-5，2026-11-11）

BASECLASS_EXPORT extern double baseTimeScale; // 缩放上限(像素/μs)（U3：2.5e-4→2.5e-3，1帧=100px，音频波形对齐/子帧操作有空间）

// ===================== 颜色默认值（均可被 QSS qproperty- 覆盖） =====================
// 颜色取值遵循 docs/brand/Flow-Brand-Visual-Identity-Manual.html（流动绿 #79AE31）：
// 选中/强调 = #79AE31；悬停 = 深一档绿 #39571E(800)；片段悬停 = #9CC455(400)。

// ---- 时间轴视图 BaseTimelineView 的可配置颜色（QSS 属性名见 BaseTimeLineView.h）----
BASECLASS_EXPORT extern QColor bgColour;            // 背景（qproperty-timelineBgColor）
BASECLASS_EXPORT extern QColor seperatorColour;     // 轨道分隔线（qproperty-timelineSeparatorColor）
BASECLASS_EXPORT extern QColor rulerColour;         // 刻度/标尺（qproperty-timelineRulerColor）
BASECLASS_EXPORT extern QColor playheadColour;      // 播放头（qproperty-timelinePlayheadColor）
BASECLASS_EXPORT extern QColor ClipColor;           // 片段默认填充（qproperty-clipFillColor）
BASECLASS_EXPORT extern QColor ClipBorderColour;    // 片段边框 [clip-sync]（qproperty-clipBorderColor）
BASECLASS_EXPORT extern QColor trackSelectedColour; // 轨道选中·品牌流动绿（qproperty-trackSelectedColor）

// ---- 轨道列表视图 BaseTracklistView 的可配置颜色（QSS 属性名见 BaseTrackListView.h）----
BASECLASS_EXPORT extern QColor trackColour;      // 轨道行背景（qproperty-trackColor）
BASECLASS_EXPORT extern QColor trackTitleColor;  // 轨道名文字（qproperty-trackTitleColor）
BASECLASS_EXPORT extern QColor trackHoverColour; // 轨道悬停·绿 800（qproperty-trackHoverColor）

// ---- 标尺时间文字颜色（qproperty-timelinePlayheadColor on BaseTracklistView）----
// 与播放头同色系，标尺中央显示的当前时间文字使用。

#endif //TIMELINESTYLE_HPP
