// ============================================================================
//  ClipTimePanel.cpp —— 片段时间属性通用面板实现
//  （收编自合并前 QtTimeLine 体系；2026-11-15 成员命名现代化对齐新风格）
//
//  行为要点（逐行保持原实现，仅改名）：
//    * 显示为 00:00:00:000 多制式时间码；
//    * 时间码 textChanged 与帧数 spinbox 一样调用 setStart/setEnd；
//    * 响应 AbstractClipModel 的 timelinePositionChanged/lengthChanged
//      信号更新 UI；
//    * 2026-11-10 微秒原生：模型域 = μs；本面板 UI 域 = 帧数
//     （IntDragValueWidget int 值 + 多制式时间码），进出模型一律经
//      framesToUs/usToFrames 显示层换算。
// ============================================================================

#include "ClipTimePanel.hpp"

#include "AbstractClipModel.hpp"
#include "Common/AppConfig/TimeCodeDefines.h"
#include "TimeLineDefines.h" // framesToUs/usToFrames 显示层换算（μs 原生）
#include <QObject>
#include <QString>

ClipTimePanel::ClipTimePanel(AbstractClipModel* owner, QWidget* parent)
    : owner_(owner),
    QGroupBox(parent)
{
    setTitle(tr("时间属性"));
    setFlat(true); // 带文字的分割线（2026-11-13 用户定案：四区块统一去框）
    time_layout_ = new QGridLayout(this);

    // 开始帧显示
    auto startLabel = new QLabel(tr("开始帧:"), this);
    start_frame_spin_ = new IntDragValueWidget(this);
    start_frame_spin_->setRange(0, 9999999);
    start_time_code_ = new QTimeEdit(this);
    start_time_code_->setDisplayFormat("hh:mm:ss:zzz");
    time_layout_->addWidget(startLabel, 0, 0,1,1);
    time_layout_->addWidget(start_frame_spin_, 0, 1,1,1);
    time_layout_->addWidget(start_time_code_, 0, 2,1,1);

    // 片段位置显示（绝对时间：毫秒）
    auto lengthLabel = new QLabel(tr("片段位置:"), this);
    position_frame_spin_ = new IntDragValueWidget(this);
    position_frame_spin_->setRange(0, 9999999);
    position_time_ = new QTimeEdit(this);
    position_time_->setDisplayFormat("hh:mm:ss:zzz");
    time_layout_->addWidget(lengthLabel, 1, 0,1,1);
    time_layout_->addWidget(position_frame_spin_, 1 ,1,1,1);
    time_layout_->addWidget(position_time_, 1, 2,1,1);

    // 结束帧显示（绝对时间：毫秒）
    auto endLabel = new QLabel(tr("结束帧:"), this);
    end_frame_spin_ = new IntDragValueWidget(this);
    end_frame_spin_->setRange(0, 9999999);
    end_time_code_ = new QTimeEdit(this);
    end_time_code_->setDisplayFormat("hh:mm:ss:zzz");
    time_layout_->addWidget(endLabel, 2, 0,1,1);
    time_layout_->addWidget(end_frame_spin_, 2, 1,1,1);
    time_layout_->addWidget(end_time_code_, 2, 2,1,1);
    setMaximumHeight(150);
    // 模型 μs → UI 帧数换算后灌初值
    updateStartFrame(usToFrames(owner_->start()));
    updateEndFrame(usToFrames(owner_->end()));
    updatePositionFrame(usToFrames(owner_->start()));
    initSignalConnections();
}

void ClipTimePanel::initSignalConnections() {
    connect(start_frame_spin_,&IntDragValueWidget::valueChanged,this,&ClipTimePanel::updateStartFrame);
    connect(start_time_code_,&QTimeEdit::timeChanged,this,&ClipTimePanel::updateStartTimeCode);
    connect(end_frame_spin_,&IntDragValueWidget::valueChanged,this,&ClipTimePanel::updateEndFrame);
    connect(end_time_code_,&QTimeEdit::timeChanged,this,&ClipTimePanel::updateEndTimeCode);
    connect(position_frame_spin_,&IntDragValueWidget::valueChanged,this,&ClipTimePanel::updatePositionFrame);
    connect(position_time_,&QTimeEdit::timeChanged,this,&ClipTimePanel::updatePositionTimeCode);
    // 模型信号参数 = μs（微秒原生）→ UI 帧数换算后走同一更新槽
    connect(owner_, &AbstractClipModel::timelinePositionChanged, this, [this](qint64 us){
        updateStartFrame(usToFrames(us));
        updatePositionFrame(usToFrames(us));
    });
    connect(owner_, &AbstractClipModel::lengthChanged, this, [this](qint64 usEnd){
       updateEndFrame(usToFrames(usEnd));
    });
}

int ClipTimePanel::clampStartFrame(int frames) const
{
    if (!owner_ || !owner_->isResizable())
        return frames;
    if (frames < 0)
        frames = 0;
    const int maxStart = usToFrames(owner_->end()) - 1; // 模型 μs → 帧
    if (frames > maxStart)
        frames = qMax(0, maxStart);
    return frames;
}

int ClipTimePanel::clampEndFrame(int frames) const
{
    if (!owner_ || !owner_->isResizable())
        return frames;
    const int minEnd = usToFrames(owner_->start()) + 1; // 模型 μs → 帧
    if (frames < minEnd)
        frames = minEnd;
    return frames;
}

