// ============================================================================
//  DisplayConnector.cpp —— Editor 侧显示端连接器实现（第五迭代）
//
//  重连策略（与 PlayerRuntime::watchdogLoop 同源）：断线后指数退避重试
//  （1s 起步 ×2 封顶 5s），健康即归位——显示端离线/重启后 Editor 自动
//  重新接入，零人工干预（"显示端不需要做什么操作"的另一面）。
// ============================================================================

#include "net/DisplayConnector.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

namespace ms {

using ms_proto::Envelope;

namespace {
/// steady 时钟毫秒读数（看门狗调度基准；日志时间戳同源）
long long steadyNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
} // namespace

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------
void DisplayConnector::start() {
    if (running_.exchange(true)) return; // 幂等
    watchdog_ = std::thread(&DisplayConnector::watchdogLoop, this);
}

void DisplayConnector::stop() {
    running_.store(false);
    if (watchdog_.joinable()) watchdog_.join();

    // 断开并 join 全部 recv 线程。
    // 【顺序关键】client->stop() 必须在锁外：recv 线程的断连回调
    // （forwardDisconnected）会拿 mutex_ —— 持锁 join 必死锁。
    std::vector<std::shared_ptr<ControlClient>> clients;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, slot] : slots_)
            clients.push_back(std::move(slot.client));
        slots_.clear();
    }
    for (auto& c : clients)
        if (c) c->stop();
}

// ---------------------------------------------------------------------------
// makeClient —— 创建并装配好回调的 ControlClient
// add() 首建与看门狗重建共用：回调只补全 display_id 后转发本类，行为
// 与连接何时创建无关（断开回调对已不存在的条目自然幂等）。
// ---------------------------------------------------------------------------
std::shared_ptr<ControlClient> DisplayConnector::makeClient(
        const std::string& display_id) {
    auto client = std::make_shared<ControlClient>();
    client->set_on_envelope([this, display_id](const Envelope& env) {
        forwardEnvelope(display_id, env);
    });
    client->set_on_disconnected([this, display_id]() {
        // 断开即清地址与身份标记（重连成功回填）；先改状态再通知上层
        {
            std::lock_guard<std::mutex> lk(mutex_);
            const auto it = slots_.find(display_id);
            if (it != slots_.end()) {
                it->second.info.connected  = false;
                it->second.info.local_host.clear();
                it->second.info.peer_host.clear();
                it->second.registered      = false; // 重连后等新注册帧再置位
            }
        }
        forwardDisconnected(display_id);
    });
    return client;
}

// ---------------------------------------------------------------------------
// 登记管理
// ---------------------------------------------------------------------------
bool DisplayConnector::add(const std::string& display_id,
                           const std::string& host, std::uint16_t port,
                           const std::string& pairing_code) {
    if (display_id.empty() || host.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (slots_.count(display_id)) return false; // 重复登记

    Slot slot;
    slot.info.display_id  = display_id;
    slot.info.host        = host;
    slot.info.port        = port ? port : std::uint16_t{17100};
    slot.info.enabled     = true;
    slot.info.pairing_code = pairing_code;
    slot.info.connected   = false; // 看门狗接入后置 true

    // ControlClient 回调包装：捕获 display_id（值拷贝；slots_ 中的条目
    // 由 remove/stop 先 join 回调线程再 erase，捕获不会悬空）
    slot.client = makeClient(display_id);

    // next_attempt=0：看门狗下一轮（≤400ms）即尝试首连
    slots_[display_id] = std::move(slot);
    // 【排障走 stderr】登记成功留痕（"自动添加显示端"只代表 UI 调用，
    // 真正入表在此——联测定位"看门狗不动"必须知道登记是否成功）
    std::fprintf(stderr, "[displays] 已登记: %s@%s:%u（看门狗 ≤400ms 内首连）\n",
                 display_id.c_str(), host.c_str(),
                 (unsigned)(port ? port : 17100));
    std::fflush(stderr);
    return true;
}

bool DisplayConnector::remove(const std::string& display_id) {
    // 1) 锁内摘出 client（shared_ptr 拷贝走所有权，map 不再持有；若看门狗
    //    线程正持同一条目的拷贝在 connect，析构延迟到其用毕——无 UAF）
    std::shared_ptr<ControlClient> client;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = slots_.find(display_id);
        if (it == slots_.end()) return false;
        client = std::move(it->second.client);
        slots_.erase(it);
    }
    // 2) 锁外断开 + join recv 线程（其断连回调对已 erase 的 id 查找落空，
    //    自然幂等）。【调用约定】remove 只在 UI 线程调用——recv 线程自己的
    //    回调里调用会 join 自己死锁（头文件注释已声明）
    if (client) client->stop();
    return true;
}

