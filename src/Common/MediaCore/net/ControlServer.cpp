// ============================================================================
//  ControlServer.cpp —— TCP 控制通道·服务端实现
// ============================================================================

#include "net/ControlServer.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>

#include <Poco/Exception.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/StreamSocket.h> // 端口占用探测（exclusive_bind）

#include "net/FrameCodec.hpp"

namespace ms {

// 【排障】stderr 日志统一带 steady 毫秒时戳：多进程日志对齐时间线的
// 唯一手段（stdout 全缓冲 + stderr 实时的双流交错无法靠文件顺序还原时序）
namespace {
long long tsMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
} // namespace

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------
bool ControlServer::start(std::uint16_t port, bool exclusive_bind) {
    if (running_.load()) return true; // 已在运行（幂等）

    // ---- 独占语义：bind 前连接探测（Windows 双绑陷阱绕行）--------------
    // Windows 上带 SO_REUSEADDR 的监听 socket，第二实例 bind 同端口也
    // "成功"，但连接只投递给先绑者——后者"假监听"永远收不到 Editor
    // （2026-11-03 联测实测：两实例都 LISTENING 17100，扫描列表却只见
    // 假象）。connect 探测能识破：有活监听者时 connect 必成功，端口
    // 空闲时立即被拒。探测到占用直接返回 false，调用方 fallback 系统分配
    // 端口；POSIX 无双绑问题，探测同样成立（有监听者就不该再绑）。
    // 已知限制：两实例严格同一瞬间启动（同毫秒竞态）仍可能双绑——
    // 手工/脚本启动天然有间隔，可接受。
    if (exclusive_bind && port != 0) {
        Poco::Net::StreamSocket probe;
        try {
            probe.connect(Poco::Net::SocketAddress("127.0.0.1", port),
                          Poco::Timespan(500 * 1000)); // 本机回环 500ms 足够
            probe.close();
            return false; // 有活监听者：端口"真占用"（无论绑定者是谁）
        } catch (const Poco::Exception&) {
            // 连接被拒/超时 = 端口无监听者（TIME_WAIT 残留不算），可绑定
        }
    }

    try {
        // SO_REUSEADDR：重启即刻重绑（TIME_WAIT 不卡端口）
        acceptor_.bind(Poco::Net::SocketAddress(Poco::Net::IPAddress(), port),
                       true);
        acceptor_.listen();
    } catch (const Poco::Exception& e) {
        // 端口占用等：交给调用方决定（打日志/退出）。
        // 【诊断留痕】显示端多实例 fallback 依赖"bind 失败的真实原因"——
        // 端口占用/权限不足在此一律可辨
        std::fprintf(stderr, "[ctrl-server] 监听绑定失败 port=%u: %s\n",
                     static_cast<unsigned>(port), e.displayText().c_str());
        return false;
    }

    running_.store(true);
    accept_thread_ = std::thread(&ControlServer::accept_loop, this);
    return true;
}

void ControlServer::stop() {
    if (!running_.exchange(false)) return; // 幂等：未运行直接返回

    // 【排障走 stderr】stop 的调用时机是连接生命周期关键事件：
    // 若会话中途出现本日志，说明有非退出路径调用了 stop（连接被静默掐断）
    std::fprintf(stderr, "[%lld][ctrl-server] stop() 被调用\n", tsMs());
    std::fflush(stderr);

    // 1) 关监听 → accept 线程的 poll 返回 false / 抛异常退出
    try { acceptor_.close(); } catch (...) {}

    // 2) 收割线程对象并关闭全部连接
    //    【注意】join 必须在锁外：recv 线程退出时要拿 conn_mutex_ 做
    //    conns_.erase()，若持锁 join 必死锁。
    std::vector<std::thread> to_join;
    {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        for (auto& [id, conn] : conns_) {
            try { conn->socket.shutdownSend(); } catch (...) {}
            try { conn->socket.close();       } catch (...) {}
            // thread 对象只能移动一次；recv 线程自身持有 Conn 的
            // shared_ptr 拷贝，clear() 不会使其悬空
            if (conn->recv_thread.joinable())
                to_join.push_back(std::move(conn->recv_thread));
        }
        conns_.clear();
    }

    // 3) join 全部线程（accept + 各连接 recv）
    if (accept_thread_.joinable()) accept_thread_.join();
    for (auto& t : to_join) t.join();
}

std::uint16_t ControlServer::port() const {
    try { return acceptor_.address().port(); } catch (...) { return 0; }
}

// ---------------------------------------------------------------------------
// accept 线程：poll 轮询 → accept → 起 recv 线程
// ---------------------------------------------------------------------------
void ControlServer::accept_loop() {
    while (running_.load()) {
        // 100ms 无连接则回来检查 running_（保证 stop() 及时退出）
        // 【防御】stop() 关闭 acceptor 与 poll 竞态时可能抛异常——接住退出
        bool readable = false;
        try {
            readable = acceptor_.poll(Poco::Timespan(100 * 1000),
                                      Poco::Net::Socket::SELECT_READ);
        } catch (const Poco::Exception& e) {
            // 【排障走 stderr】accept 线程死亡 = 后续所有接入永久失败
            // （联测出现过"重连永远无法被 accept"的偶发，死亡原因是关键）
            std::fprintf(stderr, "[%lld][ctrl-server] accept 线程异常退出: %s\n",
                         tsMs(),
                         e.displayText().c_str());
            std::fflush(stderr);
            break; // socket 已关：退出（running_ 大概率同时被置 false）
        }

        // 低频收割死连接（复用 accept 轮询节拍）：recv 线程退出只置
        // alive=false，条目与 fd 原先只在进程退出时回收——长跑 Editor
        // 每次 Player 断线泄漏一个 fd + 一个死条目（broadcast/count 遍历
        // 持续膨胀）。recv_thread 已退出（或只剩收尾几条指令），join 立即返回。
        // 【不门控 readable】收割若挂在"有新连接到达"条件下，长跑且长时间
        // 无新接入的 Editor 永远不回收死条目——改为每 100ms 轮询节拍执行
        // （连接数个位数量级，空跑一遍遍历开销可忽略）。
        reap_dead_conns();

        if (!readable) continue;

        Poco::Net::StreamSocket raw;
        try {
            raw = acceptor_.acceptConnection();
        } catch (const Poco::Exception&) {
            continue; // 竞态关闭/坏连接：下轮再试
        }
        // TCP_NODELAY：控制信令（重锚命令）要低延迟，禁用 Nagle 小包合并
        try { raw.setNoDelay(true); } catch (...) {}

        const std::uint64_t cid = next_conn_id_;
        auto conn = std::make_shared<Conn>(std::move(raw));
        // 记录对端地址（供 Editor 构造 Player 的 UDP 心跳目标；accept 后可查）
        try { conn->peer_addr = conn->socket.peerAddress(); } catch (...) {}

        {
            // 用 unique_lock：停止竞态路径需先解锁再清理 socket
            std::unique_lock<std::mutex> lock(conn_mutex_);
            // 【竞态防御·锁内复查 running_】stop() 可能在本线程 poll/accept
            // 期间已置 running_=false 并 clear 了 conns_：若此处仍插入，产生
            // "无人收割"的孤儿条目——stop 已过 clear 阶段，其 recv_thread
            // 再无人 move+join，进程退出时条目析构 joinable 线程 →
            // std::terminate。锁内复查关闭该窗口（stop 持锁 clear 与本
            // 复查互斥，二者必有先后且序内一致）。
            if (!running_.load()) {
                lock.unlock();
                try { conn->socket.close(); } catch (...) {}
                std::fprintf(stderr,
                             "[%lld][ctrl-server] accept 后发现已 stop：丢弃 cid=%llu\n",
                             tsMs(), (unsigned long long)cid);
                std::fflush(stderr);
                continue; // 循环条件 running_ 已 false，下轮即退出
            }
            conns_[cid] = conn;
            ++next_conn_id_;
            // recv 线程赋值必须在【同一临界区】内：stop() 持 conn_mutex_
            // 对 recv_thread 做 move——若赋值在锁外，stop 可恰好插入
            // "map 已插入、线程未赋值"的窗口，move 走空线程后本线程再
            // 赋值 → 条目析构 joinable 线程 → std::terminate
            conn->recv_thread = std::thread(&ControlServer::recv_loop, this, cid, conn);
        }

        // 【排障走 stderr】accept 是连接生命周期的关键事件（显示端联测
        // 排查"半开连接"必须知道每条连接的建立时刻）
        std::fprintf(stderr, "[%lld][ctrl-server] accept cid=%llu peer=%s\n",
                     tsMs(),
                     (unsigned long long)cid,
                     conn->peer_addr.host().toString().c_str());
        std::fflush(stderr);

        // 新连接通知（第五迭代）：accept 完成后触发（首帧到达前）。
        // 显示端监听模式借此向接入的 Editor 立即上报 Register。
        if (on_connected_) on_connected_(cid);
    }
}

// ---------------------------------------------------------------------------
// 死连接收割（accept 线程调用；与 stop() 的 move 竞态由 conn_mutex_ 互斥）
// ---------------------------------------------------------------------------
void ControlServer::reap_dead_conns() {
    std::vector<std::thread> to_join;
    {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        for (auto it = conns_.begin(); it != conns_.end();) {
            auto& conn = it->second;
            if (conn->alive.load()) { ++it; continue; }
            // recv 线程已退出（alive 最后置位）→ move 线程对象锁外 join，
            // 条目删除（fd/socket/缓冲随之回收）
            if (conn->recv_thread.joinable())
                to_join.push_back(std::move(conn->recv_thread));
            it = conns_.erase(it);
        }
    }
    // join 必须在锁外（recv 线程收尾的断连回调会拿 conn_mutex_）
    for (auto& t : to_join) t.join();
}

// ---------------------------------------------------------------------------
// recv 线程：收字节 → 分帧 → 回调
// ---------------------------------------------------------------------------
void ControlServer::recv_loop(std::uint64_t conn_id, const ConnPtr& conn) {
    for (;;) {
        if (!running_.load()) break;

        // 200ms 无数据则回来检查 running_（stop 关 socket 后 poll 也会返回）
        bool readable = false;
        try {
            readable = conn->socket.poll(Poco::Timespan(200 * 1000),
                                         Poco::Net::Socket::SELECT_READ);
        } catch (const Poco::Exception& e) {
            std::fprintf(stderr, "[%lld][ctrl-server] conn %llu poll 异常: %s\n",
                     tsMs(),
                         (unsigned long long)conn_id, e.displayText().c_str());
            std::fflush(stderr);
            break;
        }
        if (!readable) continue;

        char buf[64 * 1024];
        int n = 0;
        try {
            n = conn->socket.receiveBytes(buf, sizeof(buf));
        } catch (const Poco::Exception& e) {
            // 【排障走 stderr】receive 异常（对端 RST/网络栈错误）需要留痕：
            // 联测中出现过"第一条连接被静默重置"的偶发，错误码是关键线索
            std::fprintf(stderr, "[%lld][ctrl-server] conn %llu receive 异常: %s\n",
                     tsMs(),
                         (unsigned long long)conn_id, e.displayText().c_str());
            std::fflush(stderr);
            break; // 对端 reset / socket 已被 stop 关闭
        }
        if (n <= 0) {
            // 【排障走 stderr】对端优雅关闭（n=0）——连接生命周期关键事件
            std::fprintf(stderr, "[%lld][ctrl-server] conn %llu 对端关闭 (n=%d)\n",
                     tsMs(),
                         (unsigned long long)conn_id, n);
            std::fflush(stderr);
            break; // 0 = 对端优雅关闭
        }

        // 【排障走 stderr】TCP 信令帧到达即留痕（低频信令：注册/场景/补丁/
        // 重锚）——联测定位"editor 写成功但 display 未收到"必须知道字节
        // 是否到达对端传输层
        std::fprintf(stderr, "[%lld][ctrl-server] conn %llu 收到 %d 字节\n",
                     tsMs(),
                     (unsigned long long)conn_id, n);
        std::fflush(stderr);

        conn->recv_buf.append(buf, static_cast<std::size_t>(n));

        // 循环出帧：一次 receive 可能带来多帧（粘包）
        ms_proto::Envelope env;
        for (;;) {
            const int r = FrameCodec::tryExtract(conn->recv_buf, env);
            if (r == 0) break;      // 半包：等下一批字节
            if (r < 0)  {           // 协议错误：踢掉（长度超限/pb 解析失败）
                // 【排障走 stderr】协议错误是连接被踢的唯一主动路径，
                // 必须留痕（缓冲大小 + 错误码定位帧损坏原因）
                std::fprintf(stderr,
                             "[%lld][ctrl-server] conn %llu 协议错误 r=%d（缓冲 %zu 字节）\n",
                     tsMs(),
                             (unsigned long long)conn_id, r, conn->recv_buf.size());
                std::fflush(stderr);
                try { conn->socket.close(); } catch (...) {}
                break;
            }
            // Register 顺手回填 player_id（find_by_player_id / 状态展示用）
            if (env.has_register_() && !env.register_().player_id().empty()) {
                std::lock_guard<std::mutex> lock(conn_mutex_);
                // find 而非 []：stop() 清空 map 后不得再插入悬空条目
                const auto it = conns_.find(conn_id);
                if (it != conns_.end()) it->second->player_id = env.register_().player_id();
            }
            if (on_envelope_) on_envelope_(conn_id, env);
        }
        if (!running_.load()) break;
    }

    // 断连回调（无论正常关闭还是错误）——只触发一次
    // 【关键】不要在这里 conns_.erase(self)：Conn 的 recv_thread 成员若在
    // 线程退出瞬间析构且仍 joinable，会 std::terminate（abort 退出码 3）。
    // 打 alive 标记，条目由 stop() join 全部线程后统一清理。
    conn->alive.store(false);
    // 【排障走 stderr】连接结束事件（与 accept 日志配对，诊断半开/误断）
    std::fprintf(stderr, "[%lld][ctrl-server] conn %llu 结束\n",
                 tsMs(),
                 (unsigned long long)conn_id);
    std::fflush(stderr);
    if (!conn->notified.exchange(true)) {
        if (on_disconnected_) on_disconnected_(conn_id);
    }
}

// ---------------------------------------------------------------------------
// 发送
// ---------------------------------------------------------------------------
bool ControlServer::send_to(std::uint64_t conn_id, const ms_proto::Envelope& env) {
    ConnPtr conn;
    {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        const auto it = conns_.find(conn_id);
        if (it == conns_.end() || !it->second->alive.load()) return false;
        conn = it->second;
    }
    const std::string frame = FrameCodec::encode(env);
    try {
        std::lock_guard<std::mutex> slock(conn->send_mutex);
        // 全帧写入校验（同 ControlClient::send 的说明）
        const int sent = conn->socket.sendBytes(frame.data(),
                                                static_cast<int>(frame.size()));
        return sent == static_cast<int>(frame.size());
    } catch (const Poco::Exception&) {
        return false; // 发送失败：对端大概率已断开，等 recv 线程走断连流程
    }
}

// ---------------------------------------------------------------------------
// 踢除指定连接（配对码校验失败时由 display 调用；recv 线程上下文安全）
// ---------------------------------------------------------------------------
bool ControlServer::close(std::uint64_t conn_id) {
    ConnPtr conn;
    {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        const auto it = conns_.find(conn_id);
        if (it == conns_.end() || !it->second->alive.load()) return false;
        conn = it->second;
    }
    // 【只关 socket，不 join 线程】调用方可能在被踢连接自身的 recv 线程
    // 上下文（on_envelope 回调里校验配对码失败）——这里 join 自己会死锁。
    // shutdown+close 令该连接的 receiveBytes 立即出错返回 → recv 线程
    // 置 alive=false → on_disconnected 回调 → accept 线程收割，正常走
    // 既有断连流程（回调只发一次的 notified 护栏保证不重复通知）。
    try {
        conn->socket.shutdownSend();
        conn->socket.close();
    } catch (const Poco::Exception&) {
        // 已断/半开：无碍，recv 线程同样会退出
    }
    return true;
}

std::size_t ControlServer::broadcast(const ms_proto::Envelope& env) {
    // 锁内快照存活连接（ConnPtr 拷贝延长生命期），锁外逐个发送——
    // 与 send_to 同构：sendBytes 是阻塞语义（对端接收窗口满时无限期），
    // 持 conn_mutex_ 发送会让一条卡死连接冻结整个控制面（accept/回填全停）
    std::vector<ConnPtr> targets;
    {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        for (auto& [id, conn] : conns_)
            if (conn->alive.load()) targets.push_back(conn);
    }
    const std::string frame = FrameCodec::encode(env);
    std::size_t sent_ok = 0;
    for (auto& conn : targets) {
        try {
            std::lock_guard<std::mutex> slock(conn->send_mutex);
            // 全帧写入校验（部分写入 = 帧损坏，不计入送达数）
            const int n = conn->socket.sendBytes(frame.data(),
                                                 static_cast<int>(frame.size()));
            if (n == static_cast<int>(frame.size())) ++sent_ok;
        } catch (const Poco::Exception&) { /* 断连由 recv 线程处理 */ }
    }
    return sent_ok;
}

// ---------------------------------------------------------------------------
// 连接元数据
// ---------------------------------------------------------------------------
std::size_t ControlServer::connection_count() const {
    std::size_t n = 0;
    std::lock_guard<std::mutex> lock(conn_mutex_);
    for (const auto& [id, conn] : conns_)
        if (conn->alive.load()) ++n;
    return n;
}

std::uint64_t ControlServer::find_by_player_id(const std::string& player_id) const {
    std::lock_guard<std::mutex> lock(conn_mutex_);
    for (const auto& [id, conn] : conns_)
        if (conn->alive.load() && conn->player_id == player_id) return id;
    return 0;
}

std::string ControlServer::player_id_of(std::uint64_t conn_id) const {
    std::lock_guard<std::mutex> lock(conn_mutex_);
    const auto it = conns_.find(conn_id);
    if (it == conns_.end() || !it->second->alive.load()) return {};
    return it->second->player_id;
}

std::string ControlServer::peer_host_of(std::uint64_t conn_id) const {
    std::lock_guard<std::mutex> lock(conn_mutex_);
    const auto it = conns_.find(conn_id);
    if (it == conns_.end() || !it->second->alive.load()) return {};
    try { return it->second->peer_addr.host().toString(); } catch (...) { return {}; }
}

} // namespace ms
