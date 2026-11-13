// ============================================================================
//  Discovery.cpp —— 局域网显示端发现实现（协议编解码 + 应答器 + 浏览器）
// ============================================================================

#include "net/Discovery.hpp"

#include <cstring>
#include <ctime>

#include <Poco/Environment.h> // nodeName()：本机主机名（免链 ws2_32）
#include <Poco/Exception.h>

namespace ms {

// ---------------------------------------------------------------------------
// 线格式布局（#pragma pack(1) 锁定 72 字节；与 DiscoveryCodec::kWireSize 一致）
//   [0..1] magic   [2] version   [3] type
//   [4..35] player_id[32]  [36..67] hostname[32]
//   [68..69] listen_port   [70] state   [71] mode（v1 原 pad 保留字节，
//          M5 起语义化为 PlayerMode——老程序恒写 0/读取忽略，零破坏互操作）
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct DiscoveryRaw {
    std::uint16_t magic;
    std::uint8_t  version;
    std::uint8_t  type;
    char          player_id[32];
    char          hostname[32];
    std::uint16_t listen_port;
    std::uint8_t  state;
    std::uint8_t  mode;
};
#pragma pack(pop)

static_assert(sizeof(DiscoveryRaw) == DiscoveryCodec::kWireSize,
              "DiscoveryRaw 必须为 72 字节（发现帧定长布局）");

namespace {

/// 定长 char[N] 字段写入：超长截断，'\0' 填充（保证 C 串语义）
void putFixed(char* dst, std::size_t cap, const std::string& src) {
    const std::size_t n = src.size() < cap - 1 ? src.size() : cap - 1;
    std::memcpy(dst, src.data(), n);
    std::memset(dst + n, 0, cap - n);
}

/// 定长 char[N] 字段读取（'\0' 截断；不带终止符时按 strnlen 兜底）
std::string getFixed(const char* src, std::size_t cap) {
    const char* end = static_cast<const char*>(
        std::memchr(src, '\0', cap));
    return std::string(src, end ? static_cast<std::size_t>(end - src) : cap);
}

std::int64_t steadyNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

} // namespace

// ---------------------------------------------------------------------------
// 编解码
// ---------------------------------------------------------------------------
std::size_t DiscoveryCodec::encode(const DiscoveryFrame& f, void* out) noexcept {
    DiscoveryRaw raw{};
    raw.magic   = kMagic;
    raw.version = kVersion;
    raw.type    = static_cast<std::uint8_t>(f.type);
    putFixed(raw.player_id, sizeof(raw.player_id), f.player_id);
    putFixed(raw.hostname, sizeof(raw.hostname), f.hostname);
    raw.listen_port = f.listen_port;
    raw.state       = static_cast<std::uint8_t>(f.state);
    raw.mode        = static_cast<std::uint8_t>(f.mode);
    std::memcpy(out, &raw, sizeof(raw));
    return sizeof(raw);
}

std::optional<DiscoveryFrame> DiscoveryCodec::decode(const void* data,
                                                     std::size_t len) noexcept {
    if (len != kWireSize) return std::nullopt;          // 定长校验
    DiscoveryRaw raw{};
    std::memcpy(&raw, data, sizeof(raw));
    if (raw.magic != kMagic || raw.version != kVersion) return std::nullopt;
    if (raw.type != static_cast<std::uint8_t>(DiscoveryFrame::Type::Query) &&
        raw.type != static_cast<std::uint8_t>(DiscoveryFrame::Type::Response))
        return std::nullopt;

    DiscoveryFrame f;
    f.type        = static_cast<DiscoveryFrame::Type>(raw.type);
    f.player_id   = getFixed(raw.player_id, sizeof(raw.player_id));
    f.hostname    = getFixed(raw.hostname, sizeof(raw.hostname));
    f.listen_port = raw.listen_port;
    switch (raw.state) {
        case 1:  f.state = DiscoveryFrame::DevState::Idle;  break;
        case 2:  f.state = DiscoveryFrame::DevState::Bound; break;
        default: f.state = DiscoveryFrame::DevState::Unknown; break;
    }
    // mode：越界值钳 Unknown（杂包/未来扩展防御）
    switch (raw.mode) {
        case 1:  f.mode = DiscoveryFrame::PlayerMode::Standalone; break;
        case 2:  f.mode = DiscoveryFrame::PlayerMode::Sync;       break;
        default: f.mode = DiscoveryFrame::PlayerMode::Unknown;    break;
    }
    return f;
}

std::string localHostName() {
    // Poco::Environment::nodeName()：跨平台主机名（内部 gethostname/GetComputerName
    // 均已封装；Foundation 已链接，免去显式链 ws2_32 的部署负担）
    try {
        return Poco::Environment::nodeName();
    } catch (const Poco::Exception&) {
        return {}; // 罕见（环境异常）：空主机名，UI 显示为空不致命
    }
}

// ---------------------------------------------------------------------------
// DiscoveryResponder —— display 侧应答器
// ---------------------------------------------------------------------------
bool DiscoveryResponder::start(const std::string& player_id,
                               std::uint16_t tcp_listen_port,
                               BoundFn bound_fn, std::uint16_t bind_port,
                               DiscoveryFrame::PlayerMode mode) {
    if (running_.load()) return true;
    try {
        // 【同机多 display（2026-11-03）】支持同端口双绑定：
        //   * POSIX：SO_REUSEPORT 允许多 socket 绑定同一 UDP 端口（Windows
        //     无此选项，Poco 内部 no-op 不报错）；
        //   * Windows：双绑依赖双方 socket 都设置 SO_REUSEADDR——此语义下
        //     【广播】包会投递给所有同端口 socket（每个 display 实例都收到
        //     editor 查询并各自应答），【单播】包只投一个（定向查询场景）。
        // 绑定失败（对端是未设 reuse 的旧版程序占着端口等）→ 返回 false，
        // 本实例不参与发现（不影响控制通道主流程）。
        socket_.setReusePort(true);
        socket_.bind(Poco::Net::SocketAddress(Poco::Net::IPAddress(), bind_port),
                     true); // SO_REUSEADDR：Windows 双绑关键 + 重启即刻重绑
    } catch (const Poco::Exception&) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        player_id_       = player_id;
        hostname_        = localHostName();
        tcp_listen_port_ = tcp_listen_port;
        mode_            = mode;
        bound_fn_        = std::move(bound_fn);
    }
    running_.store(true);
    recv_thread_ = std::thread(&DiscoveryResponder::recv_loop, this);
    return true;
}

