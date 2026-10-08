/**
 * @file FmodDecoderWorker.h
 * @brief FMOD Studio 后台 Worker：Bank / 多 Instance 播放 / DSP 捕获 / 用户参数
 */

#ifndef FMODDECODERWORKER_H
#define FMODDECODERWORKER_H

#include "NodeDataList.hpp"
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "TimestampGenerator/AudioThreadRealtime.hpp"

#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>
#include <QVector>

#include <atomic>
#include <memory>
#include <vector>

#ifndef F_CALLBACK
    #if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
        #define F_CALLBACK __stdcall
    #else
        #define F_CALLBACK
    #endif
#endif

namespace Nodes {

/**
 * @brief 单个 FMOD 用户参数描述（供面板与输入口生成）
 *
 * 仅包含 Game Controlled、非只读、非自动参数。
 */
struct FmodParamDesc {
    QString eventPath;   ///< 完整路径，如 event:/effect/creepy
    QString paramName;   ///< Studio 参数名，如 Parameter 2
    QString caption;     ///< UI/端口标题，如 effect/creepy/Parameter 2
    float minimum = 0.f;
    float maximum = 1.f;
    float defaultValue = 0.f;
    bool discrete = false; ///< Discrete 标志
    bool labeled = false;  ///< Labeled 标志
    QStringList labels;    ///< labeled 时的枚举名（与 min..max 步进对应）
};

/**
 * @brief 运行于独立线程的 FMOD 解码与播放器
 *
 * - NOSOUND + Capture DSP：不直出系统声卡，帧写入 AudioTimestampRingQueue
 * - 多 EventInstance：不同事件可叠播；同事件多次 play 可叠层（受 Studio Max Instances 约束）
 * - 参数：paramCache_ 在 createInstance 后、start 前写入；运行中 setEventParameter 更新存活实例
 */
class FmodDecoderWorker : public QObject {
    Q_OBJECT
public:
    explicit FmodDecoderWorker(QObject* parent = nullptr);
    ~FmodDecoderWorker() override;

signals:
    /** Bank 加载/枚举完成后发出：事件列表 + 用户参数目录 */
    void eventCatalogUpdated(const QStringList& events, const QVector<Nodes::FmodParamDesc>& params);
    void errorOccurred(const QString& msg);

public slots:
    /** @brief 保存输出缓冲指针并 initFMOD */
    void initialize(std::vector<std::shared_ptr<AudioTimestampRingQueue>> buffers);

    /** @brief 启动 studioSystem_->update 定时器（约 50Hz） */
    void startProcessing();

    /** @brief 停止定时器 */
    void stopProcessing();

    /**
     * @brief 加载目录下全部 .bank（strings.bank 优先），并刷新事件目录
     * @param path Bank 文件夹或其中某一文件的路径
     */
    void loadBanks(const QString& path);

    /**
     * @brief 新建并启动一个 EventInstance（不停止其它实例）
     * @param eventPath 事件路径，或 "ID: {guid}" 形式
     */
    void playEvent(const QString& eventPath);

    /**
     * @brief 写入参数缓存，并对该事件所有存活 Instance 立即 setParameterByName
     */
    void setEventParameter(const QString& eventPath, const QString& paramName, float value);

    /**
     * @brief 批量 setEventParameter（目录刷新后把面板/存盘值灌进缓存）
     */
    void setEventParameterMap(const QString& eventPath, const QVariantMap& values);

private:
    void initFMOD();
    void updateEventCatalog();
    void stopAllInstances(FMOD_STUDIO_STOP_MODE mode = FMOD_STUDIO_STOP_IMMEDIATE);
    void pruneStoppedInstances();

    /** @brief 去掉 "ID: " 前缀，得到 getEvent 可用路径 */
    static QString cleanEventPath(const QString& eventPath);

    /** @brief 用 paramCache_[eventPath] 应用到实例 */
    void applyCachedParameters(const QString& eventPath, FMOD::Studio::EventInstance* instance) const;

    /** @brief Master 总线 Capture DSP 回调：透传 + 按帧写入各输出 RingQueue */
    static FMOD_RESULT F_CALLBACK captureDSPCallback(
        FMOD_DSP_STATE* dsp_state,
        float* inbuffer,
        float* outbuffer,
        unsigned int length,
        int inchannels,
        int* outchannels);

    FMOD::Studio::System* studioSystem_ = nullptr;
    FMOD::System* coreSystem_ = nullptr;
    FMOD::DSP* captureDSP_ = nullptr;

    std::vector<FMOD::Studio::Bank*> loadedBanks_;
    /** cleanPath → 存活实例列表（同路径可多条） */
    QHash<QString, QList<FMOD::Studio::EventInstance*>> eventInstances_;
    /** 目录事件路径 → (参数名 → 值)；与 UI/存盘键一致 */
    QHash<QString, QHash<QString, float>> paramCache_;

    std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputBuffers_;
    int sampleRate_ = 48000;
    int samplesPerFrame_ = 0;

    QTimer* updateTimer_ = nullptr;
    std::unique_ptr<AudioThreadRealtimeGuard> realtimeGuard_;

    qint64 baseFrameCount_ = 0;
    qint64 emittedFrameCount_ = 0;
    bool timestampAligned_ = false;
    int latencyOffsetFrames_ = 2;

    std::vector<QVector<float>> pendingChannelData_;
    std::vector<int> pendingChannelSampleCount_;

    /** 任一实例在播则为 true（供 DSP 决定是否推帧） */
    std::atomic<bool> isPlaying_{false};
};

} // namespace Nodes

Q_DECLARE_METATYPE(Nodes::FmodParamDesc)
Q_DECLARE_METATYPE(QVector<Nodes::FmodParamDesc>)

#endif // FMODDECODERWORKER_H
