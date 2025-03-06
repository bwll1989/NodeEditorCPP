#ifndef TIMELINETYPES_H
#define TIMELINETYPES_H
#include "QString"
#include <QMetaType>
#include <QtGlobal>
#include <QTime>

enum  hoverState {LEFT,RIGHT,NONE};
enum class ItemType {
    Track, // 轨道
    Clip, // 剪辑
    Invalid // 无效
};

enum TimelineRoles {
    ClipInRole, // 剪辑开始时间
    ClipOutRole, // 剪辑结束时间
    ClipPosRole, //剪辑位置
    ClipTimeInRole, // 剪辑时间开始
    ClipTimeOutRole, // 剪辑时间结束
    ClipLengthRole, // 剪辑长度
    ClipTypeRole, // 剪辑类型
    ClipTitle, // 剪辑标题
    ClipShowWidgetRole, // 剪辑是否显示小部件
    ClipResizableRole, // 剪辑是否可调整大小
    ClipShowBorderRole, // 剪辑是否显示边框
    ClipModelRole,  // 用于获取完整的模型对象
    TrackNumberRole, // 轨道编号
    TrackTypeRole, // 轨道类型
    TrackNameRole, // 轨道名称
    TimelineLengthRole, // 时间轴长度
   
};

namespace TimecodeFormat {
    // 标准电影帧率
    constexpr int FILM = 24;           // 24 fps - 标准电影
    
    // 标准电视帧率
    constexpr int NTSC = 30;          // 29.97 fps - NTSC
    constexpr double NTSC_DF = 29.97;  // 29.97 fps - NTSC Drop Frame
    constexpr int PAL = 25;           // 25 fps - PAL/SECAM
    
    // 高帧率格式
    constexpr int HD_50 = 50;         // 50 fps - HD
    constexpr int HD_60 = 60;         // 59.94 fps - HD
    constexpr double HD_60_DF = 59.94; // 59.94 fps - HD Drop Frame
    
    constexpr int HD_100 = 100;     // 100 fps - HD
    // 游戏和高刷新率显示器
    constexpr int GAME_120 = 120;     // 120 fps
    constexpr int GAME_144 = 144;     // 144 fps
    constexpr int GAME_240 = 240;     // 240 fps
}

// 时间码格式枚举
enum class TimecodeType {
    Film,       // 24fps
    NTSC,       // 30fps
    NTSC_DF,    // 29.97fps drop frame
    PAL,        // 25fps
    HD_50,      // 50fps
    HD_60,      // 60fps
    HD_60_DF,   // 59.94fps drop frame
    HD_100,     // 100fps
    GAME_120,   // 120fps
    GAME_144,   // 144fps
    GAME_240,   // 240fps
};

// 时间显示格式
enum class TimedisplayFormat {
    TimeCodeFormat, // 时间码
    AbsoluteTimeFormat, // 绝对时间
};
// SMPTE Timecode 类
class Timecode {
public:
    Timecode() : hours(0), minutes(0), seconds(0), frames(0) {}
    
    Timecode(int h, int m, int s, int f) 
        : hours(h), minutes(m), seconds(s), frames(f) {}
    
    // 从帧数和帧率构造
    static Timecode fromFrames(qint64 totalFrames, int frameRate) {
        Timecode tc;
        if (frameRate <= 0) return tc;
        
        // 计算时、分、秒、帧
        int framesPerSecond = frameRate;
        int framesPerMinute = framesPerSecond * 60;
        int framesPerHour = framesPerMinute * 60;
        
        tc.hours = totalFrames / framesPerHour;
        totalFrames %= framesPerHour;
        
        tc.minutes = totalFrames / framesPerMinute;
        totalFrames %= framesPerMinute;
        
        tc.seconds = totalFrames / framesPerSecond;
        tc.frames = totalFrames % framesPerSecond;
        
        return tc;
    }
    // 从帧数和时间码类型构造
    static Timecode fromFrames(qint64 totalFrames, TimecodeType timecodeType) {
        return fromFrames(totalFrames, getFrameRate(timecodeType));
    }
    // 添加 Drop Frame 支持的构造函数
    static Timecode fromFramesDF(qint64 totalFrames, double frameRate) {
        Timecode tc;
        if (frameRate <= 0) return tc;

        // Drop Frame 时间码的特殊处理
        if (frameRate == TimecodeFormat::NTSC_DF || frameRate == TimecodeFormat::HD_60_DF) {
            // Drop Frame 计算逻辑
            int dropFrames = (frameRate == TimecodeFormat::NTSC_DF) ? 2 : 4;
            qint64 framesPerMinute = qRound(frameRate * 60);
            qint64 dropFramesPerMinute = dropFrames * 9;
            
            qint64 d = totalFrames / framesPerMinute;
            qint64 m = totalFrames % framesPerMinute;

            totalFrames += dropFramesPerMinute * d;
            if (m > dropFrames)
                totalFrames += dropFrames;
        }

        return fromFrames(totalFrames, qRound(frameRate));
    }

