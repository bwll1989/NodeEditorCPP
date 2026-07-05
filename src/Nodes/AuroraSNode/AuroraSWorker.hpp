#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
#include <QtCore/QMutex>
#include <atomic>
#include <memory>

#include "aurora_pubsdk_inc.h"

/// Aurora S SDK 工作线程对象：负责连接、地图上传、重定位与 50Hz 位姿输出。
/// 运行在独立 QThread 上，SDK 回调通过 QueuedConnection 或 mutex 缓存转发到主循环。
class AuroraSWorker : public QObject
{
    Q_OBJECT

public:
    explicit AuroraSWorker(QObject* parent = nullptr);
    ~AuroraSWorker() override;

    /// 当前是否正在执行 runSession 会话循环
    bool isSessionActive() const { return m_sessionActive.load(); }

    /// 请求停止会话（设置 m_running=false，各阻塞循环会退出）
    void requestStop();

    /// SDK 位姿回调线程入队最新位姿（只保留最新一帧，避免堆积）
    void enqueuePoseFromCallback(uint64_t timestampNs,
                                 const slamtec_aurora_sdk_pose_se3_t& pose);

public slots:
    /// 转发连接状态变化到 UI / DataModel
    void notifyConnectionChanged(bool connected);
    /// 网络断线：标记 connectionLost 并触发外层重连循环
    void notifyConnectionLost();
    /// 更新定位状态并 emit localizationStatusReady（相同状态去重）
    void emitLocalizationStatusUpdate(const QString& state);
    /// 检测到 LOSTED 标志时自动触发重定位（运行中跟踪丢失）
    void tryRelocalizeIfNeeded();
    /// 会话入口：连接 → 初始化 → 位姿输出 → 断线重连
    void runSession(const QString& host, const QString& mapFilePath);
    /// 强制重新上传地图并重定位（已连接时原地执行）
    void reinitializeSession(const QString& mapFilePath);
    void stopSession();
    void shutdown();
    /// 在 worker 线程内清理 SDK 并 delete this（析构路径专用）
    void shutdownAndDelete();

signals:
    void connectionChanged(bool connected);
    void poseSampleReady(const QVariantMap& orientation, const QVariantMap& position);
    void localizationStatusReady(const QVariantMap& status);
    void errorOccurred(const QString& message);
    void sessionFinished();

private:
    class SdkListener;

    static QVariantMap orientationToMap(uint64_t timestampNs,
                                        const slamtec_aurora_sdk_pose_se3_t& pose);
    static QVariantMap positionToMap(uint64_t timestampNs,
                                     const slamtec_aurora_sdk_pose_se3_t& pose);
    static QVariantMap localizationStatusToMap(const QString& state);
    static QString localizationStateText(const QString& state);

    bool connectToDevice(const QString& host,
                         rp::standalone::aurora::SDKServerConnectionDesc& selectedDesc);
    /// 可中断 sleep，支持 requestStop 与 reinitialize 的 abortSleep
    bool interruptibleSleep(int totalMs);
    void cleanupSdk();
    /// 上传地图（可选）→ 纯定位模式 → 重定位
    bool initializeSession(const QString& mapFilePath, bool skipMapUpload);
    bool uploadMap(const QString& mapFilePath);
    /// 重定位失败每 5 秒重试，直到成功或会话停止
    bool performRelocalizationWithRetry();
    /// 单次连接会话：创建 SDK → 连接 → 初始化 → 主循环
    bool runConnectedSession(const QString& host, const QString& mapFilePath);
    /// 启动 50Hz IMU+视觉混合姿态增强
    bool startPoseAugmentationSession();
    void refreshLocalizationStatusFromFlags();
    void emitPoseSample(uint64_t timestampNs, const slamtec_aurora_sdk_pose_se3_t& pose);
    /// 处理 QueuedConnection 事件并短暂休眠（主循环空闲轮询）
    void processPendingEvents(int timeoutMs);
    /// 从缓存取出最新位姿并 emit
    void processPendingPoseIfAny();

    static constexpr int kRelocalizationRetryIntervalMs = 5000; ///< 重定位失败重试间隔
    static constexpr int kReconnectIntervalMs = 1000;           ///< 断线/连接失败重连间隔
    static constexpr int kSessionIdlePollMs = 20;               ///< 主循环单次 idle 时长

    std::atomic<bool> m_running{false};
    QString m_localizationState = QStringLiteral("unknown");
    std::atomic<bool> m_sessionActive{false};
    std::atomic<bool> m_connectionLost{false};
    std::atomic<bool> m_shouldRetry{false};
    std::atomic<bool> m_mapUploadedInSession{false};  ///< 本会话是否已上传地图（重连时跳过）
    std::atomic<bool> m_abortSleep{false};            ///< 打断 interruptibleSleep（重新初始化）
    std::atomic<bool> m_forceReinitialize{false};
    std::atomic<bool> m_posePending{false};
    std::atomic<bool> m_relocalizingActive{false};    ///< 防止并发重定位
    QMutex m_poseMutex;
    slamtec_aurora_sdk_pose_se3_t m_pendingPose{};
    uint64_t m_pendingPoseTimestampNs = 0;
    QString m_sessionMapFilePath;
    std::unique_ptr<SdkListener> m_listener;
    rp::standalone::aurora::RemoteSDK* m_sdk = nullptr;
    slamtec_aurora_sdk_pose_augmentation_mode_t m_poseMode =
        SLAMTEC_AURORA_SDK_POSE_AUGMENTATION_MODE_IMU_VISION_MIXED;
};
