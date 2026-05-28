#ifndef FMODDECODERWORKER_H
#define FMODDECODERWORKER_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include <vector>
#include <memory>
#include <atomic>
#include <QString>
#include <QStringList>
#include <QDebug>
#include "NodeDataList.hpp"
#include "fmod.hpp"
#include "fmod_studio.hpp"

#ifndef F_CALLBACK
    #if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
        #define F_CALLBACK __stdcall
    #else
        #define F_CALLBACK
    #endif
#endif

namespace Nodes {

/**
 * @brief FMOD 解码工作线程
 * 
 * 负责在单独的线程中管理 FMOD Studio System，加载 Bank，播放 Event，
 * 并通过 DSP 回调捕获音频数据，写入到 AudioTimestampRingQueue 中。
 */
class FmodDecoderWorker : public QObject {
    Q_OBJECT
public:
    explicit FmodDecoderWorker(QObject *parent = nullptr);
    ~FmodDecoderWorker() override;

signals:
    /// @brief 事件列表更新信号
    void eventListUpdated(const QStringList& events);
    
    /// @brief 错误发生信号
    void errorOccurred(const QString& msg);

public slots:
    /**
     * @brief 初始化
     * @param buffers 输出缓冲区列表
     */
    void initialize(std::vector<std::shared_ptr<AudioTimestampRingQueue>> buffers);

    /// @brief 开始处理 (启动更新定时器)
    void startProcessing();

    /// @brief 停止处理 (停止更新定时器)
    void stopProcessing();

    /**
     * @brief 加载 Bank 资源
     * @param path Bank 文件所在的目录路径
     */
    void loadBanks(const QString& path);

    /**
     * @brief 播放指定事件
     * @param eventPath 事件路径或 GUID
     */
    void playEvent(const QString& eventPath);

private:
    /// @brief 初始化 FMOD 核心系统
    void initFMOD();

    /// @brief 更新事件列表并通知前端
    void updateEventList();

    /**
     * @brief DSP 回调函数：捕获音频数据
     * 
     * 这是 FMOD 核心混合线程调用的静态回调。
     * 在此函数中，我们将音频数据从 FMOD buffer 拷贝到我们的 RingQueue 中。
     */
    static FMOD_RESULT F_CALLBACK captureDSPCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels);

private:
    // --- FMOD 系统指针 ---
    FMOD::Studio::System* studioSystem_ = nullptr;
    FMOD::System* coreSystem_ = nullptr;
    
    // --- 资源管理 ---
    std::vector<FMOD::Studio::Bank*> loadedBanks_;
    FMOD::Studio::EventInstance* eventInstance_ = nullptr; // 当前播放的事件实例
    FMOD::DSP* captureDSP_ = nullptr;                     // 用于捕获音频的 DSP
    
    // --- 输出 ---
    std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputBuffers_;
    int sampleRate_ = 48000;
    
    // --- 逻辑更新定时器 ---
    QTimer* updateTimer_ = nullptr;
    
    // --- 时间戳同步相关 ---
    qint64 baseFrameCount_ = 0;       // 播放开始时的基准帧号
    bool timestampAligned_ = false;   // 是否已对齐时间戳
    int samplesPerFrame_ = 2048;      // 每帧采样数 (Node Editor 标准)
    qint64 emittedFrameCount_ = 0;    // 已输出的帧计数
    
    // --- 数据缓冲 (用于适配 FMOD buffer size 到 samplesPerFrame_) ---
    std::vector<QVector<float>> pendingChannelData_;    // 每个通道的暂存数据
    std::vector<int> pendingChannelSampleCount_;        // 每个通道已暂存的采样数
    
    // --- 延迟补偿 ---
    // 增加固定延时以避免后端播放时读取到未准备好的数据导致断断续续
    int latencyOffsetFrames_ = 2; 

    // --- 播放控制标志 ---
    std::atomic<bool> isPlaying_{false};
};

} // namespace Nodes

#endif //FMODDECODERWORKER_H
