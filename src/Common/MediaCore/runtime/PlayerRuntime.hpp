// ============================================================================
//  PlayerRuntime.hpp —— Player 侧运行时装配层（实现计划 P2，§5）
//
//  设计依据：《media-server-sync-design.md》§5.2（Player）§6.1（anchor_time）
//    把已有 Core 组件装配成"可被网络驱动"的 Player 行为闭环：
//
//      TCP 注册 ──► MsgRegisterAck ──► MsgScene ──► SceneLoader::parse
//                                                          │
//      MsgReAnchor ──► AnchorScheduler（到点执行）◄────────┘
//                          │ poll(aligned_now)
//                          ▼
//      session_start = anchor_time - anchored_session_time
//      current      += dt × rate            （本地播放头状态机）
//      delta = target − current ──► DriftController（§7.1 纠偏闭环）
//                          │
//      250ms 节流 ──► SyncFrame UDP 上报（Editor 集中观测）
//
//  【时间注入设计】本类不持有时钟：tick(now_us) 由外部主循环以对齐时钟
//  驱动（main 用 ClockProvider；单元测试用手工时间序列）——同一份逻辑
//  既可真实运行又可确定性测试（P1 "纯函数核心 + 薄壳 IO" 原则的延续）。
//
//  线程模型（四个线程上下文共享状态，用一把 state_mutex_ 串行化）：
//    * TCP recv 线程：Ack/Scene/ReAnchor 回调 → 写 session_id / scene /
//      scheduler（post 需入锁）；
//    * UDP recv 线程：Editor 心跳 → Idle 时学 session_start（迟到加入
//      软对齐，§6"迟到加入学锚点"）；
//    * 调用方线程：tick() 推进 + 上报。
//    * 渲染线程（§5.6，可选）：每帧经 sceneSnapshot/playheadSnapshot 快照
//      取输入——永不直接触碰可变状态，与上述线程零数据竞争。
//    回调里只做"记录 + 入队"，重活（parse 场景）锁外完成后锁内存入。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "clock/ClockProvider.hpp"
#include "engine/TimelineEngine.hpp"
#include "loader/SceneLoader.hpp"
#include "model/SceneModel.hpp"
// （net/ControlClient.hpp 不再需要：直连 Editor 的旧模式已退役，2026-11-03；
//   Editor 侧主动连接显示端复用 ControlClient，但那是 DisplayConnector 的
//   内部依赖，本头文件无感知）
#include "net/ControlServer.hpp"
#include "net/Discovery.hpp"
#include "net/SyncUdp.hpp"
#include "runtime/RenderThread.hpp"
#include "sync/AnchorScheduler.hpp"
#include "sync/DriftController.hpp"
#include "sync/SyncFrame.hpp"

namespace ms {

class PlayerRuntime {
public:
    // -----------------------------------------------------------------------
    // 素材拉取进度（"同步媒体"的可视化数据；媒体文件同步独立职责的配套）
    // -----------------------------------------------------------------------
    /// 单条进度快照（拉取线程构造，回调订阅方自行加锁节流/上屏）。
    /// 【定义先于 Config】Config::asset_progress_fn 的签名引用本类型。
    struct AssetProgress {
        std::size_t   done          = 0;  ///< 已处理完的素材数（成功/幂等/失败都计）
        std::size_t   total         = 0;  ///< 本机相关素材总数（含就绪跳过项；2026-11-09 与全跳过路径口径统一）
        std::string   current_path;       ///< 当前正在处理的素材（Asset.path）
        std::uint64_t current_bytes = 0;  ///< 当前文件已传字节（含续传已有）
        std::uint64_t current_total = 0;  ///< 当前文件总字节（0 = 服务端未知）
        bool          finished      = false; ///< true = 本轮全部处理完（UI 收起）
    };

