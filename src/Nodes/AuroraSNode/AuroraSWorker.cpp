#include "AuroraSWorker.hpp"

#include <QtCore/QMutexLocker>
#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include <QtCore/QtMath>

#include "Common/AppConfig/ConstantDefines.h"

#include <chrono>
#include <future>
#include <QDir>
#include <thread>

using namespace rp::standalone::aurora;

namespace
{
/// 将 UI 中的地图路径解析为绝对路径：优先绝对/相对当前目录，否则拼接媒体库目录
QString resolveMapFilePath(const QString& mapFilePath)
{
    const QString trimmed = mapFilePath.trimmed();
    if (trimmed.isEmpty()) {
        return trimmed;
    }

    const QFileInfo directInfo(trimmed);
    if (directInfo.isAbsolute()) {
        return directInfo.absoluteFilePath();
    }

    if (directInfo.exists() && directInfo.isFile()) {
        return directInfo.absoluteFilePath();
    }

    return QDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR).absoluteFilePath(trimmed);
}
} // namespace

/// SDK 事件监听器：在 SDK 内部线程回调，通过 QueuedConnection 投递到 AuroraSWorker 所在线程
class AuroraSWorker::SdkListener : public RemoteSDKListener
{
public:
    explicit SdkListener(AuroraSWorker* worker)
        : m_worker(worker)
    {
    }

    /// 连接状态变化（恢复 / 丢失 / 配置变更）
    void onConnectionStatus(slamtec_aurora_sdk_connection_status_t status) override
    {
        QPointer<AuroraSWorker> worker = m_worker;
        if (!worker) {
            return;
        }

        bool connected = false;
        switch (status) {
        case SLAMTEC_AURORA_SDK_CONNECTION_STATUS_RESTORED:
        case SLAMTEC_AURORA_SDK_CONNECTION_STATUS_DEVICE_CONFIG_CHANGED:
            connected = true;
            break;
        case SLAMTEC_AURORA_SDK_CONNECTION_STATUS_LOST:
            // 断线时不 requestStop，由外层 1s 重连循环处理
            QMetaObject::invokeMethod(worker,
                                      "notifyConnectionLost",
                                      Qt::QueuedConnection);
            connected = false;
            break;
        default:
            connected = false;
            break;
        }

        QMetaObject::invokeMethod(worker,
                                  "notifyConnectionChanged",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, connected));
    }

    /// 50Hz 姿态增强结果回调
    void onPoseAugmentationResult(uint64_t timestampNs,
                                  slamtec_aurora_sdk_pose_augmentation_mode_t /*mode*/,
                                  const slamtec_aurora_sdk_pose_se3_t& pose) override
    {
        QPointer<AuroraSWorker> worker = m_worker;
        if (!worker) {
            return;
        }

        worker->enqueuePoseFromCallback(timestampNs, pose);
    }

    /// 设备定位/跟踪状态变化
    void onDeviceStatusChanged(uint64_t /*timestampNs*/,
                               slamtec_aurora_sdk_device_status_t status) override
    {
        QPointer<AuroraSWorker> worker = m_worker;
        if (!worker) {
            return;
        }

        QString state;
        switch (status) {
        case SLAMTEC_AURORA_SDK_DEVICE_TRACKING_LOST:
            state = QStringLiteral("tracking_lost");
            break;
        case SLAMTEC_AURORA_SDK_DEVICE_TRACKING_RECOVERED:
        case SLAMTEC_AURORA_SDK_DEVICE_RELOCALIZATION_SUCCESS:
            state = QStringLiteral("localized");
            break;
        case SLAMTEC_AURORA_SDK_DEVICE_RELOCALIZATION_FAILED:
            state = QStringLiteral("relocalization_failed");
            break;
        case SLAMTEC_AURORA_SDK_DEVICE_RELOCALIZATION_STARTED:
            state = QStringLiteral("relocalizing");
            break;
        default:
            return;
        }

        QMetaObject::invokeMethod(worker,
                                  "emitLocalizationStatusUpdate",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, state));
    }

    /// 建图/定位标志变化；LOSTED 时触发自动重定位
    void onNewMappingFlags(slamtec_aurora_sdk_mapping_flag_t flags) override
    {
        QPointer<AuroraSWorker> worker = m_worker;
        if (!worker) {
            return;
        }

        if ((flags & SLAMTEC_AURORA_SDK_MAPPING_FLAG_LOSTED) != 0) {
            QMetaObject::invokeMethod(worker,
                                      "emitLocalizationStatusUpdate",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, QStringLiteral("tracking_lost")));
            QMetaObject::invokeMethod(worker,
                                      "tryRelocalizeIfNeeded",
                                      Qt::QueuedConnection);
        }
    }

