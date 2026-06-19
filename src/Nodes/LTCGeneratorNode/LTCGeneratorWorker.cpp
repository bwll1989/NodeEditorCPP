#include "LTCGeneratorWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QDebug>
#include <QtCore/QMetaObject>
#include <cmath>
#include <cstring>
#include <chrono>

// 本文件实现：
// 1) 独立时钟生成 LTC（不依赖 QTimer），通过动态休眠使循环周期与 FPS 匹配
// 2) 以“帧”为单位推进 UI 时间码：仅在消费完整帧样本后才更新，避免 UI 领先实际音频
// 3) 输出 float32 单声道音频，环形队列中带系统时间戳供下游使用

namespace
{
    struct LtcEncoderConfig {
        double fps = 25.0;
        LTC_TV_STANDARD standard = LTC_TV_FILM_24;
    };

    LtcEncoderConfig resolveLtcEncoderConfig(TimeCodeType type)
    {
        LtcEncoderConfig cfg;

        switch (type) {
            case TimeCodeType::Film:    cfg.fps = 24.0; break;
            case TimeCodeType::Film_DF: cfg.fps = 23.976; break;
            case TimeCodeType::NTSC:    cfg.fps = 30.0; break;
            case TimeCodeType::NTSC_DF: cfg.fps = 29.97; break;
            case TimeCodeType::PAL:
            case TimeCodeType::HD:      cfg.fps = 25.0; break;
            default:                    cfg.fps = 25.0; break;
        }

        cfg.standard = LTC_TV_FILM_24;
        if (std::fabs(cfg.fps - 25.0) < 0.5) {
            cfg.standard = LTC_TV_625_50;
        } else if (std::fabs(cfg.fps - 30.0) < 0.6 || std::fabs(cfg.fps - 29.97) < 0.6) {
            cfg.standard = LTC_TV_525_60;
        }

        return cfg;
    }
}

namespace Nodes
{
    /**
     * @brief 构造函数
     * - 初始化输出缓冲区
     * - 准备默认时间码（从 00:00:00:00 开始）
     */
    LTCGeneratorWorker::LTCGeneratorWorker(QObject *parent)
        : QObject(parent)
        , _outputBuffer(std::make_shared<AudioTimestampRingQueue>())
    {
        std::memset(&_timecode, 0, sizeof(_timecode));
        strncpy_s(_timecode.timezone, sizeof(_timecode.timezone), "+0000", _TRUNCATE);
    }

    /**
     * @brief 析构函数
     * - 停止生成
     * - 释放 libltc 编码器资源
     */
    LTCGeneratorWorker::~LTCGeneratorWorker()
    {
        stopProcessing();

        QMutexLocker locker(&_mutex);
        if (_encoder) {
            ltc_encoder_free(_encoder);
            _encoder = nullptr;
        }
    }

    /**
     * @brief 获取输出缓冲区
     * @return 共享环形缓冲区指针
     */
    std::shared_ptr<AudioTimestampRingQueue> LTCGeneratorWorker::getOutputBuffer() const
    {
        QMutexLocker locker(&_mutex);
        return _outputBuffer;
    }

    /**
     * @brief 开始生成
     * - 连接 TimestampGenerator 的统一时钟信号
     */
    void LTCGeneratorWorker::startProcessing()
    {
        QMutexLocker locker(&_mutex);

        if (_isProcessing) {
            return;
        }

        _isProcessing = true;
        // 使用当前全局帧计数作为起始点
        // 启动生成时重置样本缓存与“帧队列”，避免历史状态导致时间码跳变
        _pendingSamples.clear();
        _pendingReadOffset = 0;
        _frameSampleCounts.clear();    // 每个编码帧对应的样本数
        _frameTimecodes.clear();       // 编码时刻的时间码快照（与样本数一一对应）
        _frameQueueHead = 0;           // 消费队列头索引
        _frameHeadConsumed = 0;        // 头帧已消费样本数，用于判断“整帧已播放”

        if (_outputBuffer) {
            _outputBuffer->clear();
            _outputBuffer->setActive(true);
        }

        ensureEncoderLocked();

        // 启动独立生成线程
        _running = true;
        _generationThread = std::make_unique<std::thread>(&LTCGeneratorWorker::generationLoop, this);

        // 连接系统时间戳信号
        connect(TimestampGenerator::getInstance(), &TimestampGenerator::frameCountUpdated, 
                this, &LTCGeneratorWorker::onSystemFrameTick, Qt::QueuedConnection);
        
        emit processingStatusChanged(true);
    }