    // -----------------------------------------------------------------------
    // 启动配置（与 app/main.cpp 的命令行参数一一对应）
    // -----------------------------------------------------------------------
    struct Config {
        /// --id：本地标签（发现应答自报/预览标题）；连接 Editor 后采用
        /// 其指派的槽位 id（RegisterAck.assigned_player_id），渲染选屏、
        /// 注册帧与场景过滤均以指派 id 为准——同一块屏接错 --id 也能自愈。
        std::string player_id;
        std::string version    = "0.2";    ///< 引擎版本（注册上报）
        /// 素材落盘根目录（空 = 不拉取素材）。文件写到
        /// assets_root + "/" + Asset.path（与 Editor 端目录结构一致）
        std::string assets_root;
        /// 素材并发拉取 worker 数（--asset-workers 可改；对照 WATCHOUT
        /// Runner 的 2 并发 worker，默认 3 略宽裕）。1 = 串行（旧行为）；
        /// <1 视为 1。多 worker 共享待拉清单原子游标竞争取任务，进度
        /// done 跨 worker 聚合；current_* 字段在多 worker 并行时为最近
        /// 写入者的观测近似（观测帧，丢失无害）
        int asset_workers = 3;
        /// UDP 上报节流周期（us）：P2 MVP 用 250ms（§6 心跳 ~1Hz 的加密版，
        /// 联测可观测性更好）；诊断打点密度由实现计划"同步诊断日志"消化
        std::int64_t report_interval_us = 250'000;
        /// 自身状态诊断日志周期（us）：tick 内 Playing 时按此节流打印
        /// 播放头/delta/rate（实现计划 P2"同步诊断日志"，后续做成面板）
        std::int64_t diag_interval_us = 5'000'000;

        // ---- 监听模式（WATCHOUT 模型，2026-11-03 起为唯一模式）--------
        /// 【旧直连模式（ControlClient 主动连 editor_host:editor_port）
        /// 已退役】——display 入口硬编码监听等待，Editor 按项目登记主动
        /// 接入；本端不配置任何 Editor 地址，连接方向单向（Editor→display）。
        /// 断线重连责任在 Editor 侧（DisplayConnector 指数退避），本端零操作。
        std::uint16_t listen_port = 17100; ///< 监听端口（--port 可改；被占自动 fallback）

        // ---- 配对码（2026-11-03：多 Editor 隔离，防伪装/抢占）------------
        /// 非空 = 启用配对认证：Editor 连接后首帧必须是 MsgAuth 且
        /// pairing_code 与本值一致，否则该连接被踢除（场景等业务帧在认证
        /// 通过前一律静默丢弃）。空（缺省）= 不校验，任何 Editor 都可接入
        /// （向后兼容）。Editor 侧同码配置在显示端登记（.show DisplayEntry）。
        std::string pairing_code;

        // ---- 局域网发现应答（2026-11-03：editor 扫描添加显示端）--------
        /// 绑定此端口应答 editor 的 UDP 发现查询（ DiscoveryCodec::
        /// kDefaultPort=17610）；0 = 禁用发现（不参与局域网扫描）。
        /// 同机多 display 各实例经 SO_REUSEADDR 共享同端口，都被扫描到。
        std::uint16_t discovery_port = DiscoveryCodec::kDefaultPort;
        /// 发现应答自报运行模式（M5 编辑器互通）：0=Unknown 1=standalone
        /// 2=sync——editor 扫描列表显示徽标，提示 .show 导入导出可用性。
        std::uint8_t discovery_mode = 0;

        // ---- 渲染线程（§5.6；对应 --render-fps / --render-out 参数）----
        /// 目标帧率：>0 启用渲染线程（60fps present 相位对齐）；
        /// 0 = 不渲染（纯同步联测/低配部署）
        int render_fps = 0;
        int render_w = 0, render_h = 0; ///< 输出分辨率（0 = 首个绑定 screen 1:1）
        /// 诊断帧导出目录（空 = 不导出；联测用 BMP 核对各端画面内容）
        std::string render_capture_dir;
        /// 导帧间隔（us，默认 1s 一帧）与上限（防写爆磁盘）
        std::int64_t render_capture_interval_us = 1'000'000;
        int render_capture_max = 12;
        /// 对齐时钟覆盖（渲染线程用；空 = 内部 ClockProvider）。
        /// 单测注入手工时钟序列，使渲染线程与 tick 驱动时间同域。
        std::function<std::int64_t()> clock_fn;
        /// 帧输出回调（透传 RenderThread::Config.frame_sink）：显示端预览
        /// 窗口每帧收 RGBA 画面（media_display 装配；其余端留空不回调）
        std::function<void(int, int, const std::uint8_t*, std::uint32_t)>
            render_frame_sink;
        /// 合成后端工厂（透传 RenderThread::Config.backend_factory；第②
        /// 阶段 GL 管线注入点——display 端装配 GlRenderer，空 = 软渲染基线）
        std::function<std::unique_ptr<IComposeRenderer>()>
            render_backend_factory;
        /// 硬解后端配置（透传 RenderThread::Config.hw_accel → FFmpeg 媒体
        /// 源；探测失败自动软解兜底）。display 装配层默认 D3D11VA。
        HwAccelConfig hw_accel;
        /// 素材拉取进度回调（"同步媒体"可视化；拉取线程上下文同步调用
        /// ——订阅方自行保证线程安全。留空 = 不上报，行为与旧版一致）
        std::function<void(const AssetProgress&)> asset_progress_fn;

