// ClipTimePanel.cpp
// 函数说明：实现 ClipTimePanel 构造逻辑，布局“时间属性”控件并保持原有功能：
// - 显示为 00:00:00:000（毫秒格式）
// - 文本框 textChanged 与 spinbox 一样调用 setStart/setEnd
// - 响应 AbstractClipModel 的 timelinePositionChanged/lengthChanged 信号更新 UI
// （2026-11-10 微秒原生：模型域 = μs；本面板 UI 域 = 帧数（IntDragValueWidget
//   int 值 + 多制式时间码），进出模型一律经 framesToUs/usToFrames 显示层换算）

#include "ClipTimePanel.hpp"
#include "AbstractClipModel.hpp"
#include "Common/AppConfig/TimeCodeDefines.h"
#include "TimeLineDefines.h" // framesToUs/usToFrames 显示层换算（μs 原生）
#include <QObject>
#include <QString>

ClipTimePanel::ClipTimePanel(AbstractClipModel* owner, QWidget* parent)
    : m_owner(owner),
    QGroupBox(parent)
{
    setTitle(tr("时间属性"));
    setFlat(true); // 带文字的分割线（2026-11-13 用户定案：四区块统一去框）
    m_timeLayout = new QGridLayout(this);

    // 开始帧显示
    auto startLabel = new QLabel(tr("开始帧:"), this);
    m_startFrameSpinBox = new IntDragValueWidget(this);
    m_startFrameSpinBox->setRange(0, 9999999);
    // m_startFrameSpinBox->setValue(m_owner->start());
    m_startTimeCodeLineEdit = new QTimeEdit(this);
    m_startTimeCodeLineEdit->setDisplayFormat("hh:mm:ss:zzz");
    m_timeLayout->addWidget(startLabel, 0, 0,1,1);
    m_timeLayout->addWidget(m_startFrameSpinBox, 0, 1,1,1);
    m_timeLayout->addWidget(m_startTimeCodeLineEdit, 0, 2,1,1);

    // 片段总长显示（绝对时间：毫秒）
    auto lengthLabel = new QLabel(tr("片段位置:"), this);
    m_positionFrameSpinBox = new IntDragValueWidget(this);
    m_positionFrameSpinBox->setRange(0, 9999999);
    m_positionTimeLineEdit = new QTimeEdit(this);
    m_positionTimeLineEdit->setDisplayFormat("hh:mm:ss:zzz");
    m_timeLayout->addWidget(lengthLabel, 1, 0,1,1);
    m_timeLayout->addWidget(m_positionFrameSpinBox, 1 ,1,1,1);
    m_timeLayout->addWidget(m_positionTimeLineEdit, 1, 2,1,1);

    // 结束帧显示（绝对时间：毫秒）
    auto endLabel = new QLabel(tr("结束帧:"), this);
    m_endFrameSpinBox = new IntDragValueWidget(this);
    m_endFrameSpinBox->setRange(0, 9999999);
    m_endTimeCodeLineEdit = new QTimeEdit(this);
    m_endTimeCodeLineEdit->setDisplayFormat("hh:mm:ss:zzz");
    m_timeLayout->addWidget(endLabel, 2, 0,1,1);
    m_timeLayout->addWidget(m_endFrameSpinBox, 2, 1,1,1);
    m_timeLayout->addWidget(m_endTimeCodeLineEdit, 2, 2,1,1);
    setMaximumHeight(150);
    // 模型 μs → UI 帧数换算后灌初值
    updateStartFrame(usToFrames(m_owner->start()));
    updateEndFrame(usToFrames(m_owner->end()));
    updatePositionFrame(usToFrames(m_owner->start()));
    initSignalConnections();
}
void ClipTimePanel::initSignalConnections() {
    connect(m_startFrameSpinBox,&IntDragValueWidget::valueChanged,this,&ClipTimePanel::updateStartFrame);
    connect(m_startTimeCodeLineEdit,&QTimeEdit::timeChanged,this,&ClipTimePanel::updateStartTimeCode);
    connect(m_endFrameSpinBox,&IntDragValueWidget::valueChanged,this,&ClipTimePanel::updateEndFrame);
    connect(m_endTimeCodeLineEdit,&QTimeEdit::timeChanged,this,&ClipTimePanel::updateEndTimeCode);
    connect(m_positionFrameSpinBox,&IntDragValueWidget::valueChanged,this,&ClipTimePanel::updatePositionFrame);
    connect(m_positionTimeLineEdit,&QTimeEdit::timeChanged,this,&ClipTimePanel::updatePositionTimeCode);
    // 模型信号参数 = μs（微秒原生）→ UI 帧数换算后走同一更新槽
    connect(m_owner, &AbstractClipModel::timelinePositionChanged, this, [this](qint64 us){
        updateStartFrame(usToFrames(us));
        updatePositionFrame(usToFrames(us));
    });
    connect(m_owner, &AbstractClipModel::lengthChanged, this, [this](qint64 usEnd){
       updateEndFrame(usToFrames(usEnd));
    });

}

int ClipTimePanel::clampStartFrame(int frames) const
{
    if (!m_owner || !m_owner->isResizable())
        return frames;
    if (frames < 0)
        frames = 0;
    const int maxStart = usToFrames(m_owner->end()) - 1; // 模型 μs → 帧
    if (frames > maxStart)
        frames = qMax(0, maxStart);
    return frames;
}

int ClipTimePanel::clampEndFrame(int frames) const
{
    if (!m_owner || !m_owner->isResizable())
        return frames;
    const int minEnd = usToFrames(m_owner->start()) + 1; // 模型 μs → 帧
    if (frames < minEnd)
        frames = minEnd;
    return frames;
}