bool DisplayConnector::setEnabled(const std::string& display_id, bool enabled) {
    // 【与 remove 同构】锁内只摘所有权，stop 必须锁外：recv 线程的断连
    // 回调会抢 mutex_——持锁 join 构成死锁窗口（回调已触发时必锁死）
    std::shared_ptr<ControlClient> client;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = slots_.find(display_id);
        if (it == slots_.end()) return false;
        it->second.info.enabled = enabled;
        if (!enabled) {
            // 停用即断开（recv 线程走断连回调：清地址 + 通知上层）；
            // client 置空占位（重启用时看门狗重建）
            client = std::move(it->second.client);
            it->second.next_attempt_ms = 0; // 重新启用时立即尝试
        } else {
            it->second.next_attempt_ms = 0; // 启用后看门狗立即接入
        }
    }
    if (client) client->stop();
    return true;
}

// ---------------------------------------------------------------------------
// 发送 / 查询
// ---------------------------------------------------------------------------
bool DisplayConnector::sendTo(const std::string& display_id,
                              const Envelope& env) {
    // 锁内拷贝 shared_ptr（防发送期间 remove 析构），锁外发送
    std::shared_ptr<ControlClient> client;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = slots_.find(display_id);
        if (it == slots_.end() || !it->second.info.enabled) return false;
        client = it->second.client;
    }
    return client ? client->send(env) : false;
}

std::size_t DisplayConnector::broadcast(const Envelope& env) {
    // 锁内快照 shared_ptr（sendBytes 走各 client 内部 socket_mutex，不持本类锁）
    std::vector<std::shared_ptr<ControlClient>> targets;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, slot] : slots_) {
            if (!slot.info.enabled) continue;
            if (slot.client && slot.client->is_connected())
                targets.push_back(slot.client);
        }
    }
    std::size_t ok = 0;
    for (auto& c : targets) {
        const bool sent = c->send(env);
        // 【排障走 stderr】广播送达逐端留痕（重锚等低频信令）：
        // "targets 空但上层认为已广播" / "send 假成功"两类问题定位关键
        std::fprintf(stderr, "[%lld][displays] broadcast: targets=%zu send=%s\n",
                     steadyNowMs(), targets.size(), sent ? "ok" : "FAIL");
        if (sent) ++ok;
    }
    if (targets.empty())
        std::fprintf(stderr, "[%lld][displays] broadcast: 无已连接目标（信令未发出）\n",
                     steadyNowMs());
    return ok;
}

std::vector<DisplayConnector::Entry> DisplayConnector::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Entry> out;
    out.reserve(slots_.size());
    for (const auto& [id, slot] : slots_) {
        Entry e = slot.info;
        e.connected  = slot.client && slot.client->is_connected() && e.enabled;
        e.registered = e.connected && slot.registered; // 注册帧确认过身份
        if (!e.connected) { e.local_host.clear(); e.peer_host.clear(); }
        out.push_back(std::move(e));
    }
    return out;
}

void DisplayConnector::setRegistered(const std::string& display_id,
                                     bool registered) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = slots_.find(display_id);
    if (it != slots_.end()) it->second.registered = registered;
}

std::size_t DisplayConnector::connected_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t n = 0;
    for (const auto& [id, slot] : slots_)
        if (slot.info.enabled && slot.client && slot.client->is_connected())
            ++n;
    return n;
}

std::string DisplayConnector::peerHostOf(const std::string& display_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = slots_.find(display_id);
    if (it == slots_.end()) return {};
    return it->second.info.peer_host;
}

std::string DisplayConnector::localHostOf(const std::string& display_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = slots_.find(display_id);
    if (it == slots_.end()) return {};
    return it->second.info.local_host;
}