        // ---- 音频输出（P3，2026-11-10）----------------------------------
        /// true = display 渲染拍接真实音频输出（与画面同一 session_t 解算
        /// ——A/V 同源同步；需 render_fps>0，音频拍挂在渲染拍上）
        bool audio_enable = false;
        /// 音频设备名（空 = 系统默认播放设备；FriendlyName 包含匹配）
        std::string audio_device;

        // ---- standalone 本机模式（display×MMC 合并方案 v2.2，2026-11-11）--
        /// true = 单机播放节点：start() 跳过网络三件套（TCP 监听 / UDP
        /// 上报 / 局域网发现），场景与播放控制经本机方法组
        /// （applyLocalScene / localPlay / localPause / localSeek /
        /// localStop）由 NodeController 注入——内部复用同一锚点状态机
        /// （applyAnchorLocked），纠偏环在本模式下 delta≈0 无害保留。
        /// false（缺省）= 网络受控模式，行为与旧版完全一致（sync 模式）。
        bool standalone = false;
    };

    explicit PlayerRuntime(Config cfg);

    ~PlayerRuntime() { stop(); }
    PlayerRuntime(const PlayerRuntime&) = delete;
    PlayerRuntime& operator=(const PlayerRuntime&) = delete;

    // ---- 生命周期 ----------------------------------------------------------

    /// 启动：绑定 UDP（先于注册，端口进 MsgRegister）→ 监听 TCP 等
    /// Editor 接入 → 挂网络回调（连接建立即上报 MsgRegister）。
    /// \return false = UDP 绑定失败或监听失败（详情打日志）
    bool start();

    /// 停止：关监听、断 TCP、关 UDP、join 线程（幂等）
    void stop();

    /// 实际监听端口（listen_port 被占 fallback 后与配置不同；测试登记/
    /// 诊断用。0 = 未 start）
    std::uint16_t listenPort() const { return listener_.port(); }

    /// 已收到 RegisterAck 且 TCP 在线
    bool registered() const { return registered_.load(); }

    /// 当前生效的 player id（= cfg_.player_id 或 Editor 指派 id；
    /// 指派模型 2026-11-06：注册帧/渲染选屏/窗口标题均以本值为准）
    std::string effectiveId() const;

    /// 素材是否全部就绪（无素材/未启用 = 恒 true；诊断/联测断言用）
    bool assets_ready() const { return assets_ready_.load(); }
    /// 已就绪素材数 / 总数（诊断）
    std::size_t assets_ready_count() const { return assets_ready_cnt_.load(); }
    std::size_t assets_total() const { return assets_total_.load(); }

    // ---- 主循环驱动（外部每帧调用）-----------------------------------------

    /// 一次节拍：应用到点锚点 → 推进播放头 → 纠偏 → 按需 UDP 上报。
    /// \param aligned_now_us 对齐时钟当前值（us）
    void tick(std::int64_t aligned_now_us);

    // ---- 状态观测（CLI status / 单元测试断言）------------------------------

    PlayState     state() const;                 ///< 播放状态
    std::int64_t  session_time_us() const;       ///< 本地播放头（current）
    double        rate() const;                  ///< 当前纠偏速率
    std::uint32_t frame_index() const;           ///< 推算帧号（current/16.7ms）
    std::string   session_id() const;            ///< Editor 分配的会话
    bool          has_scene() const;             ///< 场景是否已就绪
    std::int64_t  last_delta_us() const;         ///< 最近一次纠偏 delta（诊断）

