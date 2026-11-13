// ============================================================================
//  Discovery.hpp —— 局域网显示端发现（UDP 广播查询 / 单播应答）
//
//  需求（2026-11-03 用户）：屏幕属性面板"替代手填 player_id"——
//    editor 广播查询 → 局域网内 display 应答（id/主机名/IP/监听端口/状态），
//    编辑器扫描列表点选即完成绑定（id + host + port 三项自动填充）。
//
//  协议设计（风格对齐 SyncFrame：极简定长二进制、零依赖、无握手）：
//    * QUERY（editor → 受限广播 255.255.255.255:17610）：
//        "谁在线？"——帧内仅 type 有效，其余字段填 0；
//    * RESPONSE（display → 查询来源地址单播）：player_id + hostname +
//      TCP 监听端口 + 接入状态。应答发往查询包源地址，天然正确
//      （无需 display 知道 editor 地址）；
//    * 全部定向收发，【无子网风暴】：多 editor 并存互不可见，只有
//      主动查询的 editor 能收集到应答（各查各的，应答各回各的）。
//
//  端口：display 绑定 kDefaultPort(17610) 接收查询。同机多 display 时
//  各实例以 SO_REUSEADDR（POSIX 另加 SO_REUSEPORT）共享绑定同端口——
//  Windows 语义下【广播】查询投递给所有同端口实例，每个实例各自应答，
//  editor 扫描列表能看到同机全部 display（按 id / TCP 端口区分）。
//
//  线程模型（同 SyncUdp）：start 后内部一个接收线程 poll 轮询；回调在
//  接收线程触发，UI 侧用定时器拉取 results() 快照（不做跨线程 UI 直调）。
//
//  部署注意：Windows 防火墙可能拦截 UDP 入站（display 首次运行弹授权）；
//  企业网若禁广播，可用 scan(target) 定向单播查询（如网段扫描）。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>

namespace ms {

// ---------------------------------------------------------------------------
// 发现帧（收发两侧的结构化视图 + 线格式编解码）
// ---------------------------------------------------------------------------
struct DiscoveryFrame {
    enum class Type : std::uint8_t {
        Query    = 1, ///< editor → 广播：谁在线？
        Response = 2, ///< display → 查询源：我在线（附身份/端口/状态）
    };
    enum class DevState : std::uint8_t {
        Unknown = 0, ///< 直连模式等无法判定的场景
        Idle    = 1, ///< 监听中，未被任何 editor 接入
        Bound   = 2, ///< 已被某 editor 接入（TCP 层有连接）
    };
    /// display 运行模式（M5 编辑器互通 2026-11-11）：editor 扫描列表据此
    /// 显示徽标，提示该端可否导入/导出 .show 工程（仅 standalone 开放）。
    /// 落位线格式原 pad 字节（offset 71）——v1 老程序恒写 0 且读取忽略，
    /// 新程序读老帧得 Unknown，版本保持 1 零破坏互操作。
    enum class PlayerMode : std::uint8_t {
        Unknown    = 0, ///< 旧版 display（未上报模式）
        Standalone = 1, ///< 独立模式（网页编排台；.show 导入导出开放）
        Sync       = 2, ///< 同步受控模式（editor TCP 接管）
    };
    Type          type = Type::Query;
    std::string   player_id;      ///< display 标识（RESPONSE 有效；≤31 字节）
    std::string   hostname;       ///< display 主机名（人工辨认；≤31 字节）
    std::uint16_t listen_port = 0; ///< display TCP 监听端口（0 = 直连模式无监听）
    DevState      state = DevState::Unknown; ///< 接入状态（RESPONSE 有效）
    PlayerMode    mode  = PlayerMode::Unknown; ///< 运行模式（RESPONSE 有效）
};

class DiscoveryCodec {
public:
    static constexpr std::uint16_t kMagic       = 0x4D44; // 'MD'
    static constexpr std::uint8_t  kVersion     = 1;
    /// 线格式定长字节数（static_assert 锁定布局）
    static constexpr std::size_t   kWireSize    = 72;
    /// display 侧发现服务端口（editor 广播目标；ini/命令行可覆盖）
    static constexpr std::uint16_t kDefaultPort = 17610;

    /// 逻辑帧 → 线格式（写入 out，返回恒为 kWireSize；超长字符串截断）
    static std::size_t encode(const DiscoveryFrame& frame, void* out) noexcept;

    /// 线格式 → 逻辑帧
    /// \return nullopt = 长度/魔数/版本不符（静默丢弃，UDP 语义）
    static std::optional<DiscoveryFrame> decode(const void* data,
                                                std::size_t len) noexcept;
};

// ---------------------------------------------------------------------------
// 本机主机名（RESPONSE 填充用；gethostname 包装，失败返回空串）
// ---------------------------------------------------------------------------
std::string localHostName();

// ---------------------------------------------------------------------------
// DiscoveryResponder —— display 侧应答器
//
// 绑定发现端口收 QUERY → 立即回 RESPONSE（id + 主机名 + TCP 监听端口 +
// 接入状态）。接入状态由调用方注入谓词（监听模式 = editor_conn_ 非零）；
// 直连模式（display 主动连 editor）传 nullptr → state=Unknown。
// ---------------------------------------------------------------------------
class DiscoveryResponder {
public:
    /// 是否已被 editor 接入（TCP 层谓词；接收线程上下文调用，须线程安全）
    using BoundFn = std::function<bool()>;

