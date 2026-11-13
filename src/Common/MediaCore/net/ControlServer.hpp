// ============================================================================
//  ControlServer.hpp —— TCP 控制通道·服务端（Editor 侧，§6.2）
//
//  设计依据：《media-server-sync-design.md》§6：
//    * TCP 承载低频高可靠消息：注册 / 场景下发 / 重锚命令 / 确认；
//    * 帧格式 = [4 字节长度前缀][Envelope pb]（见 net/FrameCodec.hpp）；
//    * Player 主动连接 Editor，Editor 不主动建链；
//    * 单 Editor 多 Player：每连接一个接收线程（Player 数量个位数，够用；
//      未来 >10 连接再升级 POCO SocketReactor）。
//
//  线程模型：
//    * accept 线程：poll(100ms) 轮询监听 socket → 有连接即 accept，
//      为每个连接起一个 recv 线程（线程对象挂在 Conn 上，stop 时统一 join）；
//      顺带低频收割死连接（alive=false 的条目 join + 删除，防 fd 泄漏）；
//    * recv 线程：poll(200ms) → receiveBytes → FrameCodec::tryExtract
//      循环出帧 → 回调 on_envelope_（在 recv 线程上下文执行，回调方
//      自行保证线程安全）；
//    * stop()：置位 running_ → 关监听 + 关全部连接 → join 所有线程。
//      poll 超时保证 recv 线程最多 200ms 内察觉关闭并退出，无阻塞悬挂。
//
//  回调约定：
//    on_envelope(conn_id, env)   —— 收到完整一帧（Register/Ack）
//    on_disconnected(conn_id)    —— 对端断开或协议错误被踢
//    回调里可调用 find_by_player_id / peer_host_of 等本类方法，
//    内部用 conn_mutex_ 保护，无死锁风险（回调不持锁调用）。
//    player_id 由 recv 线程在收到 Register 时自动回填（无需手动设置）。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/StreamSocket.h>

#include "sync.pb.h" // ms_proto::Envelope

namespace ms {

class ControlServer {
public:
    using EnvelopeHandler    = std::function<void(std::uint64_t conn_id,
                                                  const ms_proto::Envelope&)>;
    using DisconnectHandler  = std::function<void(std::uint64_t conn_id)>;
    /// 新连接建立通知（accept 成功后、首帧到达前触发；第五迭代：
    /// 显示端监听模式下借此向接入的 Editor 上报身份 Register）
    using ConnectHandler     = std::function<void(std::uint64_t conn_id)>;

    ControlServer() = default;
    ~ControlServer() { stop(); } // RAII：忘调 stop 也安全

    ControlServer(const ControlServer&) = delete;
    ControlServer& operator=(const ControlServer&) = delete;

    /// 启动监听。port=0 时由系统分配临时端口（单元测试用），真实部署传固定端口。
    /// \param exclusive_bind true = 独占语义（display 监听模式用）：绑定前
    ///        先对本机端口做 TCP 连接探测，有活监听者立即返回 false——同机
    ///        多 display 时第二实例走 fallback（系统分配端口），不会落进
    ///        Windows 的"REUSEADDR 双绑假监听"陷阱（两个 socket 都能 bind
    ///        成功 17100，但连接只投递给先绑者）。探测失败（无人监听）则
    ///        照常绑定。false（缺省）= 不探测直接绑（editor 端快速重启
    ///        场景，SO_REUSEADDR 保证不被 TIME_WAIT 卡端口）。
    /// \return 是否成功（端口占用等失败返回 false）
    bool start(std::uint16_t port, bool exclusive_bind = false);

    /// 停止：关监听、断开全部连接、join 线程（幂等，可重复调用）
    void stop();

    /// 实际监听端口（start(port=0) 后可查询系统分配的端口）
    std::uint16_t port() const;

    // ---- 回调注册（须在 start 之前设置）----
    void set_on_envelope(EnvelopeHandler h)  { on_envelope_   = std::move(h); }
    void set_on_disconnected(DisconnectHandler h) { on_disconnected_ = std::move(h); }
    void set_on_connected(ConnectHandler h)  { on_connected_    = std::move(h); }

    // ---- 发送 ----
    /// 向指定连接发送一帧。\return false = 连接不存在或发送失败
    bool send_to(std::uint64_t conn_id, const ms_proto::Envelope& env);

    /// 向所有存活连接广播。\return 成功送达的连接数
    std::size_t broadcast(const ms_proto::Envelope& env);

    /// 踢除指定连接（异步：shutdown+close 令其 recv 线程立即退出，
    /// 随后走既有断连回调 + 收割流程）。
    /// 【线程安全】可在被踢连接自身的 recv 线程上下文调用（配对码校验
    /// 失败的 on_envelope 回调里）——内部不 join 任何线程，无自死锁。
    /// \return false = 连接不存在或已断开
    bool close(std::uint64_t conn_id);

    // ---- 连接管理 / 元数据 ----
    std::size_t connection_count() const;

    /// 按 player_id 查连接（0 = 不存在/未注册）
    std::uint64_t find_by_player_id(const std::string& player_id) const;

    /// 取某连接的 player_id（空串 = 未注册）
    std::string player_id_of(std::uint64_t conn_id) const;

    /// 取某连接的对端 IP（如 "192.168.0.101"；空串 = 连接不存在）。
    /// Editor 据此 + MsgRegister.udp_report_port 拼出 Player 的 UDP 心跳地址。
    std::string peer_host_of(std::uint64_t conn_id) const;

private:
    struct Conn {
        explicit Conn(Poco::Net::StreamSocket s) : socket(std::move(s)) {}

        Poco::Net::StreamSocket socket;  // 本连接的 TCP socket
        Poco::Net::SocketAddress peer_addr; // 对端地址（accept 时记录，只读）
        std::mutex   send_mutex;         // 序列化并发 sendBytes（sendBytes 内部循环发满）
        std::string  recv_buf;           // TCP 流累积缓冲（FrameCodec 原地消费）
        std::string  player_id;          // 注册后回填（recv 线程自动）
        std::thread  recv_thread;        // 本连接的接收线程
        // 【关键】recv 线程退出时【不】从 conns_ 自删：Conn 若在线程退出
        // 的瞬间析构，其 recv_thread 成员（joinable）析构会 std::terminate。
        // 改为打 alive 标记：accept 线程低频收割（reap_dead_conns，
        // join 后删除条目）/ stop() 统一 join 后清理。
        std::atomic<bool> alive{true};    ///< false = recv 线程已退出
        std::atomic<bool> notified{false};///< 断连回调只发一次的护栏
    };
    using ConnPtr = std::shared_ptr<Conn>;

    void accept_loop();
    void recv_loop(std::uint64_t conn_id, const ConnPtr& conn);
    /// 回收 alive=false 的连接（join 其 recv_thread + erase 条目）
    void reap_dead_conns();

    Poco::Net::ServerSocket acceptor_;
    std::atomic<bool> running_{false};
    std::thread       accept_thread_;

    mutable std::mutex conn_mutex_;
    std::map<std::uint64_t, ConnPtr> conns_;   // conn_id → 连接
    std::uint64_t next_conn_id_ = 1;           // 连接号分配（conn_mutex_ 保护）

    EnvelopeHandler   on_envelope_;
    DisconnectHandler on_disconnected_;
    ConnectHandler    on_connected_;
};

} // namespace ms