    /**
     * @brief 停止生成
     * - 断开统一时钟信号
     */
    void LTCGeneratorWorker::stopProcessing()
    {
        // 先停止线程
        _running = false;
        if (_generationThread && _generationThread->joinable()) {
            _generationThread->join();
        }
        _generationThread.reset();

        // // // 断开信号连接
        disconnect(TimestampGenerator::getInstance(), &TimestampGenerator::frameCountUpdated,
                   this, &LTCGeneratorWorker::onSystemFrameTick);

        QMutexLocker locker(&_mutex);

        if (!_isProcessing) {
            return;
        }

        _isProcessing = false;
        emit processingStatusChanged(false);
    }

    /**
     * @brief 设置是否启用生成
     * @param enabled 启用开关（false 时输出静音，但仍保持时间戳推进）
     */
    void LTCGeneratorWorker::setEnabled(bool enabled)
    {
        QMutexLocker locker(&_mutex);
        _enabled = enabled;
    }

    void LTCGeneratorWorker::setTimeCodeType(TimeCodeType type)
    {
        QMutexLocker locker(&_mutex);
        if (type == TimeCodeType::HD) {
            type = TimeCodeType::PAL;
        }
        _forcedType = type;

        if (_encoder) {
            ltc_encoder_free(_encoder);
            _encoder = nullptr;
        }
        _configuredFps = 0.0;
        _configuredStandard = LTC_TV_FILM_24;
        _pendingSamples.clear();
        _pendingReadOffset = 0;
        _frameSampleCounts.clear();
        _frameTimecodes.clear();
        _frameQueueHead = 0;
        _frameHeadConsumed = 0;
        std::memset(&_timecode, 0, sizeof(_timecode));
        strncpy_s(_timecode.timezone, sizeof(_timecode.timezone), "+0000", _TRUNCATE);
    }

    /**
     * @brief 时间码归零
     * - 重置编码器时间码到 00:00:00:00
     * - 清空待输出样本与帧队列，避免旧数据残留导致跳变
     * - 清空输出环形队列，保证下游从新时间码开始解码
     */
    void LTCGeneratorWorker::resetTimecode()
    {
        QMutexLocker locker(&_mutex);

        SMPTETimecode zero;
        std::memset(&zero, 0, sizeof(zero));
        strncpy_s(zero.timezone, sizeof(zero.timezone), "+0000", _TRUNCATE);

        _timecode = zero;

        ensureEncoderLocked();
        syncEncoderFromTimecodeLocked();

        _pendingSamples.clear();
        _pendingReadOffset = 0;
        _frameSampleCounts.clear();
        _frameTimecodes.clear();
        _frameQueueHead = 0;
        _frameHeadConsumed = 0;

        if (_outputBuffer) {
            _outputBuffer->clear();
        }

        emit timeCodeFrameGenerated(toTimeCodeFrameLocked(zero));
    }

    void LTCGeneratorWorker::setVolume(float db)
    {
        QMutexLocker locker(&_mutex);
        _volumeDb = db;
        
        // Reset debug print flag when volume changes significantly
        static float lastVol = -999.0f;
        if (std::abs(db - lastVol) > 1.0f) {
            // Can't easily access the static bool inside encodeOneLtcFrameLocked without changing scope or making it member
            // But we can add a member or just rely on the first print.
            // Let's add a simple debug here
            qDebug() << "LTC Volume set to:" << db << "dB";
            lastVol = db;
        }
    }

