// ============================================================================
//  DisplayConnector.hpp —— Editor 侧显示端连接器（第五迭代，WATCHOUT 模型）
//
//  角色反转（第五迭代需求）：
//    旧：显示端（Player）主动连接 Editor 的 ControlServer；
//    新：显示端启动后自动监听端口零操作等待；Editor 按项目登记列表
//        （ShowDocument.displays）逐个【主动连接】并同步数据——
//        连接建立 → 显示端上报 Register → Editor 回 Ack + 下发场景 →
//        此后播放命令/补丁经 broadcast 同步到全部在线显示端。
//
//  结构：每个 display 一个 ControlClient（对称复用既有 TCP 客户端），
//  外加一个看门狗线程（周期巡检 enabled 且断线的条目 → 指数退避重连，
//  与 PlayerRuntime::watchdogLoop 同策略）。消息集（Envelope）完全复用，
//  协议零改动——只换连接方向。
//
//  线程模型：
//    * 看门狗线程：巡检 + 阻塞 connect（3s 超时）→ on_connected 回调
//      （看门狗线程上下文）；
//    * ControlClient recv 线程：入站 Envelope → on_envelope 回调；
//    * 调用方线程：add/remove/sendTo/broadcast/snapshot。
//    内部一把 mutex_ 保护 slots_；回调均在锁外触发（回调方可安全调
//    sendTo 等，无死锁；但【禁止】在回调里 remove —— recv 线程回调中
//    stop 自身会 join 自己，见 remove 注释）。
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
#include <vector>

#include "net/ControlClient.hpp"
#include "sync.pb.h" // ms_proto::Envelope

namespace ms {

class DisplayConnector {
public:
    /// 显示端条目：登记信息（.show 持久化）+ 运行状态（快照观测）
    struct Entry {
        std::string   display_id;         ///< 显示端槽位标识（= 工程绑定
                                          ///<  的 player_id；Editor 在
                                          ///<  Ack 中把它指派给显示端，
                                          ///<  显示端采用后以该 id 注册）
        std::string   host;               ///< 显示端主机
        std::uint16_t port = 17100;       ///< 显示端监听端口
        bool          enabled   = true;   ///< false = 不连接/不同步
        /// 配对码（2026-11-03 多 Editor 隔离）：非空 = 连接建立后立即发送
        /// MsgAuth 配对帧，display 校验通过才处理本 Editor 的业务帧。
        /// 空 = 不认证（display 端未配码时此值可空）。
        std::string   pairing_code;
        // ---- 运行状态（快照时填充）----
        bool        connected  = false;   ///< TCP 在线
        /// 已注册（2026-11-06 指派模型）：本连接收到过注册帧。
        /// TCP 连上≠身份就绪——注册帧才宣告真实身份；未注册期间
        /// display 尚未完成指派 id 交换（可能正准备换 id 重注册）。
        bool        registered = false;
        std::string local_host;           ///< Editor 侧本端 IP（Ack.udp_host 用）
        std::string peer_host;            ///< 显示端 IP（心跳单播目标）
        long        backoff_ms = 0;       ///< 当前重连退避（诊断）
    };

    /// 入站消息（recv 线程上下文；display_id 由连接器补全）
    using EnvelopeHandler =
        std::function<void(const std::string& display_id,
                           const ms_proto::Envelope&)>;
    /// 连接状态变化（connected=true 看门狗线程 / false recv 线程）
    using StateHandler =
        std::function<void(const std::string& display_id, bool now_connected)>;

    DisplayConnector() = default;
    ~DisplayConnector() { stop(); }
    DisplayConnector(const DisplayConnector&) = delete;
    DisplayConnector& operator=(const DisplayConnector&) = delete;

    // ---- 回调（add 之前设置）-----------------------------------------------
    void set_on_envelope(EnvelopeHandler h) { on_envelope_ = std::move(h); }
    void set_on_state(StateHandler h)       { on_state_     = std::move(h); }

    // ---- 生命周期 -----------------------------------------------------------

