#pragma once

#include <QObject>
#include <QMutex>
#include <QVector>
#include <thread>
#include <atomic>
#include "DataTypes/AudioTimestampRingQueue.h"
#include "TimeCodeDefines.h"
#include <ltc.h>
static const int SAMPLE_RATE = 48000;
static const size_t BUFFER_SIZE = SAMPLE_RATE/TimestampGenerator::getInstance()->getFrameRate();
namespace Nodes
{
    /**
     * @brief LTC生成工作线程类
     * 在独立线程中生成 LTC（Linear Timecode）波形，并写入共享音频环形缓冲区供下游消费
     */
    class LTCGeneratorWorker : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         * @param parent 父对象
         */
        explicit LTCGeneratorWorker(QObject *parent = nullptr);
        
        /**
         * @brief 析构函数
         */
        ~LTCGeneratorWorker();

        /**
         * @brief 获取输出音频缓冲区（用于 outData 建立连接）
         * @return 共享环形缓冲区指针
         */
        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer() const;

    public slots:
        /**
         * @brief 开始生成
         */
        void startProcessing();

        /**
         * @brief 停止生成
         */
        void stopProcessing();

        /**
         * @brief 时间码归零
         * - 重置 libltc 编码器时间码为 00:00:00:00
         * - 清空待输出缓存与帧队列
         * - 清空输出环形队列，避免下游继续解码旧数据
         */
        void resetTimecode();

        /**
         * @brief 设置是否启用生成（不启用时输出静音）
         * @param enabled 启用开关
         */
        void setEnabled(bool enabled);

        void setTimeCodeType(TimeCodeType type);

        void setVolume(float db);

        /**
         * @brief 系统时间戳驱动的消费回调
         * - 从缓存中读取音频数据
         * - 打上系统时间戳
         * - 放入环形队列
         */
        void onSystemFrameTick(qint64 frameCount);

    signals:
        /**
         * @brief 新的时间码帧已生成（用于 UI 显示）
         * @param frame 时间码帧
         */
        void timeCodeFrameGenerated(TimeCodeFrame frame);
        
        /**
         * @brief 生成状态变化信号
         * @param isProcessing 是否正在生成
         */
        void processingStatusChanged(bool isProcessing);

    private:
        /**
         * @brief 独立线程生成循环
         */
        void generationLoop();

        /**
         * @brief 确保编码器已按当前配置初始化
         */
        void ensureEncoderLocked();

        /**
         * @brief 根据当前 LTC 编码器状态生成一个 LTC 帧的音频样本，并追加到待输出缓冲
         * @return 生成并追加的样本数
         */
        int encodeOneLtcFrameLocked();

        /**
         * @brief 从待输出缓冲中取出指定样本数，若不足则持续编码补足
         * @param samplesNeeded 需要的样本数
         * @param out 输出样本容器（写入 samplesNeeded 个 float）
         */
        void takeSamplesLocked(int samplesNeeded, QVector<float>& out);

        /**
         * @brief 将 SMPTETimecode 转换为 TimeCodeFrame（用于 UI）
         * @param t SMPTE 时间码
         * @return TimeCodeFrame
         */
        TimeCodeFrame toTimeCodeFrameLocked(const SMPTETimecode& t) const;
        void advancePlaybackFramesLocked(int consumedSamples);

    private:
        std::shared_ptr<AudioTimestampRingQueue> _outputBuffer;         ///< 输出音频缓冲区（写侧）
        mutable QMutex _mutex;                                          ///< 互斥锁
        std::atomic<bool> _isProcessing{false};                         ///< 是否正在生成
        std::atomic<bool> _running{false};                              ///< 线程运行标志
        std::unique_ptr<std::thread> _generationThread;                 ///< 独立生成线程
        bool _enabled = true;                                           ///< 是否输出 LTC（否则输出静音）
        float _volumeDb = 18.0f;
        
        LTCEncoder* _encoder = nullptr;                                 ///< libltc 编码器
        SMPTETimecode _timecode {};                                     ///< 当前时间码（由编码器推进）
        double _fps = 0.0;                                              ///< 当前 LTC 帧率（默认跟随全局帧率）
        bool _typeOverride = false;
        TimeCodeType _forcedType = TimeCodeType::PAL;

        QVector<float> _pendingSamples;                                 ///< 音频缓存（float32）
        int _pendingReadOffset = 0;                                     ///< 待输出样本读偏移（避免频繁移除头部）

        QVector<int> _frameSampleCounts;                                ///< 每个编码帧的样本数队列
        QVector<SMPTETimecode> _frameTimecodes;                         ///< 与样本队列对应的时间码快照
        int _frameQueueHead = 0;                                        ///< 队列头索引
        int _frameHeadConsumed = 0;                                     ///< 头帧已消费的样本数
    };
}
