// ============================================================================
//  MappingClipModel —— Mapping 映射片段模型（src/Clips/Mapping/）
//
//  【2026-11-15 片段移植】自节点体系 mappingclipmodel.hpp（NodeClipDelegate
//  Model 帧域，header-only）移植为 SceneClipModel（μs 域）子类（同样
//  header-only，对齐 ControlClipModel.hpp 先例）。
//
//  【2026-11-27 死代码清理】播放期求值链（onPlayheadAdvanced → emitForFrame
//  → evaluateAndSend/evalList + QJSEngine/消息表镜像）整体退役——
//  【2026-11-22 起】逐帧求值发送由 PlaybackSendScheduler 独立线程承担
//  （时钟 deadline 直读主时钟；快照 rebuildSendSnapshot 直读工作副本
//  clip_data 的 {"messages":[...]}，不经过模型；见
//  src/Widget/TimeLineWidget/SendScheduler.cpp）。本模型收敛为【纯几何壳】
//  （色/默认时长/可调长——全部编辑期语义在基类）。
//
//  几何/绘制走基类（玫红块 #B83B5E，可拖拽调长）；默认时长 = 100 帧
// （旧版构造 m_end = m_start + 100 的帧域口径，μs 化 = 100×kFrameUs）。
// ============================================================================

#pragma once

#include <QColor>
#include <QString>

#include "SceneClipModel.hpp"
#include "TimeLineDefines.h"            // kFrameUs（旧帧域口径 μs 化）

/// Mapping 映射片段模型：几何壳（玫红块 #B83B5E，可拖拽调长）。业务数据
/// （消息表）权威在 ms::Clip.clip_data——播放期由 PlaybackSendScheduler
/// 快照直读工作副本求值发送；属性面板（Section）同样直读工作副本。
/// 本类零业务状态。
class MappingClipModel : public SceneClipModel {
    Q_OBJECT
public:
    explicit MappingClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, QStringLiteral("Mapping"), {}, parent) {
        RESIZEABLE  = true;   // 可拖拽调长（时长 = 映射播放区间）
        EMBEDWIDGET = false;
        SHOWBORDER  = false;  // 【2026-11-27 用户定案】所有片段无边框
        setColor(QColor(0xB8, 0x3B, 0x5E)); // 玫红（节点体系 Mapping 类型色 #b83b5e）
        setEnd(startUs + 100 * QtTimeline::kFrameUs); // 旧 ctor：m_end = m_start + 100 帧
    }
};
