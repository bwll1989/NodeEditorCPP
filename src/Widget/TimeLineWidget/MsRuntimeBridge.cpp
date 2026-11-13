// ============================================================================
//  MsRuntimeBridge.cpp —— 桥接壳实现（PIMPL；C++20 TU，随 ms_editor_ui 编译）
// ============================================================================

#include "MsRuntimeBridge.hpp"

#include <QDir>
#include <QStandardPaths>

#include "clock/ClockProvider.hpp"   // 对齐时钟（命令 now 注入）
#include "loader/SceneLoader.hpp"    // Scene pb serialize/parse（R1 校验）
#include "runtime/EditorRuntime.hpp" // 下发运行时（连接/命令/分发）

namespace ms {

// ---------------------------------------------------------------------------
// PIMPL：运行时细节（EditorRuntime + ClockProvider）只在本文件可见
// ---------------------------------------------------------------------------
class MsRuntimeBridge::Impl {
public:
    ClockProvider  clock; ///< 全场对齐时钟（复合钟：monotonic 推进 + NTP offset）
    EditorRuntime  rt;

    explicit Impl(EditorRuntime::Config cfg) : rt(std::move(cfg)) {}

    /// 当前对齐时钟值（us；cmd* 与 sessionTimeUs 共用同一读法）
    std::int64_t nowUs() const { return clock.alignedNowUs(); }
};

MsRuntimeBridge::MsRuntimeBridge(QObject* parent)
    : QObject(parent), impl_(nullptr) {} // Impl 在 start() 时创建（空桥可构造）

MsRuntimeBridge::~MsRuntimeBridge() {
    // Impl 析构链自动 EditorRuntime::stop()（join 发送/看门狗线程）
}

bool MsRuntimeBridge::start() {
    if (impl_) return true; // 幂等
    // 素材根：本机可写缓存目录（CRC 缓存 .crc_cache.tsv 落此处持久化，
    // 重启零重算）。素材寻址不走此根——Asset.path 为绝对路径直通
    //（EditorRuntime::assetFullPath 已支持）。
    const QString root = QStandardPaths::writableLocation(
                             QStandardPaths::DocumentsLocation)
                         + QStringLiteral("/Flow/Cache/MsAssets");
    QDir().mkpath(root);

    EditorRuntime::Config cfg;          // 默认：UDP 17001 / HTTP 17002 /
                                        // lead 10ms / beat 250ms
    cfg.assets_root = root.toStdString();
    impl_ = std::make_unique<Impl>(std::move(cfg));
    if (!impl_->rt.start()) {           // 空场景模式（场景经 publishScene 注入）
        std::fprintf(stderr,
                     "[ms-bridge] 下发运行时启动失败（端口占用？）——"
                     "GUI 继续可用，仅无下发\n");
        std::fflush(stderr);
        impl_.reset();
        return false;
    }
    std::fprintf(stderr, "[ms-bridge] 下发运行时就绪：UDP %u / HTTP %u\n",
                 (unsigned)impl_->rt.udp_port(),
                 (unsigned)impl_->rt.http_port());
    std::fflush(stderr);
    return true;
}

void MsRuntimeBridge::stop() {
    if (!impl_) return;
    impl_->rt.stop();
    impl_.reset();
}

bool MsRuntimeBridge::running() const { return impl_ != nullptr; }

// ---- 场景 / 素材 -----------------------------------------------------------
void MsRuntimeBridge::publishScene(const Scene& scene) {
    if (!impl_) return;
    impl_->rt.cmdPublishScene(scene); // 异步：快照入队，发送线程准备+下发
}

std::size_t MsRuntimeBridge::syncMedia(const Scene& scene) {
    if (!impl_) return 0;
    return impl_->rt.cmdSyncMedia(scene); // 注册 + 推清单（推模型）
}

bool MsRuntimeBridge::authoritySnapshot(Scene& out) const {
    if (!impl_) return false;
    return impl_->rt.sceneSnapshot(out);
}

// ---- 播放控制 ---------------------------------------------------------------
void MsRuntimeBridge::cmdPlay() {
    if (!impl_) return;
    impl_->rt.cmdPlay(impl_->nowUs());
}

void MsRuntimeBridge::cmdPause() {
    if (!impl_) return;
    impl_->rt.cmdPause(impl_->nowUs());
}

void MsRuntimeBridge::cmdSeekToUs(std::int64_t session_time_us) {
    if (!impl_) return;
    impl_->rt.cmdSeek(impl_->nowUs(), session_time_us);
}

void MsRuntimeBridge::cmdStop() {
    if (!impl_) return;
    impl_->rt.cmdStop(impl_->nowUs());
}

bool MsRuntimeBridge::playing() const {
    if (!impl_) return false;
    return impl_->rt.state() == PlayState::Playing;
}

std::int64_t MsRuntimeBridge::sessionTimeUs() const {
    if (!impl_) return -1;
    // 仅播放态有权威推进值；Paused/Idle 的冻结值交给本地预览状态机
    if (impl_->rt.state() != PlayState::Playing) return -1;
    return impl_->rt.session_time_us(impl_->nowUs());
}

// ---- 显示端管理 ---------------------------------------------------------------
bool MsRuntimeBridge::addDisplay(const QString& id, const QString& host,
                                 int port, const QString& pairing_code) {
    if (!impl_) return false;
    return impl_->rt.addDisplay(id.toStdString(), host.toStdString(),
                                (std::uint16_t)port,
                                pairing_code.toStdString());
}

bool MsRuntimeBridge::removeDisplay(const QString& id) {
    if (!impl_) return false;
    return impl_->rt.removeDisplay(id.toStdString());
}

bool MsRuntimeBridge::setDisplayEnabled(const QString& id, bool enabled) {
    if (!impl_) return false;
    return impl_->rt.setDisplayEnabled(id.toStdString(), enabled);
}

QList<MsRuntimeBridge::DisplayRow> MsRuntimeBridge::displayRows() const {
    QList<DisplayRow> out;
    if (!impl_) return out;
    // 连接态（TCP/注册/退避）与登记态（enabled）合流：登记表为主键，
    // 连接快照按 display_id 对位补充
    std::map<std::string, DisplayConnector::Entry> conn;
    for (const auto& e : impl_->rt.displayStatusSnapshot())
        conn[e.display_id] = e;
    for (const auto& d : impl_->rt.displayEntries()) {
        DisplayRow r;
        r.id        = QString::fromStdString(d.display_id);
        r.host      = QString::fromStdString(d.host);
        r.port      = d.port;
        r.enabled   = d.enabled;
        r.pairingCode = QString::fromStdString(d.pairing_code); // 工程持久化用
        auto it     = conn.find(d.display_id);
        r.connected = it != conn.end() && it->second.connected;
        r.registered = it != conn.end() && it->second.registered;
        out.push_back(r);
    }
    return out;
}

QMap<QString, QString> MsRuntimeBridge::assetSyncTexts() const {
    QMap<QString, QString> out;
    if (!impl_) return out;
    for (const auto& [id, v] : impl_->rt.assetSyncSnapshot()) {
        QString text;
        if (v.finished) {
            text = QStringLiteral("✓ 已同步");
        } else if (v.total > 0) {
            const int done = (int)v.done;
            if (v.current_total > 0) {
                const int percent = (int)qBound<qint64>(
                    qint64(0),
                    (qint64)v.current_bytes * 100 / (qint64)v.current_total,
                    qint64(100));
                text = QStringLiteral("⤓ 同步 %1/%2 · %3%")
                           .arg(done)
                           .arg((int)v.total)
                           .arg(percent);
            } else {
                text = QStringLiteral("⤓ 同步 %1/%2")
                           .arg(done)
                           .arg((int)v.total);
            }
        }
        out.insert(QString::fromStdString(id), text);
    }
    return out;
}

// ---- .flow 工程序列化 ---------------------------------------------------------
QString MsRuntimeBridge::sceneToBase64(const Scene& scene, QString* err) {
    try {
        const std::string bytes = SceneLoader::serialize(scene);
        return QString::fromLatin1(
            QByteArray(bytes.data(), (int)bytes.size()).toBase64());
    } catch (const std::exception& e) {
        if (err) *err = QString::fromUtf8(e.what());
        return {};
    }
}

bool MsRuntimeBridge::sceneFromBase64(const QString& b64, Scene& out,
                                      QString& err) {
    const QByteArray bytes =
        QByteArray::fromBase64(b64.toLatin1());
    std::string serr;
    auto scene = SceneLoader::parse(
        std::string(bytes.constData(), (std::size_t)bytes.size()), serr);
    if (!scene) {
        err = QString::fromStdString(serr);
        return false;
    }
    out = std::move(*scene);
    return true;
}

} // namespace ms