    // ---- 本机控制（standalone 模式；NodeController 调用，2026-11-11）------
    ///
    /// 【设计：零新状态机】四个控制动作全部构造"立即锚点"
    /// （anchor_time = renderClockNow()）锁内直接走 applyAnchorLocked
    /// ——与 Editor 下发的 ReAnchor 命令共用同一状态迁移代码，本机
    /// 与受控两种模式的播放头语义天然一致。锚点不经 scheduler_ 队列
    /// （不入队 → tick 的 poll 不会取到 → 不回发 Ack，无 Editor 可发）。
    ///
    /// 线程模型：任意线程可调（HTTP 工作线程 / 主线程），内部拿
    /// state_mutex_；tick() 仍由主循环驱动（时间注入原则不变）。

    /// 注入本机场景（不等 Editor 下发）：场景指针整体替换 + 序号自增
    /// （渲染线程据 sceneSerial() 判新 → 重建媒体源）。播放头/状态不动。
    void applyLocalScene(std::shared_ptr<const Scene> s);

    /// 从 from_us 起播（us；立即锚点 PLAY：session_start = now - from_us）
    void localPlay(std::int64_t from_us);

    /// 暂停：冻结于当前播放头
    void localPause();

    /// 重定位到 to_us（状态保持：Playing 中跳转续播 / Paused 中跳转冻结
    /// ——与受控模式 SEEK 同语义）
    void localSeek(std::int64_t to_us);

    /// 停止：播放头归零、状态回 Idle（场景保留——NodeController 换场景
    /// 用 applyLocalScene，本方法只动播放头）
    void localStop();

    /// 本机模式旗标（NodeController 装配校验 / 诊断）
    bool isStandalone() const { return cfg_.standalone; }

    // ---- 渲染线程输入快照（每帧被渲染线程调用；内部加锁拷贝）--------------

    /// 当前场景快照（shared_ptr 原子拷贝；nullptr = 尚未收到场景）
    std::shared_ptr<const Scene> sceneSnapshot() const;

    /// 场景序号（每应用一帧 MsgScene 自增；0 = 从未收到）
    ///
    /// 【为什么需要它 2026-11-05】渲染线程原先用 `scene.get()` 指针是否变化
    /// 判断"这是不是一个新场景"，但上一帧的 shared_ptr 出作用域后内存被释放，
    /// 下一帧 make_shared 很可能**复用同一块地址** → 指针相等 → 误判"场景
    /// 没变"：轻则上屏探针漏计（统计偏高），重则静帧 500ms 节流重新生效
    /// （停着编辑时 display 又变成半秒才动）。序号单调递增，杜绝地址复用歧义。
    std::uint64_t sceneSerial() const;

    /// 播放头快照（状态 + 会话起点 + 本地播放头三元组，一次锁全拿）
    RenderThread::Playhead playheadSnapshot() const;

    /// 渲染线程的对齐时钟读数（clock_fn 覆盖优先；测试注入同域时钟）
    std::int64_t renderClockNow() const;

    /// 累计渲染帧数（联测断言渲染线程活着；未启用渲染恒 0）
    std::uint64_t rendered_frames() const {
        return render_ ? render_->frames() : 0;
    }

    /// 渲染线程访问（M4 MPCDI 几何投递用；render_fps<=0 / 未启动 = nullptr）
    RenderThread* renderThread() noexcept { return render_.get(); }

    /// 累计重连次数（诊断/联测断言）
    std::uint32_t reconnect_count() const;

private:
    // ---- 网络回调（recv 线程上下文）----------------------------------------
    /// TCP 消息分发。\param conn_id 帧来源连接号（配对认证按连接粒度判定）
    void onEnvelope(const ms_proto::Envelope& env, std::uint64_t conn_id);
    void onUdpFrame(const SyncFrame& f,
                    const Poco::Net::SocketAddress& from); // Editor 心跳

    /// 发送注册帧（连接建立时经 on_connected 触发；§6.2）
    bool sendRegister();

    // ---- 锚点应用（§6.1 核心）----------------------------------------------
    /// 在锁内调用：把到点锚点落成 session_start / current / state
    void applyAnchorLocked(const Anchor& a, std::int64_t aligned_now_us);