    /**
     * @brief 确保编码器初始化
     * - 默认使 LTC 帧率跟随全局帧率（TimestampGenerator）
     * - 根据帧率选择最接近的 TV standard
     */
    void LTCGeneratorWorker::applyDropFrameFlagLocked()
    {
        if (!_encoder) {
            return;
        }

        LTCFrame frame;
        ltc_encoder_get_frame(_encoder, &frame);
        const bool useDropFrame =
            (_forcedType == TimeCodeType::NTSC_DF || _forcedType == TimeCodeType::Film_DF);
        frame.dfbit = useDropFrame ? 1 : 0;
        ltc_encoder_set_frame(_encoder, &frame);
    }

    void LTCGeneratorWorker::syncEncoderFromTimecodeLocked()
    {
        if (!_encoder) {
            return;
        }

        ltc_encoder_set_timecode(_encoder, &_timecode);
        ltc_encoder_reset(_encoder);
        applyDropFrameFlagLocked();
    }

    void LTCGeneratorWorker::ensureEncoderLocked()
    {
        const LtcEncoderConfig cfg = resolveLtcEncoderConfig(_forcedType);
        _fps = cfg.fps;

        if (_encoder) {
            if (std::fabs(_configuredFps - cfg.fps) < 1e-6 && _configuredStandard == cfg.standard) {
                return;
            }

            if (ltc_encoder_reinit(_encoder, SAMPLE_RATE, cfg.fps, cfg.standard, 0) != 0) {
                ltc_encoder_free(_encoder);
                _encoder = nullptr;
            } else {
                _configuredFps = cfg.fps;
                _configuredStandard = cfg.standard;
                ltc_encoder_set_volume(_encoder, 0.0);
                ltc_encoder_set_filter(_encoder, 0.0);
                syncEncoderFromTimecodeLocked();
                return;
            }
        }

        _encoder = ltc_encoder_create(SAMPLE_RATE, cfg.fps, cfg.standard, 0);
        if (_encoder) {
            _configuredFps = cfg.fps;
            _configuredStandard = cfg.standard;
            ltc_encoder_set_volume(_encoder, 0.0);
            ltc_encoder_set_filter(_encoder, 0.0);
            syncEncoderFromTimecodeLocked();
        }
    }

    /**
     * @brief 编码一个 LTC 帧并追加到待输出样本缓冲
     * @return 追加的样本数
     */
    int LTCGeneratorWorker::encodeOneLtcFrameLocked()
    {
        ensureEncoderLocked();
        if (!_encoder) {
            return 0;
        }

        ltc_encoder_encode_frame(_encoder);

        ltcsnd_sample_t* buf = nullptr;
        const int sampleCount = ltc_encoder_get_bufferptr(_encoder, &buf, 1);
        if (sampleCount <= 0 || !buf) {
            ltc_encoder_inc_timecode(_encoder);
            ltc_encoder_get_timecode(_encoder, &_timecode);
            return 0;
        }
        // 记录本次编码帧的样本数与时间码快照
        SMPTETimecode tcSnapshot;
        ltc_encoder_get_timecode(_encoder, &tcSnapshot);
        _frameSampleCounts.push_back(sampleCount);
        _frameTimecodes.push_back(tcSnapshot);

        const int oldSize = _pendingSamples.size();
        _pendingSamples.resize(oldSize + sampleCount);
        float* dst = _pendingSamples.data() + oldSize;
        
        // 计算线性增益 (限制在 +/- 120dB 以防止溢出，并处理 1024dB 的极端情况)
        float safeVolumeDb = std::min(120.0f, std::max(-120.0f, _volumeDb));
        float linearGain = std::pow(10.0f, safeVolumeDb / 20.0f);

        // 如果用户设置了极端的音量，输出日志警告
        if (_volumeDb > 120.0f) {
            static bool warned = false;
            if (!warned) {
                qWarning() << "LTC Volume is extremely high (" << _volumeDb << "dB). Clamping to 120dB to prevent overflow.";
                warned = true;
            }
        }

        // 调试：打印前几个样本的原始值和增益后的值，仅在第一次处理时
        static bool debugPrinted = false;
        if (!debugPrinted && sampleCount > 0) {
             int v = static_cast<int>(buf[0]) - 128;
             float f = (static_cast<float>(v) / 128.0f) * linearGain;
             qDebug() << "LTC Debug: Raw[0]=" << (int)buf[0] << " Val=" << v << " Gain=" << linearGain << " Result=" << f;
             debugPrinted = true;
        }

        // 将 libltc 输出的无符号8位PCM（中心值128）转换为 float32 [-1,1]，并应用线性增益与限幅
        for (int i = 0; i < sampleCount; ++i) {
            int v = static_cast<int>(buf[i]) - 128;
            // 应用增益并限幅
            float f = (static_cast<float>(v) / 128.0f) * linearGain;
            if (f > 1.0f) f = 1.0f;
            if (f < -1.0f) f = -1.0f;
            dst[i] = f;
        }

        ltc_encoder_inc_timecode(_encoder);
        ltc_encoder_get_timecode(_encoder, &_timecode);

        return sampleCount;
    }