private:
    QPointer<AuroraSWorker> m_worker;
};

AuroraSWorker::AuroraSWorker(QObject* parent)
    : QObject(parent)
{
}

AuroraSWorker::~AuroraSWorker()
{
    requestStop();
}

void AuroraSWorker::requestStop()
{
    m_running.store(false);
    m_abortSleep.store(true); // 立即打断 interruptibleSleep，缩短会话退出时间
}

void AuroraSWorker::stopSession()
{
    requestStop();
    m_localizationState = QStringLiteral("unknown");
}

void AuroraSWorker::shutdown()
{
    requestStop();
    cleanupSdk();
    m_sessionActive.store(false);
}

void AuroraSWorker::prepareThreadExit()
{
    // 仅在无 runSession 栈帧时清理；活跃会话由 runSession 自己 cleanupSdk
    if (!m_sessionActive.load()) {
        shutdown();
    } else {
        requestStop();
    }
    if (QThread* t = thread()) {
        t->quit();
    }
}

void AuroraSWorker::notifyConnectionChanged(bool connected)
{
    emit connectionChanged(connected);
}

void AuroraSWorker::notifyConnectionLost()
{
    m_connectionLost.store(true);
    m_shouldRetry.store(true);
    emit connectionChanged(false);
}

void AuroraSWorker::enqueuePoseFromCallback(uint64_t timestampNs,
                                            const slamtec_aurora_sdk_pose_se3_t& pose)
{
    {
        QMutexLocker locker(&m_poseMutex);
        m_pendingPose = pose;
        m_pendingPoseTimestampNs = timestampNs;
    }
    m_posePending.store(true);
}

void AuroraSWorker::processPendingPoseIfAny()
{
    if (!m_posePending.load()) {
        return;
    }

    slamtec_aurora_sdk_pose_se3_t pose{};
    uint64_t timestampNs = 0;
    {
        QMutexLocker locker(&m_poseMutex);
        if (!m_posePending.load()) {
            return;
        }
        pose = m_pendingPose;
        timestampNs = m_pendingPoseTimestampNs;
        m_posePending.store(false);
    }

    emitPoseSample(timestampNs, pose);
}

void AuroraSWorker::processPendingEvents(int timeoutMs)
{
    if (QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance()) {
        dispatcher->processEvents(QEventLoop::AllEvents);
    }
    interruptibleSleep(timeoutMs);
}

void AuroraSWorker::tryRelocalizeIfNeeded()
{
    if (!m_running.load() || m_relocalizingActive.load()) {
        return;
    }

    if (m_localizationState == QStringLiteral("relocalizing")) {
        return;
    }

    if (m_sdk == nullptr) {
        return;
    }

    slamtec_aurora_sdk_mapping_flag_t flags = SLAMTEC_AURORA_SDK_MAPPING_FLAG_NONE;
    if (!m_sdk->dataProvider.getMappingFlags(flags)) {
        return;
    }

    if ((flags & SLAMTEC_AURORA_SDK_MAPPING_FLAG_LOSTED) == 0) {
        return;
    }

    m_relocalizingActive.store(true);
    performRelocalizationWithRetry();
    m_relocalizingActive.store(false);
}

