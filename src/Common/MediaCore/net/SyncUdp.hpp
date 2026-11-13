// ============================================================================
//  SyncUdp.hpp —— UDP 心跳传输（§6：高频同步帧走极简二进制）
//
//  一个类服务两端（角色由使用方式决定，本身无状态偏置）：
//    * Editor 侧：bind(固定心跳端口) → 收 Player 上报帧（on_frame 带来源
//      地址）→ send_to(心跳帧, 来源地址) 逐帧回应 + 定时向已注册 Player
//      的 UDP 地址单播（地址来自 MsgRegister.udp_report_port）；
//    * Player 侧：bind(0) 随机端口 → set_remote(编辑器 IP:心跳端口) →
//      周期 send(上报帧) / on_frame 收 Editor 心跳（学习 session_start、
//      软重锚）。
//
//  线程模型：start 后内部一个接收线程 poll(200ms) 轮询，收到完整 56B 帧
//  解码成功才回调（魔数/版本不符静默丢弃——UDP 上可能收到无关广播）。
//  remote_ 由调用方线程写（set_remote）、心跳定时线程读（send），属于跨
//  线程数据——用 remote_mutex_ 互斥保护（Poco::Net::SocketAddress 含内部
//  动态成员，并发读写是 UB，不能靠"概率上没事"）。sendTo 本身单发原子。
// ============================================================================

#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>

#include "sync/SyncFrame.hpp" // ms::SyncFrame

namespace ms {

class SyncUdp {
public:
    /// 收到一帧合法心跳：frame = 解码结果，from = 发送方地址
    /// （Editor 用 from 回应心跳；Player 用 frame 内容做软对齐）
    using FrameHandler = std::function<void(const SyncFrame& frame,
                                            const Poco::Net::SocketAddress& from)>;

    SyncUdp() = default;
    ~SyncUdp() { stop(); }

    SyncUdp(const SyncUdp&) = delete;
    SyncUdp& operator=(const SyncUdp&) = delete;

    /// 绑定并启动接收线程。
    /// \param bind_port 0 = 系统分配随机端口（Player 侧）；Editor 传固定心跳端口
    /// \return 是否成功
    bool start(std::uint16_t bind_port);

    /// 停止并 join 接收线程（幂等）
    void stop();

    /// 实际绑定端口（start(0) 后查询；未启动返回 0）
    std::uint16_t port() const;

    void set_on_frame(FrameHandler h) { on_frame_ = std::move(h); }

    /// Player 侧：设置默认发送目标（Editor 的心跳地址），随后可用 send()
    void set_remote(const std::string& host, std::uint16_t port);

    /// 发送一帧到默认目标（未 set_remote 则失败）
    bool send(const SyncFrame& frame);

    /// 发送一帧到指定地址（Editor 回应用）
    bool send_to(const SyncFrame& frame, const Poco::Net::SocketAddress& to);

private:
    void recv_loop();

    Poco::Net::DatagramSocket socket_;
    std::atomic<bool> running_{false};
    std::thread recv_thread_;
    mutable std::mutex remote_mutex_;   // 保护 remote_（set_remote 写 / send 读，跨线程）
    Poco::Net::SocketAddress remote_;   // 默认发送目标（Player 用）
    FrameHandler on_frame_;
};

} // namespace ms