void ClipTimePanel::refreshFrameRanges()
{
    if (!owner_ || !owner_->isResizable() || !start_frame_spin_ || !end_frame_spin_)
        return;
    QSignalBlocker blockStart(start_frame_spin_);
    QSignalBlocker blockEnd(end_frame_spin_);
    start_frame_spin_->setRange(0, qMax(0, usToFrames(owner_->end()) - 1));
    end_frame_spin_->setRange(usToFrames(owner_->start()) + 1, 9999999);
}

void ClipTimePanel::updateStartFrame(int frames) {
    if (!owner_) {
        qDebug() << "owner_ is nullptr";
        return;
    }
    frames = clampStartFrame(frames);
    QSignalBlocker b1(start_frame_spin_);
    QSignalBlocker b2(start_time_code_);
    start_frame_spin_->setValue(frames);
    auto tc = frames_to_timecode_frame(frames, owner_->getTimeCodeType());
    QTime time=timecode_frame_to_qtime(tc, owner_->getTimeCodeType());
    start_time_code_->setTime(time);

    owner_->setStart(framesToUs(frames)); // UI 帧 → 模型 μs
    refreshFrameRanges();
}

void ClipTimePanel::updateStartTimeCode(const QTime& tc) {
    if (!owner_) {
        qDebug() << "owner_ is nullptr";
        return;
    }
    QSignalBlocker b1(start_frame_spin_);
    QSignalBlocker b2(start_time_code_);
    TimeCodeFrame timecode = qtime_to_timecode_frame(tc, owner_->getTimeCodeType());
    int newStart = static_cast<int>(timecode_frame_to_frames(timecode, owner_->getTimeCodeType()));
    newStart = clampStartFrame(newStart);
    start_frame_spin_->setValue(newStart);
    const auto clampedTc = frames_to_timecode_frame(newStart, owner_->getTimeCodeType());
    start_time_code_->setTime(timecode_frame_to_qtime(clampedTc, owner_->getTimeCodeType()));
    owner_->setStart(framesToUs(newStart)); // UI 帧 → 模型 μs
    refreshFrameRanges();

}

void ClipTimePanel::updateEndFrame(int frames) {
    if (!owner_) {
        return;
    }
    frames = clampEndFrame(frames);
    QSignalBlocker b1(end_frame_spin_);
    QSignalBlocker b2(end_time_code_);
    end_frame_spin_->setValue(frames);
    auto tc = frames_to_timecode_frame(frames, owner_->getTimeCodeType());
    QTime time=timecode_frame_to_qtime(tc, owner_->getTimeCodeType());
    end_time_code_->setTime(time);
    owner_->setEnd(framesToUs(frames)); // UI 帧 → 模型 μs
    refreshFrameRanges();
}

void ClipTimePanel::updateEndTimeCode(const QTime& tc) {
    if (!owner_) {
        qDebug() << "owner_ is nullptr";
        return;
    }
    QSignalBlocker b1(end_frame_spin_);
    QSignalBlocker b2(end_time_code_);
    TimeCodeFrame timecode = qtime_to_timecode_frame(tc, owner_->getTimeCodeType());
    int newEnd = static_cast<int>(timecode_frame_to_frames(timecode, owner_->getTimeCodeType()));
    newEnd = clampEndFrame(newEnd);
    end_frame_spin_->setValue(newEnd);
    const auto clampedTc = frames_to_timecode_frame(newEnd, owner_->getTimeCodeType());
    end_time_code_->setTime(timecode_frame_to_qtime(clampedTc, owner_->getTimeCodeType()));
    owner_->setEnd(framesToUs(newEnd)); // UI 帧 → 模型 μs
    refreshFrameRanges();
}

void ClipTimePanel::updatePositionFrame(int frames) {
    if (!owner_) {
        qDebug() << "owner_ is nullptr";
        return;
    }
    const qint64 length_us = owner_->length(); // 模型 μs（长度不变量）
    QSignalBlocker b1(position_frame_spin_);
    QSignalBlocker b2(position_time_);
    position_frame_spin_->setValue(frames);
    auto tc = frames_to_timecode_frame(frames, owner_->getTimeCodeType());
    QTime time=timecode_frame_to_qtime(tc, owner_->getTimeCodeType());
    position_time_->setTime(time);
    const qint64 start_us = framesToUs(frames); // UI 帧 → 模型 μs
    owner_->setStart(start_us);
    owner_->setEnd(start_us + length_us);
}

void ClipTimePanel::updatePositionTimeCode(const QTime& tc) {
    if (!owner_) {
        qDebug() << "owner_ is nullptr";
        return;
    }
    const qint64 length_us = owner_->length(); // 模型 μs（长度不变量）
    QSignalBlocker b1(position_frame_spin_);
    QSignalBlocker b2(position_time_);
    TimeCodeFrame timecode = qtime_to_timecode_frame(tc, owner_->getTimeCodeType());
    int newEnd = static_cast<int>(timecode_frame_to_frames(timecode, owner_->getTimeCodeType()));
    position_frame_spin_->setValue(newEnd);
    const qint64 start_us = framesToUs(newEnd); // UI 帧 → 模型 μs
    owner_->setStart(start_us);
    owner_->setEnd(start_us + length_us);
}
