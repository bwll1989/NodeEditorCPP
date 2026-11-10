#ifndef MTCSENDER_H
#define MTCSENDER_H

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QElapsedTimer>

#include <vector>

#include <RtMidi.h>

#include "Common/AppConfig/TimeCodeDefines.h"
#include "Common/DataTypes/SyncData.h"

/**
 * @brief MTC 生成器：封装 RtMidiOut，按 4×帧率 发送 Quarter-Frame 消息
 *
 * - 播放启动/跳转/停止时先发一条 Full-Frame SysEx 定位，再进入 QF 流
 * - 定时基于 QElapsedTimer 累计耗时折算应发送的 QF 数（追赶式），
 *   不依赖 Windows 定时器精度，长期走带不漂移
 * - QF piece 0~3 携带 2 帧窗口起始时间，piece 4~7 携带 +1 帧时间（按 MTC 规范）
 */
class MtcSender : public QObject {
    Q_OBJECT

public:
    explicit MtcSender(QObject *parent = nullptr);
    ~MtcSender() override;

    /** @brief 枚举系统当前可用的 MIDI 输出端口，返回 "索引: 名称" 列表 */
    static QStringList availableDevices();

    /** @brief 当前是否正在发送 QF 流 */
    bool isPlaying() const { return _playing; }

public slots:
    /** @brief 打开输出端口并设置帧率类型（未开始播放，不发任何消息） */
    void start(const QString &deviceName, TimeCodeType type);

    /** @brief 关闭输出端口 */
    void stop();

    /** @brief 设置起始时间码（播放中调用会立即发 Full-Frame 定位并重新对齐 QF 窗口） */
    void setTimecode(const TimeCodeFrame &frame);

    /** @brief 播放/暂停。true：发 Full-Frame 后开始 QF 流；false：停止 QF */
    void setPlaying(bool playing);

    /** @brief 发送一条 Full-Frame 定位（不改变播放状态） */
    void sendFullFrame();

    /**
     * @brief 绑定同步源（SYNC→MTC 跟随模式）
     * tick 每轮读源快照：源播放中按全局时钟外推期望位置，
     * 帧差超过死区（1帧）时重新对齐（Full-Frame + QF 重置）；
     * 源暂停/失效时停发 QF 并定格。
     * 未绑定则保持原有自由生成行为。
     */
    void setSyncSource(std::shared_ptr<NodeDataTypes::SyncSharedState> src);

    /** @brief 设置 SYNC 秒 → MTC 时间码的映射偏移（秒，可负） */
    void setSyncOffsetSec(double sec) { _syncOffsetSec = sec; }

signals:
    /** @brief 每推进完整 1 帧（4 个 QF）发一次，供 UI 显示（显示的是当前线上帧号） */
    void timeCodeFrameGenerated(TimeCodeFrame frame);

    /** @brief 状态变化（error=true 显示为红色；如 ready/stopped/设备错误） */
    void statusChanged(bool error, QString message);

private slots:
    void tick();

private:
    void sendQuarterFrame(int piece, int value);
    int rateBits() const;
    TimeCodeFrame windowTime(qint64 qfIndex) const; ///< 第 qfCount 个 QF 所属的编码时间
    void followSync();

    RtMidiOut *_midiOut {nullptr};
    QTimer *_timer {nullptr};
    TimeCodeFrame _base {0, 0, 0, 0, TimeCodeType::PAL}; ///< QF 流起始时间码
    TimeCodeType _type {TimeCodeType::PAL};
    qint64 _qfCount {0};          ///< 已发送的 QF 总数
    bool _playing {false};
    QString _deviceName;
    QElapsedTimer _clock;
    std::vector<unsigned char> _msgBuffer; ///< 发送缓冲（复用避免每条消息堆分配，仅主线程 tick 使用）

    // ---- SYNC→MTC 跟随（SyncData 体系）----
    std::shared_ptr<NodeDataTypes::SyncSharedState> _syncSource; ///< 外部同步源（未绑定=自由生成）
    double _syncOffsetSec {0.0};   ///< SYNC 秒 → MTC 时间码偏移
    bool _syncHold {false};        ///< 源暂停/失效：停发 QF、定格
};

#endif // MTCSENDER_H