    /// 启动重连看门狗（EditorRuntime::start 调用；幂等）
    void start();
    /// 停止：断开全部连接 + join 看门狗（幂等）
    void stop();

    // ---- 显示端登记管理 ------------------------------------------------------

    /// 登记并连接一个显示端。【异步】连接由看门狗在 ≤巡检周期内发起
    /// （UI 线程调用不卡顿）；失败也保持登记，看门狗按退避持续重试。
    /// \param pairing_code 配对码（空 = 不认证；非空 = 连接后先发 MsgAuth）
    /// \return false = 参数非法（id/host 空）或 id 重复
    bool add(const std::string& display_id, const std::string& host,
             std::uint16_t port, const std::string& pairing_code = {});

    /// 注销并断开。【注意】不得在 on_envelope/on_state 回调里调用
    /// （recv/看门狗线程上下文中 stop 自身连接会 join 自死锁）——
    /// 删除显示端请走 UI 线程（用户操作路径天然满足）。
    bool remove(const std::string& display_id);

    /// 启停：停用立即断开，启用交看门狗重连
    bool setEnabled(const std::string& display_id, bool enabled);

    // ---- 发送 / 查询 ---------------------------------------------------------

    /// 向指定显示端发送一帧。\return false = 未登记/未连接/发送失败
    bool sendTo(const std::string& display_id, const ms_proto::Envelope& env);

    /// 广播到全部 enabled 且 connected 的显示端。\return 送达数
    std::size_t broadcast(const ms_proto::Envelope& env);

    /// 全部条目快照（含运行状态；UI/状态栏遍历用）
    std::vector<Entry> snapshot() const;

    /// 在线条数（connected && enabled）
    std::size_t connected_count() const;

    /// display 的 IP（TCP 对端；UDP 心跳单播目标主机部分）。
    /// \return 空串 = 未登记或未连接
    std::string peerHostOf(const std::string& display_id) const;

    /// Editor 侧本端 IP（连接已建立时；Ack.udp_host 用，空串 = 未知）
    std::string localHostOf(const std::string& display_id) const;

    /// 【注册确认 2026-11-06 指派模型】注册帧到达时置位。TCP 连接只能
    /// 证明"端口上有个进程"，注册帧才宣告身份（且 Editor 会在 Ack 中
    /// 下发指派 id；display 采用后以指派 id 重新注册）。断连/重连自动
    /// 复位（重连后等新注册帧再置位）。
    void setRegistered(const std::string& display_id, bool registered);

private:
    struct Slot {
        Entry info;                          // 登记 + 连接后回填的地址
        bool  registered = false;            // 已收到注册帧
        // shared_ptr 所有权：看门狗线程锁内拷贝后锁外阻塞 connect（最长
        // 3s）——期间 UI 线程 remove/setEnabled 销毁槽位不会析构 client
        // （裸 unique_ptr 在此窗口构成 use-after-free）
        std::shared_ptr<ControlClient> client;
        long backoff_ms = 1000;              // 重连退避（失败×2 封顶 5s，成功归位）
        long long next_attempt_ms = 0;       // 下次可尝试连接的时刻（steady ms）
    };

    /// 看门狗主循环：巡检 → 锁外阻塞 connect → 状态回调
    void watchdogLoop();
    /// ControlClient 回调包装（补全 display_id 后转发上层）
    void forwardEnvelope(const std::string& display_id,
                         const ms_proto::Envelope& env);
    void forwardDisconnected(const std::string& display_id);
    /// 创建并装配好回调的 ControlClient（add 首建与看门狗重建共用；
    /// setEnabled(false) 会把槽位 client move 走置空——重新启用后由
    /// 看门狗经此函数重建，否则该显示端永远无法再连（2026-11-05 修复）
    std::shared_ptr<ControlClient> makeClient(const std::string& display_id);

    std::map<std::string, Slot> slots_;  ///< display_id → 槽位（mutex_ 保护）
    mutable std::mutex mutex_;
    EnvelopeHandler on_envelope_;
    StateHandler    on_state_;
    std::atomic<bool> running_{false};
    std::thread     watchdog_;
};

} // namespace ms