    /**
     * @brief 从待输出缓冲取出 samplesNeeded 个样本
     * @param samplesNeeded 需要的样本数
     * @param out 输出样本（写入 samplesNeeded 个 float）
     */
    void LTCGeneratorWorker::takeSamplesLocked(int samplesNeeded, QVector<float>& out)
    {
        out.resize(samplesNeeded);
        if (samplesNeeded <= 0) {
            return;
        }

        if (!_enabled) {
            std::fill(out.begin(), out.end(), 0.0f);
            return;
        }

        while ((_pendingSamples.size() - _pendingReadOffset) < samplesNeeded) {
            if (encodeOneLtcFrameLocked() <= 0) {
                break;
            }
        }

        const int available = _pendingSamples.size() - _pendingReadOffset;
        const int toCopy = std::min(samplesNeeded, available);
        if (toCopy > 0) {
            // 从待输出缓存拷贝已就绪的样本到 block，并推进读偏移
            std::memcpy(out.data(),
                        _pendingSamples.constData() + _pendingReadOffset,
                        static_cast<size_t>(toCopy) * sizeof(float));
            _pendingReadOffset += toCopy;
            // 按消费掉的样本数推进“帧队列”，当完整跨过一帧边界时才发射 UI 时间码
            advancePlaybackFramesLocked(toCopy);
        }
        if (toCopy < samplesNeeded) {
            std::fill(out.begin() + toCopy, out.end(), 0.0f);
        }

        if (_pendingReadOffset > 4096 && _pendingReadOffset > (_pendingSamples.size() / 2)) {
            _pendingSamples = _pendingSamples.mid(_pendingReadOffset);
            _pendingReadOffset = 0;
        }
        if (_frameQueueHead > 64) {
            _frameSampleCounts = _frameSampleCounts.mid(_frameQueueHead);
            _frameTimecodes = _frameTimecodes.mid(_frameQueueHead);
            _frameQueueHead = 0;
        }
    }

    /**
     * @brief SMPTETimecode -> TimeCodeFrame
     * @param t SMPTE 时间码
     * @return TimeCodeFrame
     */
    TimeCodeFrame LTCGeneratorWorker::toTimeCodeFrameLocked(const SMPTETimecode& t) const
    {
        TimeCodeFrame frame;
        frame.hours = t.hours;
        frame.minutes = t.mins;
        frame.seconds = t.secs;
        frame.frames = t.frame;

        frame.type = _forcedType;
        return frame;
    }

