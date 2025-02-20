#ifndef TIMELINETYPES_H
#define TIMELINETYPES_H
#include "QString"

enum  hoverState {LEFT,RIGHT,NONE};
enum class ItemType {
    Track, // 轨道
    Clip, // 剪辑
    Invalid // 无效
};

enum TimelineRoles {
    ClipInRole, // 剪辑开始时间
    ClipOutRole, // 剪辑结束时间
    ClipLengthRole, // 剪辑长度
    ClipTypeRole, // 剪辑类型
    ClipTitle, // 剪辑标题
    ClipShowWidgetRole, // 剪辑是否显示小部件
    ClipResizableRole, // 剪辑是否可调整大小
    ClipShowBorderRole, // 剪辑是否显示边框
    TrackNumberRole, // 轨道编号
    TrackTypeRole, // 轨道类型
    TimelineLengthRole, // 时间轴长度
};

// 将帧数转换为时间字符串
static QString FramesToTimeString(int frames, double fps)
{
    int totalMilliseconds = static_cast<int>((frames / fps) * 1000);
    int hours = totalMilliseconds / (1000 * 60 * 60);
    int minutes = (totalMilliseconds / (1000 * 60)) % 60;
    int seconds = (totalMilliseconds / 1000) % 60;
    int milliseconds = totalMilliseconds % 1000;

    return QString("%1:%2:%3.%4")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(milliseconds, 3, 10, QChar('0'));
}

// 将track类型转换为字符串
static QString TrackTypeString(const QString& type) {
    return type;
}   

#endif // TIMELINETYPES_H