// ---------------------------------------------------------------------------
// 看门狗：巡检 + 重连（指数退避 1s→5s 封顶；健康归位）
// ---------------------------------------------------------------------------
void DisplayConnector::watchdogLoop() {
    constexpr long kBackoffStartMs = 1000;
    constexpr long kBackoffMaxMs   = 5000;

    // 【排障走 stderr】看门狗启动留痕（线程死亡/未启动的一锤定音证据）
    std::fprintf(stderr, "[displays] 看门狗已启动（400ms 巡检）\n");
    std::fflush(stderr);

    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        if (!running_.load()) break;

        const long long now_ms = steadyNowMs();

        // ---- 锁内快照待连接目标（阻塞 connect 必须在锁外）----
        struct Target {
            std::string id, host, pairing_code;
            std::uint16_t port;
        };
        std::vector<Target> targets;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& [id, slot] : slots_) {
                if (!slot.info.enabled) continue;
                if (slot.client && slot.client->is_connected()) continue;
                if (now_ms < slot.next_attempt_ms) continue;
                targets.push_back({id, slot.info.host,
                                   slot.info.pairing_code, slot.info.port});
            }
        }
        if (targets.empty()) continue;

        for (const auto& t : targets) {
            if (!running_.load()) break;

            // 【排障走 stderr】connect 发起留痕（netstat 级别的证据：
            // "看门狗是否尝试过连接"由此行直接回答）
            std::fprintf(stderr, "[displays] connect 尝试: %s → %s:%u\n",
                         t.id.c_str(), t.host.c_str(), (unsigned)t.port);
            std::fflush(stderr);

            // 锁内拷贝 shared_ptr（延长生命期覆盖 3s 阻塞 connect——期间
            // remove/setEnabled 析构槽位也不会销毁 client，无 use-after-free）。
            // 【2026-11-05 修复】setEnabled(false) 会把槽位 client move 走
            // 置空（停用即断开）；重新启用后此处取到 nullptr——必须原地
            // 重建并回填槽位，否则条件"!client → continue"使该显示端
            // 永远不再被连接（用户报"断开后无法再连上"的根因）。
            std::shared_ptr<ControlClient> client;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                const auto it = slots_.find(t.id);
                if (it == slots_.end()) continue; // 已被注销
                client = it->second.client;
                if (!client) {
                    client = makeClient(t.id);
                    it->second.client = client;
                    std::fprintf(stderr,
                                 "[%lld][displays] 重建连接对象: %s"
                                 "（停用期间槽位已清空）\n",
                                 steadyNowMs(), t.id.c_str());
                    std::fflush(stderr);
                }
            }
            if (!client) continue;

            // 阻塞 connect（3s 超时；display 个位数，串行重试无碍）
            const bool ok = client->connect(t.host, t.port);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                const auto it = slots_.find(t.id);
                if (it != slots_.end()) {
                    if (ok) {
                        // 接入成功：回填地址 + 退避归位（用持有时期内的
                        // client 读取地址——指针在本次临界区内有效）
                        it->second.info.connected  = true;
                        it->second.registered      = false; // 等新注册帧
                        it->second.info.local_host = client->local_host();
                        it->second.info.peer_host  = client->peer_host();
                        it->second.next_attempt_ms = 0;
                        it->second.backoff_ms      = kBackoffStartMs;
                    } else {
                        it->second.backoff_ms =
                            std::min(it->second.backoff_ms * 2,
                                     kBackoffMaxMs);
                        it->second.next_attempt_ms =
                            steadyNowMs() + it->second.backoff_ms;
                        std::printf("[%lld][displays] %s:%u 连接失败（%ldms 后重试）\n",
                            steadyNowMs(),
                                    t.host.c_str(), t.port,
                                    it->second.backoff_ms);
                    }
                }
            }
            if (ok) {
                std::printf("[%lld][displays] 已连接: %s (%s:%u 本端=%s)\n",
                            steadyNowMs(),
                            t.id.c_str(), t.host.c_str(), t.port,
                            localHostOf(t.id).c_str());
                // ---- 配对认证帧（连接后首帧；2026-11-03 多 Editor 隔离）----
                // 尽早发送：display 端在认证通过前丢弃本连接的业务帧
                // （Ack/Scene 等），发送越早其"等待认证"窗口越短。
                // display 端对认证前的帧有缓存回放兜底，此处无需强同步。
                if (!t.pairing_code.empty()) {
                    Envelope auth;
                    auth.mutable_auth()->set_pairing_code(t.pairing_code);
                    if (!client->send(auth))
                        std::fprintf(stderr,
                                     "[displays] 配对帧发送失败: %s"
                                     "（display 端将丢弃业务帧直至重连）\n",
                                     t.id.c_str());
                    else
                        std::fprintf(stderr,
                                     "[%lld][displays] 配对帧已发送: %s\n",
                                     steadyNowMs(), t.id.c_str());
                }
                if (on_state_) on_state_(t.id, true);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 回调转发（recv 线程上下文）
// ---------------------------------------------------------------------------
void DisplayConnector::forwardEnvelope(const std::string& display_id,
                                       const Envelope& env) {
    if (on_envelope_) on_envelope_(display_id, env);
}

void DisplayConnector::forwardDisconnected(const std::string& display_id) {
    // 【走 stderr】断连是关键事件，不能被 stdout 全缓冲延迟（误判时序）
    std::fprintf(stderr, "[%lld][displays] 断开: %s（看门狗将自动重连）\n",
                 steadyNowMs(), display_id.c_str());
    std::fflush(stderr);
    if (on_state_) on_state_(display_id, false);
}

} // namespace ms