    /// 渲染线程装配（§5.6）：cfg_.render_fps>0 时创建 RenderThread 并启动。
    /// 【从 start() 提取 2026-11-11】standalone 与受控两种模式的网络装配
    /// 分叉，渲染装配共用——提取成函数消除分支重复。
    void startRenderThread();
    /// 局域网发现应答装配（M5 两模式共用）：startDiscovery(实际 TCP 端口)。
    /// standalone 分支传 0（无 TCP 监听）——discovery_port=0 禁用发现；
    /// 绑定失败仅告警不阻断（控制通道不受影响）。mode 自报经
    /// cfg_.discovery_mode 透传（editor 扫描列表徽标）。
    void startDiscovery(std::uint16_t tcp_port);

    // ---- 素材拉取（后台线程；§5.5）------------------------------------------
    /// 收到场景后启动：按 Asset 表逐个从 Editor HTTP 拉取（幂等/续传/CRC）。
    /// 阻塞 IO 全部在独立线程——控制消息接收与 tick 循环零阻塞。
    /// \param http_host Editor 的 IP（注册 Ack 学到的 udp_host）
    /// \param http_port Editor 的素材 HTTP 分发端口（Ack 学到）
    void fetchAssetsAsync(const std::string& http_host, std::uint16_t http_port);

    // ---- 发送 ---------------------------------------------------------------
    void sendReport(std::int64_t aligned_now_us); // UDP 上报（锁外调用）
    void sendAck(std::int64_t anchor_time_us, bool ok, const char* detail);

    // ---- 成员 ---------------------------------------------------------------
    Config cfg_;

    // ---- 网络传输（顺序：先声明 stop 依赖者后析构——成员逆序析构保证 UDP/TCP
    // 先于状态销毁）--------------------------------------------------------------
    ControlServer listener_;              ///< TCP 服务端（监听等 Editor 接入）
    std::atomic<std::uint64_t> editor_conn_{0}; ///< 已接入的 Editor 连接号（0=无）
    /// 已通过配对认证的连接号（0=无）。仅 cfg_.pairing_code 非空时有意义：
    /// 认证通过后业务帧（Ack/Scene/ReAnchor）只接受来自该连接者。
    std::atomic<std::uint64_t> authed_conn_{0};
    /// 认证前业务帧缓存（conn_id → 帧，FIFO；pending_mutex_ 保护）：
    /// MsgAuth 与 Ack/Scene 到达顺序不保证（Editor 两线程并发写），认证
    /// 通过后按序回放。上限 32 帧（FIFO 淘汰），断开/踢除时按连接清理。
    std::mutex pending_mutex_;
    std::deque<std::pair<std::uint64_t, ms_proto::Envelope>> pending_frames_;
    SyncUdp       udp_;
    /// 局域网发现应答器（editor 扫描"谁在线"时自报身份/端口/接入状态；
    /// 2026-11-03。start 成功后运行，stop 关闭。同机多实例经 SO_REUSEADDR
    /// 共享发现端口，各实例独立应答）
    DiscoveryResponder discovery_;

    // 同步核心（纯内存组件）
    AnchorScheduler  scheduler_;   ///< 到点执行的锚点队列
    std::shared_ptr<const Scene> scene_; ///< 已加载场景（网络接收；shared_ptr
                                          ///  供渲染线程免锁快照）
    /// 场景序号（state_mutex_ 保护；每应用一帧 MsgScene 自增）
    /// 见 sceneSerial() 注释：替代"指针是否变化"的判新语义
    std::uint64_t    scene_serial_ = 0;
    std::thread      asset_thread_; ///< 素材拉取线程（收场景后启动，stop join）
    /// 拉取循环代数（自愈信号，2026-11-04）：每次 fetchAssetsAsync 进入时
    /// 递增——进行中的旧循环在轮间休眠里检测到代数变化即退出（≤200ms），
    /// 让位给携带新清单的新循环（场景替换/新清单推送即时接管，旧循环的
    /// join 无需等待长退避）。
    std::atomic<std::uint32_t> asset_gen_{0};
    /// stop() 置位：拉取线程的自愈轮询看到即退出（join ≤200ms）
    std::atomic<bool> asset_quit_{false};
    /// 【2026-11-03 旧模式退役说明】重连看门狗线程已随直连模式一并移除——
    /// 监听模式下 Editor 是主动方，重连由 Editor 侧 DisplayConnector 负责。

    // ---- 渲染线程（§5.6；start 起线程，stop 先于网络关闭 join）------------
    ClockProvider clock_;                 ///< 对齐时钟（构造记锚点；atomic 成员
                                          ///  不可拷贝，本类按值持有唯一实例）
    std::unique_ptr<RenderThread> render_; ///< 渲染线程（render_fps>0 时创建）