    DiscoveryResponder() = default;
    ~DiscoveryResponder() { stop(); }
    DiscoveryResponder(const DiscoveryResponder&) = delete;
    DiscoveryResponder& operator=(const DiscoveryResponder&) = delete;

    /// 启动应答（幂等）。
    /// \param player_id       应答身份（display 的 --id）
    /// \param tcp_listen_port 本端 TCP 监听端口（0 = 直连模式无监听）
    /// \param bound_fn        接入状态谓词（空 = state 恒 Unknown）
    /// \param bind_port       发现端口（默认 17610；绑定失败 → 返回 false，
    ///                        调用方日志提示后继续主流程，不参与发现。
    ///                        同机多实例经 SO_REUSEADDR/SO_REUSEPORT
    ///                        共享同端口，各实例都能被扫描到）
    /// \param mode            运行模式（M5：应答帧自报，editor 列表徽标用）
    bool start(const std::string& player_id, std::uint16_t tcp_listen_port,
               BoundFn bound_fn,
               std::uint16_t bind_port = DiscoveryCodec::kDefaultPort,
               DiscoveryFrame::PlayerMode mode =
                   DiscoveryFrame::PlayerMode::Unknown);

    /// 停止并 join 接收线程（幂等）
    void stop();

    /// 实际绑定端口（start(bind_port=0) 后查询；未启动返回 0）。
    /// 测试用：随机端口起应答器 + 浏览器定向查询。
    std::uint16_t port() const;

private:
    void recv_loop();

    Poco::Net::DatagramSocket socket_;
    std::atomic<bool> running_{false};
    std::thread       recv_thread_;
    std::string       player_id_;        ///< 应答身份（start 固定）
    std::string       hostname_;         ///< 本机主机名（start 固定）
    std::uint16_t     tcp_listen_port_ = 0;
    DiscoveryFrame::PlayerMode mode_ = DiscoveryFrame::PlayerMode::Unknown;
    BoundFn           bound_fn_;         ///< 接入状态谓词（可空）
    std::mutex        mutex_;            ///< 保护以上可变成员
};

// ---------------------------------------------------------------------------
// DiscoveryBrowser —— editor 侧浏览器
//
// scan() 广播查询（连发 2 帧防丢包）→ 收集 RESPONSE → 按 player_id 去重
// 记录（host 取 UDP 源地址，port 取帧内 listen_port）。results() 快照供
// UI 定时拉取；8s 未再应答的条目视为离线剔除。
// ---------------------------------------------------------------------------
class DiscoveryBrowser {
public:
    struct Device {
        std::string   player_id;   ///< display 标识（绑定用）
        std::string   hostname;    ///< 主机名（列表展示）
        std::string   host;        ///< 应答来源 IP（登记用）
        std::uint16_t listen_port = 0; ///< TCP 监听端口（0 = 直连模式）
        bool          bound = false;   ///< 已被某 editor 接入
        /// 运行模式徽标（M5）：Unknown = 旧版 display 未上报
        DiscoveryFrame::PlayerMode mode = DiscoveryFrame::PlayerMode::Unknown;
    };

    DiscoveryBrowser() = default;
    ~DiscoveryBrowser() { stop(); }
    DiscoveryBrowser(const DiscoveryBrowser&) = delete;
    DiscoveryBrowser& operator=(const DiscoveryBrowser&) = delete;

    /// 启动（绑定随机端口 + 开接收线程；幂等）
    /// \param discovery_port display 侧发现端口（查询目标端口）
    bool start(std::uint16_t discovery_port = DiscoveryCodec::kDefaultPort);
    void stop();

    /// 发送一轮查询。默认受限广播 255.255.255.255（不跨路由器）；
    /// \param target 可定向单播（测试用 127.0.0.1 / 跨网段网关地址）
    void scan(const std::string& target = "255.255.255.255");

    /// 当前在线设备快照（去重 + 离线剔除；任意线程可调）
    std::vector<Device> results() const;

private:
    void recv_loop();

    Poco::Net::DatagramSocket socket_;
    std::atomic<bool> running_{false};
    std::thread       recv_thread_;
    std::uint16_t     discovery_port_ = 0; ///< 查询目标端口
    struct Rec {
        Device      dev;
        std::int64_t last_seen_ms = 0;   ///< 最近应答时刻（steady，ms）
    };
    std::map<std::string, Rec> found_;   ///< player_id → 最近应答（mutex_ 保护）
    mutable std::mutex         mutex_;
};

} // namespace ms