void DiscoveryResponder::stop() {
    if (!running_.exchange(false)) return;
    try { socket_.close(); } catch (...) {}
    if (recv_thread_.joinable()) recv_thread_.join();
}

std::uint16_t DiscoveryResponder::port() const {
    try { return socket_.address().port(); } catch (...) { return 0; }
}

void DiscoveryResponder::recv_loop() {
    char buf[256]; // 查询帧 72B；留余量吞掉杂包
    while (running_.load()) {
        // 200ms 轮询保证 stop() 后线程及时退出
        if (!socket_.poll(Poco::Timespan(200 * 1000),
                          Poco::Net::Socket::SELECT_READ))
            continue;

        Poco::Net::SocketAddress from;
        int n = 0;
        try {
            n = socket_.receiveFrom(buf, sizeof(buf), from);
        } catch (const Poco::Exception&) {
            if (!running_.load()) break;
            continue; // socket 被 stop 关闭 / 瞬时错误
        }

        // 只应答合法 QUERY（RESPONSE/杂包静默丢弃）
        auto q = DiscoveryCodec::decode(buf, static_cast<std::size_t>(n));
        if (!q || q->type != DiscoveryFrame::Type::Query) continue;

        // 组装应答（成员短锁拷贝——bound_fn_ 谓词锁外调用，避免死锁）
        std::string pid, host;
        std::uint16_t port = 0;
        DiscoveryFrame::PlayerMode mode = DiscoveryFrame::PlayerMode::Unknown;
        bool bound = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pid  = player_id_;
            host = hostname_;
            port = tcp_listen_port_;
            mode = mode_;
            if (bound_fn_) bound = bound_fn_();
        }
        DiscoveryFrame resp;
        resp.type        = DiscoveryFrame::Type::Response;
        resp.player_id   = pid;
        resp.hostname    = host;
        resp.listen_port = port;
        resp.mode        = mode;
        resp.state = port == 0 ? DiscoveryFrame::DevState::Unknown
                   : bound     ? DiscoveryFrame::DevState::Bound
                               : DiscoveryFrame::DevState::Idle;
        // 编码回接收缓冲区（已 decode 完，可安全覆盖）；72B 远小于 MTU
        const std::size_t len = DiscoveryCodec::encode(resp, buf);
        try {
            // 应答发往查询包源地址（editor 的临时端口）——无需知道 editor 身份
            socket_.sendTo(buf, static_cast<int>(len), from);
        } catch (const Poco::Exception&) {
            continue; // 发送失败忽略：editor 下轮查询会再来
        }
    }
}

