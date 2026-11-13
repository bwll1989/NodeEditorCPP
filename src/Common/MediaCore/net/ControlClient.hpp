// ============================================================================
//  ControlClient.hpp —— TCP 控制通道·客户端（Player 侧，§6.2）
//
//  与 ControlServer 对称的精简版：
//    * connect(host, port) 后起一个接收线程（poll 200ms 轮询，可及时停止）；
//    * send(envelope) 线程安全（内部 send_mutex）；
//    * 断线回调上报，重连策略留给上层 Runtime（MVP：Player 直接退出报错，
//      由守护脚本/人工重启；自动重连在 P4 稳定性阶段补）。
// ============================================================================

#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <Poco/Net/StreamSocket.h>

#include "sync.pb.h" // ms_proto::Envelope

namespace ms {

class ControlClient {
public:
    using EnvelopeHandler   = std::function<void(const ms_proto::Envelope&)>;
    using StateHandler      = std::function<void()>; // 连接建立 / 断开通知

    ControlClient() = default;
    ~ControlClient() { stop(); }

    ControlClient(const ControlClient&) = delete;
    ControlClient& operator=(const ControlClient&) = delete;

    /// 连接 Editor（阻塞直到 TCP 握手完成或失败）
    /// \return 是否成功
    bool connect(const std::string& host, std::uint16_t port);

    /// 断开并 join 接收线程（幂等）
    void stop();

    bool is_connected() const { return running_.load(); }

    /// 本端地址的主机部分（连接成功后有效；第五迭代：Editor 主动连接
    /// 显示端后，Ack.udp_host 用它——比"TCP 对端 IP"反推更准确）。
    /// \return 如 "192.168.0.50"；未连接返回空串
    std::string local_host() const {
        try { return socket_.address().host().toString(); } catch (...) { return {}; }
    }

    /// 对端地址的主机部分（第五迭代：DisplayConnector 记录 display 的 IP，
    /// 作为 UDP 心跳单播目标的主机部分）。\return 未连接返回空串
    std::string peer_host() const {
        try { return socket_.peerAddress().host().toString(); }
        catch (...) { return {}; }
    }

    // ---- 回调（connect 之前设置）----
    void set_on_envelope(EnvelopeHandler h) { on_envelope_ = std::move(h); }
    void set_on_connected(StateHandler h)   { on_connected_   = std::move(h); }
    void set_on_disconnected(StateHandler h){ on_disconnected_ = std::move(h); }

    /// 发送一帧（Register/Ack）。线程安全。
    /// \return false = 未连接或发送失败
    bool send(const ms_proto::Envelope& env);

private:
    void recv_loop();

    Poco::Net::StreamSocket socket_;
    std::mutex socket_mutex_;      // 保护 socket_ 的并发 send/close
    std::atomic<bool> running_{false};
    std::thread  recv_thread_;
    std::string  recv_buf_;        // 仅 recv 线程访问，无需加锁

    EnvelopeHandler on_envelope_;
    StateHandler    on_connected_;
    StateHandler    on_disconnected_;
};

} // namespace ms