bool AuroraSWorker::interruptibleSleep(int totalMs)
{
    constexpr int stepMs = 20;
    int elapsed = 0;
    while (elapsed < totalMs && m_running.load()) {
        // reinitializeSession 可设置 abortSleep 提前结束等待
        if (m_abortSleep.exchange(false)) {
            return m_running.load();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
        elapsed += stepMs;
    }
    return m_running.load();
}

void AuroraSWorker::cleanupSdk()
{
    m_listener.reset();

    if (m_sdk != nullptr) {
        // 删除节点时尽快打断可能仍在进行的地图上传
        if (m_sdk->mapManager.isSessionActive()) {
            m_sdk->mapManager.abortSession();
        }
        m_sdk->dataProvider.stopPoseAugmentation();
        m_sdk->disconnect();
        RemoteSDK::DestroySession(m_sdk);
        m_sdk = nullptr;
    }
}

QVariantMap AuroraSWorker::orientationToMap(const slamtec_aurora_sdk_pose_se3_t& pose)
{
    slamtec_aurora_sdk_euler_angle_t euler{};
    slamtec_aurora_sdk_convert_quaternion_to_euler(&pose.quaternion, &euler);

    const QVariantList rad{euler.roll, euler.pitch, euler.yaw};
    const QVariantList deg{qRadiansToDegrees(euler.roll),
                           qRadiansToDegrees(euler.pitch),
                           qRadiansToDegrees(euler.yaw)};

    QVariantMap map;
    map.insert(QStringLiteral("rad"), rad);
    map.insert(QStringLiteral("deg"), deg);
    // 主载荷：asFloats() 默认读 default → 弧度向量
    map.insert(QStringLiteral("default"), rad);
    return map;
}

QVariantList AuroraSWorker::positionToList(const slamtec_aurora_sdk_pose_se3_t& pose)
{
    return QVariantList{pose.translation.x, pose.translation.y, pose.translation.z};
}

QString AuroraSWorker::localizationStateText(const QString& state)
{
    if (state == QStringLiteral("localized")) {
        return QStringLiteral("已定位");
    }
    if (state == QStringLiteral("relocalizing")) {
        return QStringLiteral("重定位中");
    }
    if (state == QStringLiteral("tracking_lost")) {
        return QStringLiteral("跟踪丢失");
    }
    if (state == QStringLiteral("relocalization_failed")) {
        return QStringLiteral("重定位失败");
    }
    if (state == QStringLiteral("reconnecting")) {
        return QStringLiteral("重连中");
    }
    return QStringLiteral("未知");
}

QVariantMap AuroraSWorker::localizationStatusToMap(const QString& state)
{
    QVariantMap map;
    map.insert(QStringLiteral("state"), state);
    map.insert(QStringLiteral("state_text"), localizationStateText(state));
    map.insert(QStringLiteral("localized"), state == QStringLiteral("localized"));
    map.insert(QStringLiteral("tracking_lost"), state == QStringLiteral("tracking_lost"));
    map.insert(QStringLiteral("relocalizing"), state == QStringLiteral("relocalizing"));
    map.insert(QStringLiteral("relocalization_failed"),
               state == QStringLiteral("relocalization_failed"));
    map.insert(QStringLiteral("reconnecting"), state == QStringLiteral("reconnecting"));
    return map;
}

void AuroraSWorker::emitLocalizationStatusUpdate(const QString& state)
{
    if (m_localizationState == state) {
        return;
    }

    m_localizationState = state;
    emit localizationStatusReady(localizationStatusToMap(state));
}

void AuroraSWorker::refreshLocalizationStatusFromFlags()
{
    if (m_sdk == nullptr) {
        return;
    }

    slamtec_aurora_sdk_mapping_flag_t flags = SLAMTEC_AURORA_SDK_MAPPING_FLAG_NONE;
    if (!m_sdk->dataProvider.getMappingFlags(flags)) {
        return;
    }

    if ((flags & SLAMTEC_AURORA_SDK_MAPPING_FLAG_LOSTED) != 0) {
        emitLocalizationStatusUpdate(QStringLiteral("tracking_lost"));
        return;
    }

    if (m_localizationState != QStringLiteral("relocalizing")) {
        emitLocalizationStatusUpdate(QStringLiteral("localized"));
    }
}

bool AuroraSWorker::uploadMap(const QString& mapFilePath)
{
    const QString trimmedPath = mapFilePath.trimmed();
    if (trimmedPath.isEmpty()) {
        return true; // 无地图路径时跳过上传
    }

    if (m_sdk == nullptr) {
        return false;
    }

    const QString resolvedPath = resolveMapFilePath(trimmedPath);
    const QFileInfo fileInfo(resolvedPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        emit errorOccurred(QStringLiteral("地图文件不存在: %1").arg(resolvedPath));
        return false;
    }

    std::promise<bool> resultPromise;
    auto resultFuture = resultPromise.get_future();

    auto resultCallBack = [](void* userData, int isOK) {
        auto* promise = reinterpret_cast<std::promise<bool>*>(userData);
        promise->set_value(isOK != 0);
    };

    const QByteArray absolutePath = fileInfo.absoluteFilePath().toUtf8();
    if (!m_sdk->mapManager.startUploadSession(absolutePath.constData(),
                                              resultCallBack,
                                              &resultPromise)) {
        emit errorOccurred(QStringLiteral("启动地图上传失败"));
        return false;
    }

    // 轮询上传进度直至会话结束
    while (m_sdk->mapManager.isSessionActive()) {
        if (!m_running.load()) {
            m_sdk->mapManager.abortSession();
            return false;
        }

        slamtec_aurora_sdk_mapstorage_session_status_t status{};
        if (!m_sdk->mapManager.querySessionStatus(status)) {
            emit errorOccurred(QStringLiteral("查询地图上传进度失败"));
            m_sdk->mapManager.abortSession();
            return false;
        }

        if (!interruptibleSleep(200)) {
            m_sdk->mapManager.abortSession();
            return false;
        }
    }

    while (resultFuture.wait_for(std::chrono::milliseconds(200)) != std::future_status::ready) {
        if (!m_running.load()) {
            return false;
        }
    }

    if (!resultFuture.get()) {
        emit errorOccurred(QStringLiteral("地图上传失败"));
        return false;
    }

    return true;
}

bool AuroraSWorker::initializeSession(const QString& mapFilePath, bool skipMapUpload)
{
    if (!mapFilePath.trimmed().isEmpty()) {
        if (!skipMapUpload) {
            if (!uploadMap(mapFilePath)) {
                return false;
            }
            m_mapUploadedInSession.store(true);
        }

        m_sdk->controller.requirePureLocalizationMode();
    }

    return performRelocalizationWithRetry();
}

bool AuroraSWorker::performRelocalizationWithRetry()
{
    while (m_running.load()) {
        emitLocalizationStatusUpdate(QStringLiteral("relocalizing"));

        if (m_sdk != nullptr && m_sdk->controller.requireRelocalization()) {
            refreshLocalizationStatusFromFlags();
            if (m_localizationState == QStringLiteral("tracking_lost")) {
                emitLocalizationStatusUpdate(QStringLiteral("relocalization_failed"));
            } else {
                emitLocalizationStatusUpdate(QStringLiteral("localized"));
                return true;
            }
        } else {
            emitLocalizationStatusUpdate(QStringLiteral("relocalization_failed"));
        }

        if (!interruptibleSleep(kRelocalizationRetryIntervalMs)) {
            return false;
        }
    }

    return false;
}

void AuroraSWorker::emitPoseSample(uint64_t timestampNs,
                                   const slamtec_aurora_sdk_pose_se3_t& pose)
{
    Q_UNUSED(timestampNs)
    emit poseSampleReady(orientationToMap(pose), positionToList(pose));
}

bool AuroraSWorker::connectToDevice(const QString& host,
                                    SDKServerConnectionDesc& selectedDesc)
{
    const QString connection = host.trimmed();
    if (connection.isEmpty()) {
        emit errorOccurred(QStringLiteral("设备地址不能为空"));
        return false;
    }

    // 支持 "tcp://ip:port" 或 "ip:port" 或纯 IP/主机名
    QString address = connection;
    const int schemeIndex = address.indexOf(QStringLiteral("://"));
    if (schemeIndex >= 0) {
        address = address.mid(schemeIndex + 3);
    }

    const int portIndex = address.lastIndexOf(':');
    if (portIndex > 0 && address.indexOf('.') < portIndex) {
        const QString ip = address.left(portIndex);
        bool ok = false;
        const int port = address.mid(portIndex + 1).toInt(&ok);
        if (ok) {
            selectedDesc = SDKServerConnectionDesc(ip.toUtf8().constData(), port);
            return true;
        }
    }

    selectedDesc = SDKServerConnectionDesc(address.toUtf8().constData());
    return true;
}

bool AuroraSWorker::startPoseAugmentationSession()
{
    if (m_sdk == nullptr) {
        return false;
    }

    slamtec_aurora_sdk_pose_augmentation_config_t config{};
    config.output_frequency = SLAMTEC_AURORA_SDK_POSE_OUTPUT_FREQ_50HZ;
    config.enable_smoothing = 0;
    config.smoothing_factor = 0.3f;
    m_poseMode = SLAMTEC_AURORA_SDK_POSE_AUGMENTATION_MODE_IMU_VISION_MIXED;

    slamtec_aurora_sdk_errorcode_t errcode = SLAMTEC_AURORA_SDK_ERRORCODE_OK;
    if (!m_sdk->dataProvider.startPoseAugmentation(m_poseMode, config, &errcode)) {
        emit errorOccurred(QStringLiteral("启动姿态增强失败，错误码: %1").arg(errcode));
        return false;
    }

    return true;
}

void AuroraSWorker::reinitializeSession(const QString& mapFilePath)
{
    if (!m_running.load()) {
        return;
    }

    if (!mapFilePath.trimmed().isEmpty()) {
        m_sessionMapFilePath = mapFilePath;
    }

    m_mapUploadedInSession.store(false);
    m_forceReinitialize.store(true);
    m_abortSleep.store(true);

    // 断线时标记 shouldRetry，等重连后在 runConnectedSession 中执行
    if (m_sdk == nullptr || m_connectionLost.load()) {
        m_shouldRetry.store(true);
        return;
    }

    m_sdk->dataProvider.stopPoseAugmentation();

    if (!initializeSession(m_sessionMapFilePath, false)) {
        emit errorOccurred(QStringLiteral("重新初始化失败"));
        return;
    }

    if (!startPoseAugmentationSession()) {
        return;
    }

    emit errorOccurred(QString()); // 清除错误提示
}

bool AuroraSWorker::runConnectedSession(const QString& host, const QString& mapFilePath)
{
    cleanupSdk();
    m_posePending.store(false);

    m_listener = std::make_unique<SdkListener>(this);
    m_sdk = RemoteSDK::CreateSession(m_listener.get());
    if (m_sdk == nullptr) {
        emit errorOccurred(QStringLiteral("创建 Aurora SDK 会话失败"));
        m_listener.reset();
        m_shouldRetry.store(true);
        return false;
    }

    SDKServerConnectionDesc selectedDesc;
    if (!connectToDevice(host, selectedDesc)) {
        cleanupSdk();
        m_shouldRetry.store(true);
        return false;
    }

    if (!m_sdk->connect(selectedDesc)) {
        emit errorOccurred(QStringLiteral("连接 Aurora 设备失败"));
        cleanupSdk();
        emit connectionChanged(false);
        m_shouldRetry.store(true);
        return false;
    }

    emit connectionChanged(true);
    emitLocalizationStatusUpdate(QStringLiteral("unknown"));

    if (m_forceReinitialize.exchange(false)) {
        m_mapUploadedInSession.store(false);
    }

    // 重连时 skipMapUpload=true，避免重复上传
    const bool skipMapUpload = m_mapUploadedInSession.load();
    if (!initializeSession(mapFilePath, skipMapUpload)) {
        cleanupSdk();
        emit connectionChanged(false);
        m_shouldRetry.store(true);
        return false;
    }

    if (!startPoseAugmentationSession()) {
        cleanupSdk();
        emit connectionChanged(false);
        m_shouldRetry.store(true);
        return false;
    }

    // 主循环：处理位姿缓存 + SDK 投递的 Queued 事件
    while (m_running.load() && !m_connectionLost.load()) {
        processPendingPoseIfAny();
        processPendingEvents(kSessionIdlePollMs);
    }

    cleanupSdk();
    emit connectionChanged(false);
    return true;
}

void AuroraSWorker::runSession(const QString& host, const QString& mapFilePath)
{
    if (m_sessionActive.exchange(true)) {
        return; // 防止重复启动
    }

    m_running.store(true);
    m_sessionMapFilePath = mapFilePath;
    m_mapUploadedInSession.store(false);

    // 外层循环：单次连接失败或断线后 1s 重试
    while (m_running.load()) {
        m_shouldRetry.store(false);
        m_connectionLost.store(false);

        if (m_forceReinitialize.exchange(false)) {
            m_mapUploadedInSession.store(false);
        }

        runConnectedSession(host, m_sessionMapFilePath);

        if (!m_running.load()) {
            break;
        }

        if (!m_shouldRetry.load()) {
            break;
        }

        if (!interruptibleSleep(kReconnectIntervalMs)) {
            break;
        }
    }

    cleanupSdk();
    m_running.store(false);
    emit connectionChanged(false);
    m_sessionActive.store(false);
    emit sessionFinished();
}
