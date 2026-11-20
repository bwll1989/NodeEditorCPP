// ============================================================================
//  ClipTimePanel —— 片段时间属性通用面板（属性面板「通用」页的时间区块）
//
//  出处：收编自合并前 QtTimeLine 体系（原 AbstractClipModel::initPropertyWidget
//  内嵌 timeGroupBox 独立成类，原类名 ClipTimeGroupBox——文件名已随
//  「XxxPanel」通用页组件命名定稿；2026-11-15 注释/命名现代化对齐新风格）。
//
//  【P3 面板迁库 2026-11-17】自 BaseClass 迁入 ClipPanelBase（通用片段 UI
//  框架库）——BaseClass 回归零 UI 纯模型/接口库；导出宏换
//  CLIPPANELBASE_EXPORT。OSC 外控登记随面板自治（见下）。
//
//  职责（「通用」页时间区块三行——帧数 + 多制式时间码双视图）：
//    * 开始帧 / 片段位置 / 结束帧编辑——任一控件变更即时写回模型
//     （帧级 UI ↔ 模型 μs 经 framesToUs/usToFrames 显示层换算）；
//    * 双向同步：模型 timelinePositionChanged/lengthChanged → 灌显示
//     （QSignalBlocker 防回环）；start/end 互夹（RESIZEABLE 时）；
//    * 挂接方式 = 构造即自治（owner 指针直连模型信号——11-13 判定保持
//      项：本组件与构造链强耦合，与 ClipPropertyPanel/XxxClipSection 的
//      回调注入式解耦不同层）；构造尾完成 OSC 外控登记（原
//      AbstractClipModel::initPropertyWidget 尾部 6× registerExternalControl
//      + pushOscInitialState 收编入构造——面板在哪装配，外控注册就在哪，
//      独立打开路径不再漏登记）；
//    * 公开控件成员 = OSC 外部控制注册表 API。
//
//  【2026-11-15 现代化（用户定案「片段面板对齐工程统一风格」）】收编残留
//  清理：删除三个死信号（startFrameChanged/positionChanged/endFrameChanged
//  ——全工程零 emit 零 connect，且原文件无 Q_OBJECT 宏它们本就不是真信号）
//  与三个死成员（*_frame_label_——声明后从未赋值，构造用的是局部变量）；
//  补 Q_OBJECT 对齐工程惯例；成员命名 m_xxx → 尾下划线（对齐
//  ClipPropertyPanel/TweenSection/XxxClipSection 体系）；头注释统一
//  ==== 框新风格。src/Nodes/TimeLineNode/ 的同名内嵌副本（TimelineEngine.h）
//  为节点引擎冻结胶囊，按既定铁律独立演化、不同步本改动。
// ============================================================================

#pragma once

#include "ClipPanelBaseExport.h"
#include "IntDragValueWidget/IntDragValueWidget.hpp"
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include "Common/AppConfig/TimeCodeDefines.h"
#include <QTimeEdit>

class AbstractClipModel;

/// 片段时间属性通用面板（「通用」页时间区块；owner 指针直连模型信号）
class CLIPPANELBASE_EXPORT ClipTimePanel : public QGroupBox {
    Q_OBJECT
public:
    explicit ClipTimePanel(AbstractClipModel* owner, QWidget* parent = nullptr);
    /// 控件 ↔ 槽接线（构造尾调用；模型信号连接独立成方法供测试复用）
    void initSignalConnections();

    // ---- 控件成员（OSC 外部控制注册 API；公开=AbstractClipModel 注册引用）--
    //  帧数（IntDragValueWidget）+ 时间码（QTimeEdit）成对
    IntDragValueWidget* start_frame_spin_ {nullptr};
    QTimeEdit*  start_time_code_ {nullptr};

    IntDragValueWidget* position_frame_spin_ {nullptr};
    QTimeEdit*  position_time_ {nullptr};

    IntDragValueWidget* end_frame_spin_ {nullptr};
    QTimeEdit*  end_time_code_ {nullptr};

public Q_SLOTS:
    void updateStartFrame(int frames);
    void updateStartTimeCode(const QTime& tc);
    void updateEndFrame(int frames);
    void updateEndTimeCode(const QTime& tc);
    void updatePositionFrame(int frames);
    void updatePositionTimeCode(const QTime& tc);

private:
    int clampStartFrame(int frames) const;
    int clampEndFrame(int frames) const;
    void refreshFrameRanges();

private:
    AbstractClipModel* owner_ {nullptr};
    QGridLayout* time_layout_ {nullptr};
    bool m_connections_inited {false}; ///< 信号接线防重入（重复 connect 防护）
};
