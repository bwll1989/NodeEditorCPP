#ifndef TYPES_H
#define TYPES_H
#include "QString"
enum class MediaType {VIDEO,AUDIO,CONTROL};
enum  hoverState {LEFT,RIGHT,NONE};
enum class ItemType {
    Track,
    Clip,
    Invalid
};
enum TimelineRoles {

    ClipInRole,
    ClipOutRole,
    ClipPosRole,
    ClipLengthRole,
    ClipTypeRole,
    ClipTitle,
    TrackNumberRole,
    TrackTypeRole,
    TimelineLengthRole,
    SelectedRole,
    InRole,
    OutRole

};

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

#endif // TYPES_H