    // 获取指定格式的帧率
    static double getFrameRate(TimecodeType type) {
        switch (type) {
            case TimecodeType::Film:     return TimecodeFormat::FILM;
            case TimecodeType::NTSC:     return TimecodeFormat::NTSC;
            case TimecodeType::NTSC_DF:  return TimecodeFormat::NTSC_DF;
            case TimecodeType::PAL:      return TimecodeFormat::PAL;
            case TimecodeType::HD_50:    return TimecodeFormat::HD_50;
            case TimecodeType::HD_60:    return TimecodeFormat::HD_60;
            case TimecodeType::HD_60_DF: return TimecodeFormat::HD_60_DF;
            case TimecodeType::HD_100:   return TimecodeFormat::HD_100;
            case TimecodeType::GAME_120: return TimecodeFormat::GAME_120;
            case TimecodeType::GAME_144: return TimecodeFormat::GAME_144;
            case TimecodeType::GAME_240: return TimecodeFormat::GAME_240;
            default:                     return 0.0;
        }
    }
    // 转换为总帧数
    qint64 toFrames(int frameRate) const {
        if (frameRate <= 0) return 0;
        return ((hours * 3600 + minutes * 60 + seconds) * frameRate + frames);
    }
    
    // 转换为字符串
    QString toString() const {
        return QString("%1:%2:%3:%4")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(frames, 2, 10, QChar('0'));
    }
    
    // 从字符串解析
    static Timecode fromString(const QString& str, int frameRate) {
        Timecode tc;
        QStringList parts = str.split(':');
        if (parts.size() == 4) {
            tc.hours = parts[0].toInt();
            tc.minutes = parts[1].toInt();
            tc.seconds = parts[2].toInt();
            tc.frames = parts[3].toInt();
            
            // 验证值的范围
            tc.normalize(frameRate);
        }
        return tc;
    }
    
    // 转换为带有 Drop Frame 标记的字符串
    QString toStringDF() const {
        return QString("%1:%2:%3:%4DF")  // 注意这里使用分号表示 Drop Frame
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(frames, 2, 10, QChar('0'));
    }

    // 从带有 Drop Frame 标记的字符串解析
    static Timecode fromStringDF(const QString& str, double frameRate) {
        Timecode tc;
        QStringList parts = str.split(':'); // 支持冒号和分号分隔
        if (parts.size() == 4) {
            tc.hours = parts[0].toInt();
            tc.minutes = parts[1].toInt();
            tc.seconds = parts[2].toInt();
            tc.frames = parts[3].toInt();
            
            tc.normalize(qRound(frameRate));
        }
        return tc;
    }
    // 规范化时间值
    void normalize(int frameRate) {
        if (frameRate <= 0) return;
        
        // 处理帧溢出
        if (frames >= frameRate) {
            seconds += frames / frameRate;
            frames %= frameRate;
        }
        
        // 处理秒溢出
        if (seconds >= 60) {
            minutes += seconds / 60;
            seconds %= 60;
        }
        
        // 处理分钟溢出
        if (minutes >= 60) {
            hours += minutes / 60;
            minutes %= 60;
        }
    }
    // 比较运算符
    bool operator==(const Timecode& other) const {
        return hours == other.hours && 
               minutes == other.minutes && 
               seconds == other.seconds && 
               frames == other.frames;
    }
    
    bool operator<(const Timecode& other) const {
        if (hours != other.hours) return hours < other.hours;
        if (minutes != other.minutes) return minutes < other.minutes;
        if (seconds != other.seconds) return seconds < other.seconds;
        return frames < other.frames;
    }

public:
    int hours;
    int minutes;
    int seconds;
    int frames;
};

// 帮助函数：将帧数转换为时间码字符串
inline QString FramesToTimeString(int frames, int frameRate) {
    return Timecode::fromFrames(frames, frameRate).toString();
}

// 帮助函数：将时间码字符串转换为帧数
inline int TimeStringToFrames(const QString& timeStr, int frameRate) {
    return Timecode::fromString(timeStr, frameRate).toFrames(frameRate);
}

// 注册为 Qt 元类型
Q_DECLARE_METATYPE(Timecode)

// 将track类型转换为字符串
static QString TrackTypeString(const QString& type) {
    return type;
}   


#endif // TIMELINETYPES_H
