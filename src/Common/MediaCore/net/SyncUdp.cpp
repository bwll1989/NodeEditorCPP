// ============================================================================
//  SyncUdp.cpp —— UDP 心跳传输实现
// ============================================================================

#include "net/SyncUdp.hpp"

#include <cstring>

#include <Poco/Exception.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace ms {

bool SyncUdp::start(std::uint16_t bind_port) {
    if (running_.load()) return true;

    try {
        // reuseAddress：进程重启即刻重绑；同时支持本机多实例（不同端口）
        socket_.bind(Poco::Net::SocketAddress(Poco::Net::IPAddress(), bind_port), true);
    } catch (const Poco::Exception&) {
        return false;
    }

#ifdef _WIN32
    // 【句柄防泄漏 2026-11-13】QProcess/CreateProcess(bInheritHandles=TRUE)
    // 派生子进程时会把本 socket 句柄整表复制——旧进程退出后"幽灵 socket"
    // 随子进程存活并劫持端口流量（display 状态上报黑洞实证）。标记为不可
    // 继承，从源头掐断（详见 Common/ProcessGuard/SpawnNoInherit.hpp 头注释）。
    {
        HANDLE h = reinterpret_cast<HANDLE>(socket_.impl()->sockfd());
        ::SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0);
    }
#endif

    running_.store(true);
    recv_thread_ = std::thread(&SyncUdp::recv_loop, this);
    return true;
}

void SyncUdp::stop() {
    if (!running_.exchange(false)) return;
    try { socket_.close(); } catch (...) {}
    if (recv_thread_.joinable()) recv_thread_.join();
}

std::uint16_t SyncUdp::port() const {
    try { return socket_.address().port(); } catch (...) { return 0; }
}

void SyncUdp::set_remote(const std::string& host, std::uint16_t port) {
    // remote_ 与 send() 的读取跨线程，必须持锁写（详见头文件线程模型注释）
    std::lock_guard<std::mutex> lock(remote_mutex_);
    try { remote_ = Poco::Net::SocketAddress(host, port); } catch (...) { remote_ = {}; }
}

bool SyncUdp::send(const SyncFrame& frame) {
    // 锁内拷出目标地址、锁外发送：最小化临界区（sendTo 可能阻塞在内核）
    Poco::Net::SocketAddress target;
    {
        std::lock_guard<std::mutex> lock(remote_mutex_);
        target = remote_;
    }
    if (target.host().isWildcard()) return false; // 未设置目标
    return send_to(frame, target);
}

bool SyncUdp::send_to(const SyncFrame& frame, const Poco::Net::SocketAddress& to) {
    unsigned char buf[SyncFrameCodec::kWireSize];
    const std::size_t n = SyncFrameCodec::encode(frame, buf);
    try {
        // UDP 单次 sendTo：56B 远小于 MTU，必为原子报文（无分片丢序问题在此层）
        const int sent = socket_.sendTo(buf, static_cast<int>(n), to);
        return sent == static_cast<int>(n);
    } catch (const Poco::Exception&) {
        return false; // ICMP 不可达会在下一次收发抛出，忽略即可（UDP 语义）
    }
}

void SyncUdp::recv_loop() {
    char buf[2048]; // 心跳 56B；留余量吞掉网络上的杂包
    while (running_.load()) {
        // 200ms 轮询保证 stop() 后线程及时退出
        if (!socket_.poll(Poco::Timespan(200 * 1000), Poco::Net::Socket::SELECT_READ))
            continue;

        Poco::Net::SocketAddress from;
        int n = 0;
        try {
            n = socket_.receiveFrom(buf, sizeof(buf), from);
        } catch (const Poco::Exception&) {
            if (!running_.load()) break;
            continue; // socket 被 stop 关闭 / 瞬时错误
        }

        // 魔数/版本不符 → nullopt → 静默丢弃（下帧即最新，UDP 语义）
        auto frame = SyncFrameCodec::decode(buf, static_cast<std::size_t>(n));
        if (frame && on_frame_) on_frame_(*frame, from);
    }
}

} // namespace ms
