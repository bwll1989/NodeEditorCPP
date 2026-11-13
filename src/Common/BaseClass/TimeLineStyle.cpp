//
// TimeLineStyle.cpp —— TimeLineStyle.h 的唯一定义点（2026-11-12 随
// AbstractClipModel 收编 BaseClass.dll 而新建）。
//
// 背景：原 TimeLineStyle.h 的全局量是 inline 变量（静态库时代随消费方 exe
// 单实例）。基类收编 BaseClass.dll（SHARED）后，inline 变量在 exe 与 dll
// 各持一份副本，会折断 [clip-sync] 样式桥接（BaseTimeLineView 的 QSS 同步
// 写 exe 份、AbstractClipModel::drawBackground 在 dll 读）——故 extern 化：
// 本 cpp 是全进程唯一实例，头文件只留 BASECLASS_EXPORT extern 声明。
//
#include "TimeLineStyle.h"

// ===================== 布局度量（结构参数，非颜色） =====================
int toolbarButtonWidth = 16;  // 工具栏按钮宽度
int toolbarHeight = 30;       // 工具栏高度
int trackHeight = 52;         // 轨道高度（2026-11-05 40→52：片段底部让位
                              // 给 30px tween 曲线编辑带 + 18px 标题区，
                              // WATCHOUT 式 timeline 内曲线拖值需要竖向空间）
int trackExpandedExtra = 168; // 轨道展开附加高度（2026-11-06）
int rulerHeight = 30;         // 刻度高度
int clipoffset = 2;           // clip 偏移
int clipround = 0;            // clip 圆角
int textoffset = 13;          // 文本偏移
int fontSize = 25;            // 显示时间字体大小
int playheadwidth = 5;        // 播放头宽度
int playheadheight = 12;      // 播放头高度
int playheadCornerHeight = playheadheight / 4; // 播放头圆角高度
int ClipBorderWidth = 2;      // clip 选中边框宽度

// ===================== 时间刻度（缩放参数，μs 原生） =====================
double timescale = 1.25e-5;    // 时间刻度(像素/μs) = 旧 0.5 px/帧
double minTimescale = 1e-5;    // 最小缩放(像素/μs) = 旧 0.4 px/帧（1.25e-7→1e-5，2026-11-11）
double baseTimeScale = 2.5e-3; // 缩放上限(像素/μs)（U3：2.5e-4→2.5e-3，1帧=100px）

// ===================== 颜色默认值（均可被 QSS qproperty- 覆盖） =====================
// 颜色取值遵循 docs/brand/Flow-Brand-Visual-Identity-Manual.html（流动绿 #79AE31）：
// 选中/强调 = #79AE31；悬停 = 深一档绿 #39571E(800)；片段悬停 = #9CC455(400)。

// ---- 时间轴视图 BaseTimelineView 的可配置颜色 ----
QColor bgColour            = QColor(53, 53, 53);        // 背景（qproperty-timelineBgColor）
QColor seperatorColour     = QColor("#414141");         // 轨道分隔线（qproperty-timelineSeparatorColor）
QColor rulerColour         = QColor("#a7a7a7");         // 刻度/标尺（qproperty-timelineRulerColor）
QColor playheadColour      = QColor(206, 91, 87);       // 播放头（qproperty-timelinePlayheadColor）
QColor ClipColor           = QColor(126, 188, 105);     // 片段默认填充（qproperty-clipFillColor）
QColor ClipBorderColour    = QColor(235, 154, 5);       // 片段边框 [clip-sync]（qproperty-clipBorderColor）
QColor trackSelectedColour = QColor("#79AE31");         // 轨道选中·品牌流动绿（qproperty-trackSelectedColor）

// ---- 轨道列表视图 BaseTracklistView 的可配置颜色 ----
QColor trackColour      = QColor(53, 53, 53);           // 轨道行背景（qproperty-trackColor）
QColor trackTitleColor  = QColor(255, 255, 255);        // 轨道名文字（qproperty-trackTitleColor）
QColor trackHoverColour = QColor("#39571E");            // 轨道悬停·绿 800（qproperty-trackHoverColor）