void ClipTimePanel::refreshFrameRanges()
{
    if (!m_owner || !m_owner->isResizable() || !m_startFrameSpinBox || !m_endFrameSpinBox)
        return;
    QSignalBlocker blockStart(m_startFrameSpinBox);
    QSignalBlocker blockEnd(m_endFrameSpinBox);
    m_startFrameSpinBox->setRange(0, qMax(0, usToFrames(m_owner->end()) - 1));
    m_endFrameSpinBox->setRange(usToFrames(m_owner->start()) + 1, 9999999);
}

void ClipTimePanel::updateStartFrame(int frames) {
    if (!m_owner) {
        qDebug() << "m_owner is nullptr";
        return;
    }
    frames = clampStartFrame(frames);
    QSignalBlocker b1(m_startFrameSpinBox);
    QSignalBlocker b2(m_startTimeCodeLineEdit);
    m_startFrameSpinBox->setValue(frames);
    auto tc = frames_to_timecode_frame(frames, m_owner->getTimeCodeType());
    QTime time=timecode_frame_to_qtime(tc, m_owner->getTimeCodeType());
    m_startTimeCodeLineEdit->setTime(time);

    m_owner->setStart(framesToUs(frames)); // UI 帧 → 模型 μs
    refreshFrameRanges();
}

void ClipTimePanel::updateStartTimeCode(const QTime& tc) {
    if (!m_owner) {
        qDebug() << "m_owner is nullptr";
        return;
    }
    QSignalBlocker b1(m_startFrameSpinBox);
    QSignalBlocker b2(m_startTimeCodeLineEdit);
    TimeCodeFrame timecode = qtime_to_timecode_frame(tc, m_owner->getTimeCodeType());
    int newStart = static_cast<int>(timecode_frame_to_frames(timecode, m_owner->getTimeCodeType()));
    newStart = clampStartFrame(newStart);
    m_startFrameSpinBox->setValue(newStart);
    const auto clampedTc = frames_to_timecode_frame(newStart, m_owner->getTimeCodeType());
    m_startTimeCodeLineEdit->setTime(timecode_frame_to_qtime(clampedTc, m_owner->getTimeCodeType()));
    m_owner->setStart(framesToUs(newStart)); // UI 帧 → 模型 μs
    refreshFrameRanges();

}
void ClipTimePanel::updateEndFrame(int frames) {
    if (!m_owner) {
        return;
    }
    frames = clampEndFrame(frames);
    QSignalBlocker b1(m_endFrameSpinBox);
    QSignalBlocker b2(m_endTimeCodeLineEdit);
    m_endFrameSpinBox->setValue(frames);
    auto tc = frames_to_timecode_frame(frames, m_owner->getTimeCodeType());
    QTime time=timecode_frame_to_qtime(tc, m_owner->getTimeCodeType());
    m_endTimeCodeLineEdit->setTime(time);
    m_owner->setEnd(framesToUs(frames)); // UI 帧 → 模型 μs
    refreshFrameRanges();
}
void ClipTimePanel::updateEndTimeCode(const QTime& tc) {
    if (!m_owner) {
        qDebug() << "m_owner is nullptr";
        return;
    }
    QSignalBlocker b1(m_endFrameSpinBox);
    QSignalBlocker b2(m_endTimeCodeLineEdit);
    TimeCodeFrame timecode = qtime_to_timecode_frame(tc, m_owner->getTimeCodeType());
    int newEnd = static_cast<int>(timecode_frame_to_frames(timecode, m_owner->getTimeCodeType()));
    newEnd = clampEndFrame(newEnd);
    m_endFrameSpinBox->setValue(newEnd);
    const auto clampedTc = frames_to_timecode_frame(newEnd, m_owner->getTimeCodeType());
    m_endTimeCodeLineEdit->setTime(timecode_frame_to_qtime(clampedTc, m_owner->getTimeCodeType()));
    m_owner->setEnd(framesToUs(newEnd)); // UI 帧 → 模型 μs
    refreshFrameRanges();
}
void ClipTimePanel::updatePositionFrame(int frames) {
    if (!m_owner) {
        qDebug() << "m_owner is nullptr";
        return;
    }
    const qint64 length_us = m_owner->length(); // 模型 μs（长度不变量）
    QSignalBlocker b1(m_positionFrameSpinBox);
    QSignalBlocker b2(m_positionTimeLineEdit);
    m_positionFrameSpinBox->setValue(frames);
    auto tc = frames_to_timecode_frame(frames, m_owner->getTimeCodeType());
    QTime time=timecode_frame_to_qtime(tc, m_owner->getTimeCodeType());
    m_positionTimeLineEdit->setTime(time);
    const qint64 start_us = framesToUs(frames); // UI 帧 → 模型 μs
    m_owner->setStart(start_us);
    m_owner->setEnd(start_us + length_us);
}
void ClipTimePanel::updatePositionTimeCode(const QTime& tc) {
    if (!m_owner) {
        qDebug() << "m_owner is nullptr";
        return;
    }
    const qint64 length_us = m_owner->length(); // 模型 μs（长度不变量）
    QSignalBlocker b1(m_positionFrameSpinBox);
    QSignalBlocker b2(m_positionTimeLineEdit);
    TimeCodeFrame timecode = qtime_to_timecode_frame(tc, m_owner->getTimeCodeType());
    int newEnd = static_cast<int>(timecode_frame_to_frames(timecode, m_owner->getTimeCodeType()));
    m_positionFrameSpinBox->setValue(newEnd);
    const qint64 start_us = framesToUs(newEnd); // UI 帧 → 模型 μs
    m_owner->setStart(start_us);
    m_owner->setEnd(start_us + length_us);
}