    /**
     * @brief 根据当前全局时间戳帧率计算 LTC 输出缓冲块大小
     * @return 每次系统节拍需要输出的采样数
     */
    int LTCGeneratorWorker::getBufferSize() const
    {
        return TimestampGenerator::getInstance()->getSamplesPerFrame(SAMPLE_RATE);
    }

    /**
     * @brief 独立线程生成循环
     * - 持续监测音频缓存，不足时生成新的 LTC 音频帧
     * - 独立时钟源，根据 LTC 格式的 FPS 生成
     */
    void LTCGeneratorWorker::generationLoop()
    {
        while (_running) {
            auto start = std::chrono::steady_clock::now();
            double globalFps = TimestampGenerator::getInstance()->getFrameRate();

            {
                QMutexLocker locker(&_mutex);
                if (_isProcessing) {
                    ensureEncoderLocked();
                    // 预填缓冲：按 TimestampGenerator 每块采样数（如 2048@23.4375fps）保持约 1 块余量
                    const int pendingTarget = getBufferSize();
                    if ((_pendingSamples.size() - _pendingReadOffset) < pendingTarget) {
                        encodeOneLtcFrameLocked();
                    }
                }
            }

            // 预填节奏跟随全局时钟帧率，而非 LTC SMPTE 帧率
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            double frameDurationMs = (globalFps > 0.0001) ? (1000.0 / globalFps) : 40.0;
            double sleepTimeMs = frameDurationMs - elapsed.count();
            // 至少休眠1ms，避免忙等导致CPU占用
            if (sleepTimeMs < 1.0) sleepTimeMs = 1.0;
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(sleepTimeMs)));
        }
    }

    /**
     * @brief 系统时间戳驱动的消费回调
     * - 从缓存中读取 2048 个采样
     * - 打上系统时间戳
     * - 放入环形队列
     */
    void LTCGeneratorWorker::onSystemFrameTick(qint64 frameCount)
    {
        QMutexLocker locker(&_mutex);
        if (!_isProcessing || !_outputBuffer || !_outputBuffer->isActive()) {
            return;
        }

        // 读取“整帧样本数”，保证与当前全局时间戳帧率保持一致
        const int samplesNeeded = getBufferSize();
        QVector<float> block;
        
        // 从缓存中取出数据；不足时会尝试补编码或以静音填充，保证输出节拍稳定
        takeSamplesLocked(samplesNeeded, block);

        AudioFrame outFrame;
        outFrame.sampleRate = static_cast<int>(SAMPLE_RATE);
        outFrame.channels = 1;
        outFrame.bitsPerSample = 32;
        // 时间戳领先若干帧，使 AudioDeviceOut / LTCDecoder 按当前全局帧计数取块时数据已就绪
        outFrame.timestamp = frameCount + LTC_TIMESTAMP_LEAD_FRAMES;
        
        outFrame.data = QByteArray(reinterpret_cast<const char*>(block.constData()),
                                   block.size() * static_cast<int>(sizeof(float)));
        
        _outputBuffer->pushFrame(outFrame);
    }
    void LTCGeneratorWorker::advancePlaybackFramesLocked(int consumedSamples)
    {
        // 将“已消费的样本数”按帧边界扣减：
        // - 未跨过帧边界：仅累计当前帧已消费样本
        // - 跨过帧边界：发射对应时间码快照并进入下一帧
        int remaining = consumedSamples;
        while (remaining > 0 && _frameQueueHead < _frameSampleCounts.size()) {
            int headRemaining = _frameSampleCounts[_frameQueueHead] - _frameHeadConsumed;
            if (remaining < headRemaining) {
                _frameHeadConsumed += remaining;
                break;
            } else {
                remaining -= headRemaining;
                SMPTETimecode tc = _frameTimecodes[_frameQueueHead];
                _frameQueueHead++;
                _frameHeadConsumed = 0;
                // 仅在完整播放完一帧时才更新 UI，确保与实际输出一致
                emit timeCodeFrameGenerated(toTimeCodeFrameLocked(tc));
            }
        }
    }
}
