#ifndef TIMELINETYPES_H
#define TIMELINETYPES_H
#include "QString"
#include <QMetaType>
#include <QtGlobal>
#include <QTime>
#include <QtCore/QMetaObject>
#include "BaseClassExport.h"
namespace QtTimeline
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
BASECLASS_EXPORT Q_NAMESPACE
#else
Q_NAMESPACE_EXPORT(BASECLASS_EXPORT)
#endif

    // hoverState：片段内鼠标悬停分区（视图边缘/中心命中 + 关键帧子元素）
    // KEYFRAME（2026-11-05）：悬停在片段模型的子元素（Tween 关键帧标记）上
    // ——经 AbstractClipModel::hitKeyframe 询问模型，默认不命中（无关键帧
    // 的片段/基类骨架恒 NONE/CENTER），故对既有插件零行为影响。
    enum  hoverState {LEFT,RIGHT,CENTER,KEYFRAME,NONE};

    // 自定义 data() role：显式 = Qt::UserRole 基编号（2026-11-10 路线 C）。
    // 【历史坑】原枚举从 0 起步与 Qt 标准 role 低段重叠（0=DisplayRole/
    // 2=EditRole/3=ToolTipRole…），ClipOscWidgetsRole 曾"恰好占位 3"避开
    // 冲突——删中间成员即撞车（C2196 实测）。显式编号后增删成员互不影响。
    // Qt::UserRole = 256，自定义 role 从 257 起（Qt 约定）。
    enum TimelineRoles {
        ClipIdRole = Qt::UserRole + 1, // 剪辑ID
        ClipInRole,                    // 剪辑开始时间（μs）
        ClipOutRole,                   // 剪辑结束时间（μs）
        ClipLengthRole,                // 剪辑长度（μs）
        ClipPosRole,                   // 剪辑位置（μs）
        ClipTypeRole,                  // 剪辑类型
        ClipShowWidgetRole,            // 剪辑是否显示小部件
        ClipResizableRole,             // 剪辑是否可调整大小
        ClipShowBorderRole,            // 剪辑是否显示边框
        ClipModelRole,                 // 用于获取完整的模型对象
        TrackTypeRole,                 // 轨道类型
        TrackNameRole,                 // 轨道名称
        TrackModelRole,                // 轨道模型
        TrackClipsCountRole,           // 轨道剪辑数量
        TrackClipsRole,                // 轨道剪辑列表
        TimelineLengthRole,            // 时间轴长度（μs）
        TimeCodeTypeRole,              // 时间码格式
        ClipColorRole,                 // 片段颜色
        ClipNoteRole,                  // 片段备注名
        TrackExpandedRole,             // 轨道展开态（2026-11-06 轨道展开：true = 行高扩展并内嵌 clip panel）
    };
    Q_ENUM_NS(TimelineRoles)

    using ClipId = unsigned int;

    // ===================== 时间基准（2026-11-10 路线 C：微秒原生） =====================
    // 时间轴库内所有时间量（start/end/length/播放头）单位 = 微秒（qint64），
    // 与 Scene 权威域同基——adapter 不再双向换算（原 framesToUs/usToFrames
    // 25fps 镜像已退役）。"帧"概念只在两处存活：
    //   1) ClipTimePanel 的帧数显示/编辑（显示层换算，用户可见行为不变）；
    //   2) TimeCodeDefines 的多制式时间码显示（WATCHOUT 语义）。
    inline constexpr qint64 kFrameUs = 40'000;   // PAL 25fps：1 帧时长（μs）
    inline constexpr qint64 kUsPerSecond = 1'000'000;

    // 帧数 → μs（显示层换算：ClipTimePanel 等以"帧"为 UI 语言的入口）
    inline constexpr qint64 framesToUs(int frames) { return frames * kFrameUs; }
    // μs → 帧数（四舍五入；显示层换算用）
    inline constexpr qint64 usToFrames(qint64 us_total) {
        return (us_total * 25 + 500'000) / 1'000'000;
    }
    // namespace TimecodeFormat {
    //     // 标准电影帧率
    //     constexpr int FILM = 24;           // 24 fps - 标准电影
    //     constexpr double FILM_DF = 23.976; // 24 fps - 标准电影 Drop Frame
    //     // 标准电视帧率
    //     constexpr int NTSC = 30;          // 29.97 fps - NTSC
    //     constexpr double NTSC_DF = 29.97;  // 29.97 fps - NTSC Drop Frame
    //     constexpr int PAL = 25;           // 25 fps - PAL/SECAM
    //     constexpr double PAL_DF = 24.975; // 25 fps - PAL/SECAM Drop Frame
    // }



    // 定义支持的文件类型列表
    inline QStringList VideoTypes = {"mp4", "avi", "mov", "wmv", "mkv", "flv"};
    inline QStringList AudioTypes = {"mp3", "wav", "aac", "ogg", "flac"};
    inline QStringList ImageTypes = {"jpg", "jpeg", "png", "bmp", "gif", "tiff"};
    inline QStringList ControlTypes = {"ilk"};
    // 将track类型转换为字符串
    static QString TrackTypeString(const QString& type) {
        return type;
    }
}

#endif // TIMELINETYPES_H