// ---------------------------------------------------------------------------
// DiscoveryBrowser —— editor 侧浏览器
// ---------------------------------------------------------------------------
bool DiscoveryBrowser::start(std::uint16_t discovery_port) {
    if (running_.load()) return true;
    try {
        socket_.bind(Poco::Net::SocketAddress(Poco::Net::IPAddress(), 0),
                     true); // 随机端口（应答目标由 QUERY 源地址自动回填）
        socket_.setBroadcast(true); // 受限广播 255.255.255.255 需要此选项
    } catch (const Poco::Exception&) {
        return false;
    }
    discovery_port_ = discovery_port;
    running_.store(true);
    recv_thread_ = std::thread(&DiscoveryBrowser::recv_loop, this);
    return true;
}

void DiscoveryBrowser::stop() {
    if (!running_.exchange(false)) return;
    try { socket_.close(); } catch (...) {}
    if (recv_thread_.joinable()) recv_thread_.join();
}

void DiscoveryBrowser::scan(const std::string& target) {
    if (!running_.load()) return;
    DiscoveryFrame q;
    q.type = DiscoveryFrame::Type::Query; // 其余字段 QUERY 无效
    unsigned char buf[DiscoveryCodec::kWireSize];
    const std::size_t len = DiscoveryCodec::encode(q, buf);
    try {
        const auto to = Poco::Net::SocketAddress(target, discovery_port_);
        // 连发 2 帧防丢包（局域网 UDP 偶发丢弃；应答去重，多发无害）
        socket_.sendTo(buf, static_cast<int>(len), to);
        socket_.sendTo(buf, static_cast<int>(len), to);
    } catch (const Poco::Exception&) {
        // 广播失败（如无网络接口）：results() 保持上次结果，UI 侧提示
    }
}

std::vector<DiscoveryBrowser::Device> DiscoveryBrowser::results() const {
    const std::int64_t now = steadyNowMs();
    constexpr std::int64_t kStaleMs = 8000; // 8s 无应答视为离线（查询周期数倍）
    std::vector<Device> out;
    std::lock_guard<std::mutex> lock(mutex_);
    out.reserve(found_.size());
    for (const auto& [id, rec] : found_)
        if (now - rec.last_seen_ms <= kStaleMs) out.push_back(rec.dev);
    return out; // 纯读（stale 条目由接收线程清理，const 语义不改动）
}

void DiscoveryBrowser::recv_loop() {
    char buf[256];
    while (running_.load()) {
        if (!socket_.poll(Poco::Timespan(100 * 1000),
                          Poco::Net::Socket::SELECT_READ))
            continue;

        Poco::Net::SocketAddress from;
        int n = 0;
        try {
            n = socket_.receiveFrom(buf, sizeof(buf), from);
        } catch (const Poco::Exception&) {
            if (!running_.load()) break;
            continue;
        }

        auto resp = DiscoveryCodec::decode(buf, static_cast<std::size_t>(n));
        if (!resp || resp->type != DiscoveryFrame::Type::Response) continue;
        if (resp->player_id.empty()) continue; // 无身份的应答无登记价值

        // 去重记录：host 取 UDP 源 IP（display 可能多网卡，应答路径即
        // 连通路径——比 display 自报地址更可靠）；port 取帧内监听端口
        const std::int64_t now_ms = steadyNowMs();
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rec  = found_[resp->player_id];
        rec.dev.player_id   = resp->player_id;
        rec.dev.hostname    = resp->hostname;
        rec.dev.host        = from.host().toString();
        rec.dev.listen_port = resp->listen_port;
        rec.dev.bound = resp->state == DiscoveryFrame::DevState::Bound;
        rec.dev.mode  = resp->mode;
        rec.last_seen_ms    = now_ms;

        // 顺带清理 stale 条目（防长开对话框累积；results() 保持纯读）
        for (auto it = found_.begin(); it != found_.end();) {
            if (now_ms - it->second.last_seen_ms > 8000)
                it = found_.erase(it);
            else
                ++it;
        }
    }
}

} // namespace ms