    // ---- 可变状态（state_mutex_ 保护）--------------------------------------
    mutable std::mutex state_mutex_;
    std::string   session_id_;            ///< 当前会话（空 = 未入会话）
    PlayState     state_      = PlayState::Idle;
    std::int64_t  session_start_us_ = 0;  ///< 全场共享的会话起点（对齐时钟域）
    std::int64_t  current_us_       = 0;  ///< 本地播放头（状态机维护）
    std::int64_t  last_tick_us_     = 0;  ///< 上次 tick 的时钟值（算 dt）
    bool          ticking_          = false; ///< 首个 tick 不推进（dt 无从谈起）
    double        rate_             = 1.0;  ///< 纠偏播放速率（DriftController 步进）
    std::int64_t  last_delta_us_    = 0;    ///< 最近一次 target-current（上报）
    std::int64_t  last_report_us_   = 0;    ///< 上次 UDP 上报时刻（节流）
    std::int64_t  last_diag_us_     = 0;    ///< 上次诊断日志时刻（节流）
    PlayState     logged_state_     = PlayState::Idle; ///< 上次"状态变化"日志
    /// 已记录的状态（tick 对比打点；联测观察命令应用时序）
    std::uint64_t reports_sent_     = 0;    ///< 累计上报数（诊断）
    std::uint32_t reconnects_       = 0;    ///< 累计重连次数（诊断；监听模式下
                                            ///  重连由 Editor 侧负责，本端恒 0）
    std::uint16_t editor_http_port_ = 0;    ///< Editor 素材 HTTP 端口（Ack 学到；
                                            ///  0 = 未启用分发）
    std::string   editor_host_learned_;     ///< Editor 的 IP（Ack.udp_host 学到；
                                            ///  素材 HTTP 拉取主机）
    /// 生效 player id（指派模型，2026-11-06）：初值 = cfg_.player_id；
    /// RegisterAck 携带 assigned_player_id 且非空时被指派覆盖（换 id 后
    /// 立即重新注册，Editor 端注册表换键）。state_mutex_ 保护。
    std::string   effective_id_;
    /// 【素材就绪缓存（手动同步去重，2026-11-04）】asset_id → (size, checksum)。
    /// 拉取成功后记录；后续 fetchAssetsAsync 重建循环时命中项（大小与校验
    /// 和均同）直接跳过——不重下载也不重做本地 CRC（旧实现就绪表是拉取
    /// 线程局部变量，每次新清单/新场景都清零重扫，大文件 CRC 数秒一轮，
    /// 用户观感即"反复自动同步"）。state_mutex_ 保护；文件被外部删除由
    /// 过滤时的存在性 stat 检查兜底（不存在/大小不符 → 视为未就绪重拉）。
    /// 【持久化 2026-11-09】跨会话落盘 <assets_root>/.asset_state.tsv
    /// （AssetReadyTable.hpp）：display 重启后加载恢复——已就绪素材零 CRC
    /// 零网络秒就绪，根治"重启后已就绪文件被全量重传/重扫"（重复同步
    /// 修复链 display 侧）。
    std::map<std::string, std::pair<std::uint64_t, std::string>> asset_done_;
    /// 就绪表磁盘缓存是否已加载（懒加载一次；state_mutex_ 保护——
    /// 仅在 fetchAssetsAsync 的拷贝段锁内读写）
    bool ready_table_loaded_ = false;

    /// 上次打印"素材去重"日志时的待拉数（SIZE_MAX = 尚未打过）。
    /// 【刷屏抑制 2026-11-05】editor 每次自动下发场景都会重走素材就绪
    /// 比对，常态（全部就绪）下待拉数恒定——仅在数值变化时打印日志，
    /// 避免终端被无信息量的重复行刷屏。场景帧处理线程串行访问，免锁。
    std::size_t last_pending_assets_ = SIZE_MAX;

    // ---- 原子轻量标记（跨线程读、免锁）------------------------------------
    std::atomic<bool> registered_{false};
    std::atomic<bool> assets_ready_{false}; ///< 全部素材就绪（无素材恒 true）
    std::atomic<std::size_t> assets_ready_cnt_{0};
    std::atomic<std::size_t> assets_total_{0};
};

} // namespace ms
