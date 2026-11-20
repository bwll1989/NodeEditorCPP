// ============================================================================
//  PlayerRuntime.cpp —— Player 侧运行时实现
//
//  各环节与设计文档的对应：
//    * 注册（§6.2）：UDP 先绑定 → 端口随 MsgRegister 上报 → Editor 得到
//      「player_id → UDP 地址」映射（TCP 对端 IP + udp_report_port）；
//    * 锚点应用（§6.1）：不在"收到时"执行，AnchorScheduler::poll 到点才
//      应用；session_start = anchor_time - anchored_session_time；
//    * 播放头推进（§4.4）：current += dt × rate（dt 为两次 tick 间隔），
//      target = aligned_now - session_start（全场统一公式）；
//    * 纠偏（§7.1）：DriftController 纯函数决策——环内不动 / 微调 ±0.5%
//      / >40ms 硬 seek（seek 后速率复位）；
//    * 迟到加入（§6）：Idle 状态收到 Editor Playing 心跳 → 直接学
//      session_start 进入播放（session_start 是全场共享值，抄即对齐）；
//    * 上报（§7.1①）：SyncFrame 携带 state/session_start/current/delta，
//      Editor 按 player_id 归位做集中观测。
// ============================================================================

#include "runtime/PlayerRuntime.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstring>
#include <unordered_set>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <map>
#include <mutex>
#include <thread>

#include "clock/ClockProvider.hpp"
// （第七迭代：增量补丁机制退役——场景只经 MsgScene 全量下发，无 patch 路径）
#include "model/AssetDeps.hpp" // 素材依赖计算（clip∩screen，§5.5 补丁）
#include "net/AssetReceiver.hpp"
#include "runtime/AssetPath.hpp" // 素材归集寻址（2026-11-13：store/resolve）
#include "runtime/AssetReadyTable.hpp" // 就绪表持久化（2026-11-09）

namespace ms {

// 【排障】stderr 时戳（毫秒）：display 关键事件与 editor/diag 日志对齐时间线
namespace {
long long tsMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
/// 60fps 帧槽宽（us）：播放头 → 帧号换算基准（诊断打点 / 状态上报 /
/// frame_index() 三处同源——2026-11-06 收敛原三处独立硬编码 16667）。
constexpr std::int64_t kFrameSlotUs = 1'000'000 / 60;

// ---- 端间时钟校准探测参数（2026-11-16；SNTP 式 4 时间戳法）--------------
// 每轮 kProbeCount 个样本取 RTT 最小者（排队/重传污染最小的测量最可信）；
// 轮间隔跟踪时钟漂移；采纳阈值防抖（offset 微动不写，免得播放头反复微跳）。
constexpr int         kProbeCount         = 8;
constexpr std::int64_t kProbeIntervalUs   = 100'000;     // 请求间隔 100ms
constexpr std::int64_t kProbeRoundUs      = 30'000'000;  // 重测周期 30s
constexpr std::int64_t kProbeTimeoutUs    = 5'000'000;   // 单轮截止 5s
constexpr int         kProbeMinSamples    = 3;           // 最低采纳样本数
constexpr std::int64_t kOffsetAdoptDeltaUs = 50'000;     // 采纳阈值 50ms
} // namespace（文件局部常量到此；成员函数不可定义于匿名命名空间）

using ms_proto::Envelope;

/// 端间时钟校准探测（SNTP 式，2026-11-16）——原理与线程模型见头文件。
/// tick 末尾驱动：开轮 / 按间隔发请求 / 收满或超时结算。
void PlayerRuntime::maybeProbeTick(std::int64_t now_us) {
    if (cfg_.standalone || !registered_.load()) return; // 本机模式 / 未注册

    SyncFrame probe;
    bool send_now = false;
    bool settle   = false;
    {
        std::lock_guard<std::mutex> lock(probe_mutex_);
        if (!probing_) {
            // 轮节流：距上轮结算不足 30s 不开新轮（首轮 last=0 立即开）
            if (last_probe_round_us_ != 0 &&
                now_us - last_probe_round_us_ < kProbeRoundUs)
                return;
            probing_   = true;
            probe_sent_ = 0;
            probe_recv_ = 0;
            probe_samples_.clear();
            ++probe_seq_; // 轮序号（int32 溢出 ≈ 2000 年，无忧）
            probe_next_us_     = now_us;
            probe_deadline_us_ = now_us + kProbeTimeoutUs;
        }
        if (probe_recv_ >= kProbeCount || now_us >= probe_deadline_us_) {
            settle = true; // 结算在锁外（内部重拿 probe_mutex_）
        } else if (probe_sent_ < kProbeCount && now_us >= probe_next_us_) {
            probe.kind       = SyncKind::TimeProbeReq;
            probe.role       = SyncRole::Player;
            // T1：发送前即时取对齐时钟（【精度 2026-11-20】原取 tick 时戳
            // now_us——粒度 ±10ms 直接成为 offset 误差上限；即时读数亚毫秒，
            // 与 T4 的 recv 线程即时取对等。时戳到 sendTo 的 CPU 时间进
            // RTT，被"RTT 最小样本"筛选排除，无系统性偏差）
            probe.t1_us      = clock_.alignedNowUs();
            probe.delta_us   = probe_seq_; // 轮序号（响应回传匹配）
            send_now = true;
            ++probe_sent_;
            probe_next_us_ = now_us + kProbeIntervalUs;
        }
    }
    if (settle) {
        settleProbe(now_us);
    } else if (send_now) {
        udp_.send(probe); // 锁外 IO
    }
}

/// 采样结算（tick 主循环上下文）：RTT 最小样本 = 排队污染最小的测量。
/// offset 写入 clock_ 后本机对齐时钟即进入 Editor 时钟域——正在播放时
/// alignedNow 突变直接体现为下一拍 dt（跳进/不推进各方向均正确），纠偏环
/// target 同源突变，播放头一步到位，无需额外重锚。
/// 【锁内绝不做 I/O（2026-11-16 校准打印卡死实测）】原实现在 probe_mutex_
/// 临界区内 printf——Windows 控制台是同步阻塞 I/O，QuickEdit 拖选/管道
/// 满时打印无限期挂起，主线程持锁挂死 → recv 线程 onProbeResponse 抢
/// probe_mutex_ 连锁挂死 → UDP 心跳链冻结（与下方诊断日志"锁内快照 +
/// 锁外打印"同一纪律）。数据结算与 offset 写入在锁内完成，打印移出。
void PlayerRuntime::settleProbe(std::int64_t now_us) {
    std::int64_t offset = 0, rtt = 0, cur_off = 0;
    int       received = 0;
    bool      adopt    = false;
    int       fail_streak = 0; // 本轮失败连击（0 = 成功轮；锁外告警判定）
    {
        std::lock_guard<std::mutex> lock(probe_mutex_);
        last_probe_round_us_ = now_us;
        probing_             = false;
        if (static_cast<int>(probe_samples_.size()) < kProbeMinSamples) {
            // 样本不足（丢包严重）：放弃本轮等下轮，不动 offset
            probe_samples_.clear();
            fail_streak = ++probe_fail_streak_; // 锁内簿记（打印挪锁外）
        } else {
            probe_fail_streak_ = 0; // 成功轮清零连击
            const auto best = std::min_element(
                probe_samples_.begin(), probe_samples_.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            offset   = best->first;
            rtt      = best->second;
            cur_off  = clock_.offsetUs();
            received = probe_recv_; // 结算前留存（下面 clear）
            probe_samples_.clear();
            // 首次（offset==0）必采纳；此后仅显著变化才写（±50ms 防抖，避免
            // 测量噪声反复微调播放头）
            adopt = (cur_off == 0 ||
                     std::llabs(offset - cur_off) > kOffsetAdoptDeltaUs);
        }
    }
    // 锁外打印（锁内绝不做 I/O 纪律；now_us/now_ms 仅 tick 线程 touch）
    if (fail_streak >= 3) {
        const std::int64_t now_ms = now_us / 1000;
        if (now_ms - last_probe_fail_warn_ms_ >= 60'000) {
            last_probe_fail_warn_ms_ = now_ms;
            std::fprintf(stderr,
                         "\n[player] W-时钟探测持续失败（连续 %d 轮样本<%d）"
                         "——检查 display→editor 的 UDP 单向连通（防火墙/路由）"
                         "；期间心跳被动校准兜底（若 Editor 心跳可达）\n\n",
                         fail_streak, kProbeMinSamples);
            std::fflush(stderr);
        }
        return;
    }
    if (fail_streak > 0) return; // 失败但未到告警阈值：静默等下轮
    if (adopt) {
        clock_.setOffsetUs(offset); // 原子量，锁外写安全（tick 单写者）
        // stderr 通道（项目日志纪律：无缓冲即时落盘，stdout 重定向为全缓冲）
        std::fprintf(stderr,
                     "player: 时钟校准: offset=%+.3fs rtt=%.1fms"
                     "（旧 offset=%+.3fs；samples=%d/%d）\n",
                     static_cast<double>(offset) / 1e6,
                     static_cast<double>(rtt) / 1e3,
                     static_cast<double>(cur_off) / 1e6,
                     received, kProbeCount);
    }
}

/// 探测响应（UDP recv 线程上下文）：SNTP 四时间戳算本样本 offset/rtt。
/// T1/T4 同含当前生效 offset，相减抵消——测量与既有 offset 数学解耦，
/// 反复校准无反馈发散。
void PlayerRuntime::onProbeResponse(const SyncFrame& f,
                                    std::int64_t t4_us) {
    std::lock_guard<std::mutex> lock(probe_mutex_);
    if (!probing_) return;                       // 无进行中的采样轮
    if (f.delta_us != probe_seq_) return;        // 过期/伪造轮序号
    if (f.t1_us == 0 || f.t3_us < f.t2_us) return; // 字段合理性防御
    const std::int64_t offset =
        ((f.t2_us - f.t1_us) + (f.t3_us - t4_us)) / 2;
    const std::int64_t rtt = (t4_us - f.t1_us) - (f.t3_us - f.t2_us);
    if (rtt < 0 || rtt > kProbeTimeoutUs) return; // 负 RTT/超时样本丢弃
    probe_samples_.emplace_back(offset, rtt);
    ++probe_recv_;
}

// ---------------------------------------------------------------------------
// 心跳被动粗校准（recv 线程；无锁——滑窗字段仅本线程 touch）
//
// 【为什么需要】SNTP 主动探测有冷启动空窗（注册后首轮 8×100ms≈0.7s 才
// 结算）与失败态（UDP 单向被防火墙拦截时样本永远不足 3 个）——期间
// offset==0，锚点 time戳走裸墙钟域，跨机秒级差直接触发域差兜底（画面
// 不冻结但端间各按单向延迟重表达）。本通道从 Editor 心跳帧免费拿
// Editor 域时戳：raw = editor - local，多拍中位数过滤网络抖动后粗修
// 大差（>250ms 才动），把"秒级裸差"在 1-2s 内消掉；probe 收敛后以其
// 亚毫秒值为准（30s 轮自动精修）。
// ---------------------------------------------------------------------------
void PlayerRuntime::passiveCalibrateTick(std::int64_t editor_aligned_now_us) {
    if (editor_aligned_now_us == 0) return;      // 无数据源帧（旧版 Editor）
    if (cfg_.standalone) return;                 // 本机模式无 Editor 域概念
    if (clock_.offsetUs() != 0) {                // 已有生效 offset：通道自锁
        passive_off_n_ = 0;                      // 清窗（下次 offset 归零重开）
        return;
    }
    const std::int64_t raw =
        editor_aligned_now_us - clock_.alignedNowUs();
    // 滑窗 5 拍：FIFO 推进（环形不必——5 元素 memmove 纳秒级）
    if (passive_off_n_ < 5)
        passive_off_win_[passive_off_n_++] = raw;
    else {
        std::memmove(passive_off_win_, passive_off_win_ + 1,
                     sizeof(passive_off_win_) - sizeof(std::int64_t));
        passive_off_win_[4] = raw;
    }
    if (passive_off_n_ < 5) return;              // 窗未满：先观测
    // 中位数（5 元素，小数组插排即可）
    std::int64_t sorted[5];
    std::copy(std::begin(passive_off_win_), std::end(passive_off_win_),
              std::begin(sorted));
    std::sort(std::begin(sorted), std::end(sorted));
    const std::int64_t median = sorted[2];
    constexpr std::int64_t kPassiveAdoptUs = 250'000;  // 只修秒级裸差
    // 一致性：中位数显著 + 各拍与中位差 < 200ms（抖动过大 = 网络污染，弃）
    if (std::llabs(median) < kPassiveAdoptUs) return;
    for (const auto v : sorted)
        if (std::llabs(v - median) > 200'000) return;
    clock_.setOffsetUs(median);
    ++passive_adopted_;
    std::fprintf(stderr,
                 "player: 心跳被动校准: offset=%+.3fs（SNTP 未生效期"
                 "粗修，主动探测收敛后精修；adopted=%d）\n",
                 static_cast<double>(median) / 1e6, passive_adopted_);
    std::fflush(stderr);
}

/// 素材本地寻址（2026-11-13 归集改造收敛进 runtime/AssetPath.hpp——原
/// 本文件内 assetLocalPath（盘符绝对路径直通）退役：绝对路径素材此前
/// "直读原路径 + 下载保存回原路径"，display 的 assets_root 完全无效
///（用户配置 assets_root 期望素材归集，实测 assets/ 永远为空）。
/// 新语义：下载保存 assetStorePath（绝对路径也落 assets_root/文件名）；
/// 读取/就绪判定 assetResolvePath（归集副本优先，缺失回退原路径——
/// 同机原文件在时零下载直读，文件缺失才拉取归集）。
/// 调用点：① 就绪 stat（原 assetLocalPath→assetResolvePath）；
/// ② 拉取保存（原 assetLocalPath→assetStorePath）。

// ---------------------------------------------------------------------------
// 构造 / 生命周期
// ---------------------------------------------------------------------------
PlayerRuntime::PlayerRuntime(Config cfg) : cfg_(std::move(cfg)) {
    // 生效 id 初值 = 本地标签（指派模型 2026-11-06：连接后被 Ack 指派覆盖）
    effective_id_ = cfg_.player_id;
}

std::string PlayerRuntime::effectiveId() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return effective_id_;
}

bool PlayerRuntime::start() {
    // ---- 0. standalone 本机模式（display×MMC 合并方案 v2.2，2026-11-11）--
    // 单机播放节点：跳过网络三件套（UDP 上报 / TCP 监听 / 局域网发现），
    // 场景与播放控制经本机方法组由 NodeController 注入；渲染线程/音频
    // 照常装配。素材本地就绪（NodeController 负责 assets_root 相对路径
    // 装配），无需网络拉取 → 素材就绪旗标直接置位。
    if (cfg_.standalone) {
        std::printf("player: standalone 模式: 网络通道关闭"
                    "（本机控制，NodeController 装配）id=%s\n",
                    cfg_.player_id.c_str());
        assets_ready_.store(true); // 本地素材无需拉取 = 视为就绪
        startRenderThread();
        return true;
    }

    // ---- 1. 先绑 UDP：注册帧要携带 udp_report_port ----
    if (!udp_.start(0)) { // 随机端口（P2 MVP；固定端口留给防火墙友好部署）
        std::fprintf(stderr, "player: UDP 绑定失败\n");
        return false;
    }

    // ---- 2. 挂 UDP 回调（必须在收包前装好）----
    // lambda 捕获 this：运行期不得解构本对象（main 保证 stop 后退出）
    udp_.set_on_frame([this](const SyncFrame& f,
                             const Poco::Net::SocketAddress& from) {
        onUdpFrame(f, from);
    });

    // ---- 3. 建立控制通道（监听模式，WATCHOUT 模型）----
    //   ControlServer 监听 listen_port，零操作等待 Editor 主动接入；
    //   连接建立回调里立即上报 Register（身份先行，Editor 随即回
    //   Ack + 下发场景）。【旧直连模式（ControlClient 主动连 editor）
    //   已退役——display 入口硬编码本模式，Editor 端重连由
    //   DisplayConnector 负责】
    listener_.set_on_connected([this](std::uint64_t cid) {
        editor_conn_.store(cid);
        authed_conn_.store(0); // 新连接须重新认证（配对码按连接粒度）
        // 连接建立即上报身份（§6.2）：player_id + version + UDP 上报端口。
        // Editor 收到后回 Ack（含其 UDP 心跳地址）+ 下发场景。
        if (!sendRegister())
            std::fprintf(stderr, "player: 注册帧发送失败\n");
    });
    listener_.set_on_envelope([this](std::uint64_t cid,
                                     const Envelope& env) {
        onEnvelope(env, cid);
    });
    listener_.set_on_disconnected([this](std::uint64_t cid) {
        // 【精确清理】只清断开连接自己的状态——被踢连接与正常连接并存时
        // （配对码场景，Editor A 被踢、B 已接入），A 的断连回调不得误伤
        // B 的会话状态
        if (editor_conn_.load() == cid) {
            editor_conn_.store(0);
            registered_.store(false);
            std::printf("player: Editor 连接断开（等待重连，本端配置零改动）\n");
        }
        if (authed_conn_.load() == cid) authed_conn_.store(0);
    });
    // 【同机多实例（2026-11-03）】默认端口被占（典型：同一台电脑开了
    // 第二个 media_display）时自动改用系统分配端口，第二实例照常服务
    // 而不是启动失败退出。实际端口两条途径让 Editor 知晓：
    //   1. 局域网发现应答帧 listen_port（屏幕属性面板扫描列表自动带出）；
    //   2. 下面的控制台日志（人工登记兜底）。
    // 显式传 0（系统分配）仍失败 = 系统级问题（无可用端口/权限），退出。
    // exclusive_bind=true：独占端口——多实例时第二实例 bind 真失败走
    // fallback（缺省 SO_REUSEADDR 共享绑定时 Windows 上两实例会双双
    // bind 成功 17100，但连接只投递其一，另一实例"假监听"收不到 Editor）
    if (!listener_.start(cfg_.listen_port, /*exclusive_bind=*/true)) {
        if (cfg_.listen_port == 0) {
            std::fprintf(stderr,
                         "player: 监听失败（系统分配端口不可用？）\n");
            udp_.stop();
            return false;
        }
        std::fprintf(stderr,
                     "player: 监听 port=%u 被占用（同机多 display？），"
                     "改用系统分配端口\n", cfg_.listen_port);
        if (!listener_.start(0, /*exclusive_bind=*/true)) {
            std::fprintf(stderr, "player: 系统分配端口监听失败\n");
            udp_.stop();
            return false;
        }
    }
    std::printf("player: 监听模式: tcp=%u（等待 Editor 接入）id=%s udp=%u\n",
                listener_.port(), cfg_.player_id.c_str(), udp_.port());
    // 端口变化显著提示（多实例部署的登记依据）
    if (listener_.port() != cfg_.listen_port)
        std::printf("player: 注意: 实际监听端口 %u ≠ 配置端口 %u"
                    "（Editor 端请按实际端口登记显示端）\n",
                    listener_.port(), cfg_.listen_port);

    // ---- 4. 渲染线程（§5.6）：render_fps>0 启用；providers 全部经
    // 成员函数快照（内部持 state_mutex_），渲染线程与网络线程零竞争 ----
    startRenderThread();
    return true;
}


// ---------------------------------------------------------------------------
// 渲染线程装配（§5.6；standalone 与受控模式共用——2026-11-11 从 start() 提取）
// ---------------------------------------------------------------------------
void PlayerRuntime::startRenderThread() {
    if (cfg_.render_fps <= 0) return;
    RenderThread::Config rcfg;
    rcfg.player_id     = cfg_.player_id;
    rcfg.fps           = cfg_.render_fps;
    rcfg.out_w         = cfg_.render_w;
    rcfg.out_h         = cfg_.render_h;
    rcfg.capture_dir   = cfg_.render_capture_dir;
    rcfg.capture_interval_us = cfg_.render_capture_interval_us;
    rcfg.capture_max   = cfg_.render_capture_max;
    rcfg.assets_root   = cfg_.assets_root; // FFmpeg 源按素材根目录装配
    rcfg.hw_accel      = cfg_.hw_accel;    // 硬解意图透传（探测失败自动软解）
    rcfg.frame_sink    = cfg_.render_frame_sink; // 预览窗口帧回调透传
    rcfg.backend_factory = cfg_.render_backend_factory; // GL 后端注入透传
    rcfg.audio_enable  = cfg_.audio_enable; // P3：音频输出（随渲染拍）
    rcfg.audio_device  = cfg_.audio_device; // 音频设备（空=默认）
    // 【素材同步 OSD 2026-11-16】原子量采样注入（零锁）——active 期间
    // GL 输出画面底部叠"素材同步中 n/m · xx%"，就绪后自动消失
    rcfg.asset_osd_provider = [this]() {
        RenderThread::AssetOsdState s;
        s.active  = !assets_ready_.load(std::memory_order_relaxed);
        s.done    = static_cast<int>(assets_ready_cnt_.load(
                        std::memory_order_relaxed));
        s.total   = static_cast<int>(assets_total_.load(
                        std::memory_order_relaxed));
        s.file_percent = asset_file_percent_.load(std::memory_order_relaxed);
        return s;
    };
    render_ = std::make_unique<RenderThread>(
        std::move(rcfg),
        [this]() { return sceneSnapshot(); },     // 场景快照（锁内拷贝）
        [this]() { return playheadSnapshot(); },  // 播放头快照
        [this]() { return renderClockNow(); },    // 对齐时钟（可注入）
        [this]() { return sceneSerial(); },       // 场景序号（判"已换"）
        [this]() { return effectiveId(); });      // 生效 id（指派模型）
    render_->start();
    std::printf("player: 渲染线程已启动: %dfps 输出=%dx%d 捕获=%s\n",
                cfg_.render_fps,
                render_->outWidth(), render_->outHeight(),
                cfg_.render_capture_dir.empty() ? "关"
                                                : cfg_.render_capture_dir.c_str());
}

// ---------------------------------------------------------------------------
// 注册帧（on_connected 触发；§6.2）
// ---------------------------------------------------------------------------
bool PlayerRuntime::sendRegister() {
    Envelope reg;
    auto* m = reg.mutable_register_();
    // 【指派模型 2026-11-06】注册帧报生效 id（cfg_.player_id 或 Editor
    // 已指派的槽位 id）——换 id 后的重注册由此完成换键。
    m->set_player_id(effectiveId());
    m->set_version(cfg_.version);
    m->set_udp_report_port(udp_.port());
    // 发往已接入的 Editor 连接（on_connected 记录）
    const auto cid = editor_conn_.load();
    if (cid == 0) return false; // 尚无连接（正常：等 on_connected 触发）
    return listener_.send_to(cid, reg);
}

void PlayerRuntime::stop() {
    // 渲染线程先于网络关闭 join：其 provider 回调引用本对象成员，
    // 必须保证 join 完成后才动 UDP/TCP 与后续析构（§5.6 线程契约）
    if (render_) render_->stop();
    // 【先停网络再 join 素材线程】recv 线程（onEnvelope）随时可能再次
    // 调 fetchAssetsAsync（spawn/重赋值 asset_thread_）——先终结所有
    // spawn 源头，再 join，消除"join 与线程对象重新赋值"的竞态窗口
    listener_.stop(); // 关监听 + 断开 Editor 连接（幂等）
    udp_.stop();      // 关 UDP + join 接收线程（幂等）
    // 素材自愈循环快速退出标志（2026-11-04）：轮间休眠 200ms 粒度检查，
    // join 不再被退避 sleep 卡住（旧实现最长等 3×500ms 重试链）
    asset_quit_.store(true);
    // 最后 join 素材线程（若在拉取中会等当前 HTTP 超时/完成——避免线程
    // 引用已销毁的成员；正常联测素材早已拉完，join 立即返回）
    if (asset_thread_.joinable()) asset_thread_.join();
}

void PlayerRuntime::onEnvelope(const Envelope& env, std::uint64_t conn_id) {
    // ---- 【recv 线程卡顿探针，2026-11-05】----
    // 语义：上一帧处理完到本帧进入 handler 的间隔。帧间隔正常 = 对端发送
    // 节奏（场景发布 50ms 防抖，联测高频发布可到 20ms）；>100ms 说明 recv
    // 线程被上一帧
    // 的 handler 内阻塞（场景解析 / 素材拉取 join / 素材进度上报阻塞写），
    // 或对端根本没写 —— 这正是"editor 已更新、display 却迟迟不切场景"的
    // 第一现场证据（TCP 全双工：只堵这条连接的读，后续帧全排队）。
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        static long long s_last_ms = 0;
        const long long now_ms = tsMs();
        if (s_dbg && s_last_ms != 0 && now_ms - s_last_ms > 100) {
            std::fprintf(stderr,
                         "[%lld][player] recv 线程停顿 %lldms（本帧含 "
                         "scene=%d ack=%d re_anchor=%d asset_sync=%d）\n",
                         now_ms, now_ms - s_last_ms,
                         (int)env.has_scene(), (int)env.has_register_ack(),
                         (int)env.has_re_anchor(), (int)env.has_asset_sync());
            std::fflush(stderr);
        }
        s_last_ms = now_ms;
    }

    // ---- 配对认证门卫（cfg_.pairing_code 非空时启用；空 = 全兼容）----
    // 连接建立后 Editor 应发 MsgAuth（配对证明）；认证通过前，该连接上
    // 的业务帧（Ack/Scene/ReAnchor）【缓存不丢】——Editor 侧 MsgAuth（看
    // 门狗线程）与 Ack/Scene（发送线程）并发写同一 socket，到达顺序不
    // 保证，场景帧可能先于认证帧抵达。缓存上限 32 帧（注册握手的业务
    // 帧就 2~3 帧，余量充足），认证通过后按序回放。
    if (!cfg_.pairing_code.empty()) {
        if (env.has_auth()) {
            if (env.auth().pairing_code() == cfg_.pairing_code) {
                authed_conn_.store(conn_id);
                std::printf("player: 配对认证通过 conn=%llu\n",
                            (unsigned long long)conn_id);
                // 回放认证前缓存的业务帧（FIFO；帧内已无 Auth 帧）
                std::vector<Envelope> replay;
                {
                    std::lock_guard<std::mutex> lock(pending_mutex_);
                    for (auto it = pending_frames_.begin();
                         it != pending_frames_.end();) {
                        if (it->first == conn_id) {
                            replay.push_back(std::move(it->second));
                            it = pending_frames_.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                for (auto& e : replay) onEnvelope(e, conn_id); // 重入门卫放行
            } else {
                std::fprintf(stderr,
                             "player: 配对码校验失败，踢除连接 conn=%llu"
                             "（对端可能不是本项目 Editor）\n",
                             (unsigned long long)conn_id);
                listener_.close(conn_id); // 异步踢除（recv 线程上下文安全）
                authed_conn_.store(0);
                std::lock_guard<std::mutex> lock(pending_mutex_);
                pending_frames_.erase(
                    std::remove_if(pending_frames_.begin(),
                                   pending_frames_.end(),
                                   [conn_id](const auto& p) {
                                       return p.first == conn_id;
                                   }),
                    pending_frames_.end());
            }
            return; // Auth 帧自身无业务语义，到此为止
        }
        if (conn_id != authed_conn_.load()) {
            // 未认证连接的业务帧：入缓存（有界；不回包不踢除——给 Editor
            // 留出 Auth 到达余量，认证通过后回放）
            std::lock_guard<std::mutex> lock(pending_mutex_);
            if (pending_frames_.size() >= 32) pending_frames_.pop_front();
            pending_frames_.emplace_back(conn_id, env);
            return;
        }
    }

    // ---- RegisterAck：入会话（会话切换时旧锚点队列作废）----
    if (env.has_register_ack()) {
        const auto& ack = env.register_ack();
        if (!ack.accepted()) {
            std::fprintf(stderr, "player: 注册被拒: %s\n",
                         ack.reason().c_str());
            return;
        }
        std::uint16_t http_port = 0;
        bool refetch_on_port = false; // 端口首次学到且场景已就位 → 补触发拉取
        bool id_reassigned = false;   // 指派 id 与生效 id 不同（出锁后重注册）
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (session_id_ != ack.session_id()) {
                scheduler_.clear(); // 新会话：未应用的旧锚点全部作废（§6.2）
                // 旧会话的播放状态一并作废（2026-11-20 注册状态对账配套）：
                // session 号更替 = Editor 进程重启，旧 state/session_start
                // 已无权威来源——保持播放会让 display 失控跑旧内容、两端
                // 时间完全脱节。归零后由紧跟本 Ack 的注册状态对账帧
                //（Editor 单播）恢复权威播放状态。
                state_            = PlayState::Idle;
                session_start_us_ = 0;
                current_us_       = 0;
                rate_             = 1.0;
                ticking_          = false;
                session_id_       = ack.session_id();
            }
            // 学习素材分发端口（§6 地址互相学习）：0 = Editor 未启用分发
            http_port = static_cast<std::uint16_t>(ack.http_port());
            refetch_on_port =
                (editor_http_port_ == 0 && http_port != 0 && scene_ != nullptr);
            editor_http_port_ = http_port;
            // 学习 Editor 主机（§6 地址互相学习；监听模式下本端不配置
            // Editor 地址——素材 HTTP 拉取的主机部分以 Ack 告知为准）
            if (!ack.udp_host().empty()) editor_host_learned_ = ack.udp_host();
            // 【指派 id 2026-11-06】Editor 随 Ack 下发工程槽位 id：连接靠
            // ip:port，屏身份由 Editor 指派、本端采用——非空且 ≠ 生效 id
            // 时覆盖（自报 --id 只是本地标签）。采用后重新注册：Editor
            // 注册表按注册帧 pid 键控，重注册即自然换键（其端自动清理
            // 同槽位旧条目）。
            if (ack.has_assigned_player_id()
                && !ack.assigned_player_id().empty()
                && ack.assigned_player_id() != effective_id_) {
                effective_id_ = ack.assigned_player_id();
                id_reassigned = true;
            }
        }
        registered_.store(true);
        // 【指派 id 采用】必须在锁外重发（sendRegister 内部取锁，重入死锁）；
        // 换 id 前 Ack/Scene 里的过滤均以旧 id 进行，本帧后续逻辑不受影响。
        if (id_reassigned) {
            std::fprintf(stderr,
                         "[%lld][player] 采用 Editor 指派 id: %s（本地标签 --id=%s）"
                         " → 重新注册换键\n",
                         tsMs(), effectiveId().c_str(), cfg_.player_id.c_str());
            std::fflush(stderr);
            sendRegister();
        }
        // 端口竞态自愈（推模型，2026-11-04）：AssetSync 清单到达但端口未学
        // 到时会重注册换新 Ack——此处端口首次学到且场景已就位 → 立即补拉，
        // 素材就绪不依赖下一次编辑器动作（清单已被丢弃，不能再等推送）。
        if (refetch_on_port) {
            std::fprintf(stderr,
                         "player: HTTP 端口首次学到（%u）→ 补触发素材拉取\n",
                         static_cast<unsigned>(http_port));
            fetchAssetsAsync(editor_host_learned_, http_port);
        }
        // 学习 Editor 的 UDP 心跳地址（Ack 下发）→ 后续上报帧发往此处
        if (ack.udp_port() != 0 && !ack.udp_host().empty())
            udp_.set_remote(ack.udp_host(),
                            static_cast<std::uint16_t>(ack.udp_port()));
        std::fprintf(stderr, "[%lld][player] 注册成功 session=%s editor_udp=%s:%u http=%s:%u\n",
                    tsMs(),
                    ack.session_id().c_str(), ack.udp_host().c_str(),
                    ack.udp_port(), ack.udp_host().c_str(),
                    static_cast<unsigned>(ack.http_port()));
        return;
    }

    // ---- Scene：整场景下发 → 锁外解析校验 → 锁内存入 ----
    if (env.has_scene()) {
        const auto& msg = env.scene();
        const auto parse_t0 = std::chrono::steady_clock::now();
        std::string err;                       // parse 含引用完整性校验
        auto scene = SceneLoader::parse(msg.payload(), err);
        if (!scene) {
            std::fprintf(stderr, "player: 场景解析失败: %s\n", err.c_str());
            return;
        }
        std::uint16_t http_port = 0;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            // optional → shared_ptr<const Scene>：渲染线程经 sceneSnapshot()
            // 拷贝智能指针快照，与网络线程的覆盖写互斥但不拷贝场景内容
            scene_     = std::make_shared<const Scene>(std::move(*scene));
            ++scene_serial_;               // 渲染线程据此判"场景已换"
            http_port  = editor_http_port_; // Ack 阶段学到的分发端口
        }
        // 【下发延迟探针】解析耗时（recv 线程内、锁外）——大场景/素材表
        // 上千项时该值即"display 收到但还没切换"的那段固定成本
        const auto parse_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - parse_t0).count();
        std::fprintf(stderr,
                     "[%lld][player] 场景已接收: id=%s version=%s (%zu 字节) "
                     "解析=%lldus\n",
                    tsMs(),
                    msg.scene_id().c_str(), msg.version().c_str(),
                    msg.payload().size(), static_cast<long long>(parse_us));
        // 【场景内容摘要 2026-11-13】"display 收到的到底是什么"一线可见——
        //  两端场景脱节（编辑后未下发/下发旧状态）排障的显示端证据链：
        //  屏（id@player + 画布矩形）+ 每 clip（时间域 + asset_id + 矩形）
        // 【锁内绝不做 I/O 2026-11-16】锁内只拷贝 shared_ptr 快照（原子
        //  引用计数，零内容拷贝），遍历打印全部在锁外——QuickEdit 拖选
        //  挂起打印时不再持 state_mutex_ 连锁冻结渲染线程。
        {
            std::shared_ptr<const Scene> snapshot;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                snapshot = scene_;
            }
            if (snapshot) {
                for (const auto& scr : snapshot->screens) {
                    // 屏姿态标注（阶段2c）：滚转/俯仰/偏航任一非 0 即追加——
                    // 屏姿态排障第一证据（display 收到的姿态值直接可见）
                    char pose[72] = "";
                    if (scr.rotation != 0.0 || scr.rot_x != 0.0 ||
                        scr.rot_y != 0.0)
                        std::snprintf(pose, sizeof(pose),
                                      " 姿态(滚%.0f° 俯%.0f° 偏%.0f°)",
                                      scr.rotation, scr.rot_x, scr.rot_y);
                    std::fprintf(stderr,
                                 "  [scene] 屏 %s → %s 画布(%.0f,%.0f "
                                 "%.0fx%.0f)%s\n",
                                 scr.id.c_str(), scr.player_id.c_str(),
                                 scr.x, scr.y, scr.w, scr.h, pose);
                }
                for (const auto& tk : snapshot->timeline.tracks)
                    for (const auto& c : tk.clips)
                        std::fprintf(stderr,
                                     "  [scene] 轨'%s' clip=%s asset=%s "
                                     "域[%.2f,%.2f)s 矩形(%.1f,%.1f %.1fx%.1f)\n",
                                     tk.id.c_str(), c.id.c_str(),
                                     c.asset_id.c_str(),
                                     static_cast<double>(c.start_us) / 1e6,
                                     static_cast<double>(c.start_us + c.duration_us) / 1e6,
                                     c.transform.x, c.transform.y,
                                     c.transform.w, c.transform.h);
                for (const auto& a : snapshot->assets)
                    std::fprintf(stderr, "  [scene] 素材 %s kind=%d path=%s\n",
                                 a.id.c_str(), static_cast<int>(a.kind),
                                 a.path.c_str());
            }
        }
        std::fflush(stderr);

        // ---- 触发素材拉取（§5.5）：后台线程逐个 HTTP 拉取 ----
        // host 用 Ack 学到的 Editor 地址（编辑器告知）；port 来自 Ack。
        // 未启用（assets_root 空 / http_port=0）时 fetchAssetsAsync 内部
        // 直接置 ready 返回。
        fetchAssetsAsync(editor_host_learned_, http_port);
        return;
    }

    // （第七迭代：MsgPatch 增量补丁路径已退役——场景同步一律 MsgScene
    //  全量广播，收到即整体切换：解析 + 校验 + 原子换指针，与上方 Scene
    //  分支完全同构；播放头三元组不碰，渲染线程下一帧经快照自动换版。）

    // ---- AssetSync：素材清单推送（推模型触发，2026-11-04）----
    // 语义：Editor 侧素材在分发侧注册/变化时主动推清单——本端据此触发
    // 拉取（与场景消息触发等效；独立于场景版本，注册即推/重发即推）。
    // 清单本身不携带文件内容（大文件走 HTTP），只承担"该去拉了"的信号 +
    // 最新 CRC（兜住场景里 CRC 空值的窗口）。
    if (env.has_asset_sync()) {
        const auto& sync = env.asset_sync();
        std::uint16_t http_port;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            http_port = editor_http_port_;
        }
        // 端口未学到（0）：Ack 只在注册时发一次，若注册早于 Editor 素材
        // 服务就绪会拿到 0——重新注册换取带端口的 Ack（自愈端口竞态）。
        if (http_port == 0) {
            std::fprintf(stderr,
                         "player: 素材清单到达但 HTTP 端口未就绪 → 重新注册\n");
            sendRegister();
            return;
        }
        std::fprintf(stderr,
                     "[%lld][player] 收到素材清单: %u 项（scene=%s）→ 触发拉取\n",
                     tsMs(),
                     static_cast<unsigned>(sync.entries_size()),
                     sync.scene_version().c_str());
        // 场景可能晚于清单到达（两消息并发下发）——场景未就绪时清单仅
        // 打点不拉取（依赖过滤需要场景几何；下一个场景/清单会再触发）。
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (!scene_) return;
        }
        fetchAssetsAsync(editor_host_learned_, http_port);
        return;
    }

    // ---- ReAnchor：入队等待"到点执行"（§6.1）----
    if (env.has_re_anchor()) {
        const auto& m = env.re_anchor();
        Anchor a;
        a.anchor_time_us           = m.anchor_time_us();
        a.anchored_session_time_us = m.anchored_session_time_us();
        switch (m.cmd()) {
            case ms_proto::MsgReAnchor::CMD_PLAY:  a.cmd = AnchorCmd::Play;  break;
            case ms_proto::MsgReAnchor::CMD_PAUSE: a.cmd = AnchorCmd::Pause; break;
            case ms_proto::MsgReAnchor::CMD_SEEK:  a.cmd = AnchorCmd::Seek;  break;
            case ms_proto::MsgReAnchor::CMD_STOP:  a.cmd = AnchorCmd::Stop;  break;
            default: return; // CMD_NONE：忽略
        }
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            scheduler_.post(a);
        }
        std::fprintf(stderr, "[%lld][player] 收到重锚: cmd=%d anchor_time=%lldus anchored=%lldus\n",
                    tsMs(),
                    static_cast<int>(a.cmd),
                    static_cast<long long>(a.anchor_time_us),
                    static_cast<long long>(a.anchored_session_time_us));
        return;
    }

    // MsgAck 是 Player→Editor 方向，Player 端不处理
}

// ---------------------------------------------------------------------------
// Editor 心跳（UDP recv 线程上下文）
// ---------------------------------------------------------------------------
void PlayerRuntime::onUdpFrame(const SyncFrame& f,
                               const Poco::Net::SocketAddress& /*from*/) {
    if (f.role != SyncRole::Editor) return; // 只关心 Editor 下行心跳

    // ---- 时钟校准探测响应（2026-11-16）：独立路径，不入会话状态机 ----
    // 探测是会话无关的基础设施（probe_mutex_，不碰 state_mutex_）；
    // T4 在 recv 线程即时取——对齐时钟实时读数是 SNTP 的 T4 语义。
    if (f.kind == SyncKind::TimeProbeResp) {
        onProbeResponse(f, clock_.alignedNowUs());
        return;
    }

    std::string  aligned_session; // 【锁内绝不做 I/O】锁内改状态，
    std::int64_t aligned_t = 0;   // 出锁后打印（同 settleProbe/诊断日志纪律）
    {
        std::lock_guard<std::mutex> lock(state_mutex_);

        // 会话过滤：已入会话后，其他会话的心跳一律丢弃（§6.2）
        if (!session_id_.empty() && f.session_id != session_id_) return;

        // ---- 迟到加入软对齐：Idle 时收到 Playing 心跳 → 直接开播 ----
        // session_start 为全场共享值（Editor 心跳帧里带着），抄过来即对齐；
        // current 以 Editor 的 session_time 为初值（误差 = 单向网络延迟，
        // 由纠偏环吸收）。
        // 【防御】负播放头 = Editor 已广播 play 但锚点未到——不接受，
        // 否则会"提前开播"破坏 §6.1 到点执行语义。
        if (state_ == PlayState::Idle && f.state == PlayState::Playing &&
            f.session_time_us >= 0) {
            session_id_       = f.session_id;
            // 【域差兜底 2026-11-20】轴原点本机化重表达：本机对齐时钟读数
            // - 会话时刻，替代直接抄 Editor 域的 f.session_start_us——两端
            // 墙钟裸差（SNTP 未校准）会让抄来的轴原点落在本机"未来"，
            // 渲染 FramePacer 钳 0 → 播放画面冻结（与 applyAnchorLocked
            // 同根因）。本机化误差=单向网络延迟，纠偏环吸收。
            session_start_us_ = clock_.alignedNowUs() - f.session_time_us;
            current_us_       = f.session_time_us;
            ticking_          = false; // 下个 tick 重新锚定 dt 基准
            state_            = PlayState::Playing;
            aligned_session   = f.session_id;
            aligned_t         = current_us_;
        }
        // Playing/Paused 中的心跳：不直接改播放头——纠偏环（tick 内）以共享
        // 时钟公式为准；心跳仅作为"Editor 存活"旁证，两端残余偏差由 Editor
        // 比对我方上报帧集中观测（§7.1①）。
    }
    if (!aligned_session.empty())
        std::fprintf(stderr,
                     "player: 心跳软对齐（迟到加入）: session=%s t=%lldus\n",
                     aligned_session.c_str(),
                     static_cast<long long>(aligned_t));

    // ---- 心跳被动粗校准（SNTP 冷启动空窗/失败态的消解通道；recv 线程）--
    // f.wall_us = Editor 对齐时钟读数（主动单播 + 逐帧回应帧均携带）
    passiveCalibrateTick(f.wall_us);
}

// ---------------------------------------------------------------------------
// 锚点应用（§6.1；锁内调用）
// ---------------------------------------------------------------------------
void PlayerRuntime::applyAnchorLocked(const Anchor& a,
                                      std::int64_t aligned_now_us) {
    if (a.cmd == AnchorCmd::Stop) {
        // STOP：状态归零（Idle）；锚点队列由 tick 的 poll 循环取空
        state_            = PlayState::Idle;
        session_start_us_ = 0;
        current_us_       = 0;
        rate_             = 1.0;
        ticking_          = false;
        return;
    }

    // PLAY / PAUSE / SEEK 共用的"时间设定"（§6.1 公式）：
    //   session_start = anchor_time - anchored_session_time
    // PLAY/SEEK 应用后 session_time(now) = anchored + (now - anchor)，
    // 迟到时自动落在"anchored + 迟到量"——正确的追帧语义。
    session_start_us_ = a.anchor_time_us - a.anchored_session_time_us;

    // ---- 跨机时钟域差兜底（2026-11-20，"播放画面冻结不刷新"根因修复）----
    // 【域差形态】anchor_time 是 Editor 域时戳；display 本机对齐时钟若未
    // 与 Editor 校准（SNTP 未收敛 / UDP 响应被防火墙拦 / 旧版 exe 无
    // probe），裸差 = 两机墙钟原始偏差（秒级常态，可达分钟级）。此时
    // session_start 落在本机时钟"未来"→ 渲染线程 FramePacer 对
    // now<epoch 钳 0 → 播放画面永久冻结在第一帧；暂停分支直通
    // current_us_ 却正常 → "播放不刷新、暂停会跳"的现场形态。
    // 【区分合法未来锚点】协议 §6.1 支持预约式锚点，但当前全部调用方
    // （cmdPlay/cmdPause/cmdSeek）都是立即锚点（anchor≈now）；预约开播
    // 尚无调用方。故 >2s 的"未来"判为域差（网络延迟亚秒不可能 2s+）。
    // 若未来引入预约开播，需在协议加显式 delay 字段，不可依赖裸时戳。
    // 【兜底语义】本机化重表达轴原点：session_start = 本机现在 - anchored
    // ——误差=单向网络延迟（局域网 <5ms），画面立即正常；多端端间差
    // = 各自单向延迟差（亚 10ms），远优于"整端冻结"。SNTP 正常时
    // 偏差亚秒不触发，共享轴精确语义不受影响（分层防御）。
    if (a.anchor_time_us - aligned_now_us > 2'000'000) {
        const std::int64_t domain_gap_us = a.anchor_time_us - aligned_now_us;
        session_start_us_ = aligned_now_us - a.anchored_session_time_us;
        std::fprintf(stderr,
                     "\n[player] W-时钟域差兜底: 锚点在本机时钟未来 %.3fs"
                     "（两端墙钟未校准，SNTP offset=%.3fs）→ 已按本机时钟"
                     "重表达轴原点（画面不冻结；建议检查 UDP 双向连通）\n\n",
                     static_cast<double>(domain_gap_us) / 1e6,
                     static_cast<double>(clock_.offsetUs()) / 1e6);
        std::fflush(stderr);
    }
    rate_             = 1.0;   // 重锚后速率复位（残留调速不叠加）
    ticking_          = false; // 重新锚定 dt 基准，避免旧 dt 造成突变

    switch (a.cmd) {
        case AnchorCmd::Play:
            // 迟到补偿：current 含迟到量（锚点已过 → 直接落在应处位置）
            current_us_ = aligned_now_us - session_start_us_;
            state_      = PlayState::Playing;
            break;
        case AnchorCmd::Seek:
            // 【状态保持语义（2026-11-03 修订）】SEEK 只重定位播放头，不
            // 改变播放状态（旧语义"一律转 Playing"会把暂停/停止中的
            // display 强行拉回播放——编辑器拖播放头定位是高频操作，"暂停
            // 失效/拖动就自动播放"两个用户报告的根因）：
            //   * Playing → 跳转续播：current 含迟到量（与 Play 同式）；
            //   * Paused  → 跳转冻结：current=anchored（不含迟到量，与
            //     Pause 同式——冻结语义下推算值会多出 lead 时长）；
            //   * Idle    → 忽略（无会话上下文；editor 端 Idle 已不广播）。
            if (state_ == PlayState::Playing)
                current_us_ = aligned_now_us - session_start_us_;
            else if (state_ == PlayState::Paused)
                current_us_ = a.anchored_session_time_us;
            break;
        case AnchorCmd::Pause:
            // PAUSE → 冻结于 anchored_session_time（【关键】不是 now 推算：
            // editor 下命令后各端播放头已多走了命令送达延迟，anchored 是
            // 权威冻结点——回跳仅 1~5ms，视觉无感）。PAUSE 锚点 anchor=0
            // 立即到点（见 EditorRuntime::broadcastReAnchor 的立即到点
            // 语义注释：未来锚点会让 display"倒回 lead 时长再停"）
            current_us_ = a.anchored_session_time_us;
            state_      = PlayState::Paused;
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// 本机控制方法组（standalone 模式；display×MMC 合并方案 v2.2，2026-11-11）
//
// 【零新状态机】每个动作 = 构造立即锚点（anchor_time = renderClockNow()）
// 锁内直接 applyAnchorLocked——与受控模式的 ReAnchor 命令共用同一状态
// 迁移代码。锚点不经 scheduler_ 队列：不入队 → tick 的 poll 取不到 →
// 不回发 Ack（standalone 无 Editor 可发，sendAck 里 editor_conn_ 恒 0
// 会打无意义告警，绕开队列即从根上避免）。
// 【时钟域】renderClockNow() = clock_fn 覆盖优先 / ClockProvider 兜底
// ——与主循环 tick(aligned_now_us) 的驱动时钟同域（main 用同一
// ClockProvider；单测注入手工时钟序列），锚点公式的时间基准一致。
// ---------------------------------------------------------------------------

void PlayerRuntime::applyLocalScene(std::shared_ptr<const Scene> s) {
    unsigned long long serial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        scene_ = std::move(s);
        serial = static_cast<unsigned long long>(++scene_serial_); // 渲染线程据 sceneSerial() 判新 → 重建媒体源
    }
    // 锁外打印（锁内绝不做 I/O）：QuickEdit 拖选挂起时不持锁冻结渲染线程
    std::fprintf(stderr, "player: 本机场景注入: serial=%llu\n", serial);
}

void PlayerRuntime::localPlay(std::int64_t from_us) {
    Anchor a;
    a.cmd                    = AnchorCmd::Play;
    a.anchor_time_us         = renderClockNow(); // 立即锚点
    a.anchored_session_time_us = from_us;
    std::lock_guard<std::mutex> lock(state_mutex_);
    applyAnchorLocked(a, a.anchor_time_us);
}

void PlayerRuntime::localPause() {
    Anchor a;
    a.cmd            = AnchorCmd::Pause;
    a.anchor_time_us = renderClockNow();
    // 冻结点 = 当前播放头（applyAnchorLocked 的 PAUSE 语义：
    // current = anchored）。同一次锁内"读 current → 应用"原子完成，
    // 与 tick 的推进无交错窗口。
    std::lock_guard<std::mutex> lock(state_mutex_);
    a.anchored_session_time_us = current_us_;
    applyAnchorLocked(a, a.anchor_time_us);
}

void PlayerRuntime::localSeek(std::int64_t to_us) {
    Anchor a;
    a.cmd                    = AnchorCmd::Seek;
    a.anchor_time_us         = renderClockNow();
    a.anchored_session_time_us = to_us;
    std::lock_guard<std::mutex> lock(state_mutex_);
    applyAnchorLocked(a, a.anchor_time_us);
}

void PlayerRuntime::localStop() {
    Anchor a;
    a.cmd            = AnchorCmd::Stop;
    a.anchor_time_us = renderClockNow();
    std::lock_guard<std::mutex> lock(state_mutex_);
    applyAnchorLocked(a, a.anchor_time_us);
}

// ---------------------------------------------------------------------------
// 主循环节拍
// ---------------------------------------------------------------------------
void PlayerRuntime::tick(std::int64_t aligned_now_us) {
    // ---- 1. 应用所有"到点"锚点（积压多个时循环取空，最早优先）----
    // 【锁外日志 2026-11-17】应用详情收集，出锁打印——"停止后画面迟迟不
    // 切"的日志时间线锚点：收到重锚（recv 线程）→ 本条（tick 应用）两
    // 个时戳一减即命令链耗时；state/t 应用后值直接可见（不用猜）。
    // 【锁内只收 POD】字符串格式化挪到锁外（锁内分配违背临界区最小化
    // 纪律——同 sendAck/打印的锁外原则）；与 Ack 回发共用一次遍历。
    struct AppliedInfo {
        std::int64_t anchor_time_us;
        AnchorCmd    cmd;
        PlayState    state;
        std::int64_t current_us;
    };
    std::vector<AppliedInfo> applied; // 锁内收集 POD，锁外打印+回 Ack
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        while (auto a = scheduler_.poll(aligned_now_us)) {
            applyAnchorLocked(*a, aligned_now_us);
            applied.push_back({a->anchor_time_us, a->cmd, state_, current_us_});
        }
    }
    for (const auto& ai : applied) {
        const char* name = ai.cmd == AnchorCmd::Play   ? "Play"
                           : ai.cmd == AnchorCmd::Pause ? "Pause"
                           : ai.cmd == AnchorCmd::Seek  ? "Seek"
                                                        : "Stop";
        std::fprintf(stderr,
                     "[%lld][player] 锚点已应用: cmd=%s → state=%d "
                     "current_us=%lld\n",
                     tsMs(), name,
                     static_cast<int>(ai.state),
                     static_cast<long long>(ai.current_us));
        std::fflush(stderr);
        // Ack 回发（协议 §6：调试/超时检测用；listener_.send_to 线程安全）
        sendAck(ai.anchor_time_us, true, "applied");
    }

    // ---- 2. 播放头推进 + 纠偏（仅 Playing；锁内完成保证一致性）----
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (state_ == PlayState::Playing && ticking_) {
            // dt = 两次 tick 的真实间隔（时钟回退防护：dt<=0 不推进）
            const std::int64_t dt = aligned_now_us - last_tick_us_;
            if (dt > 0) {
                current_us_ += static_cast<std::int64_t>(
                    static_cast<double>(dt) * rate_);
            }

            // 纠偏环（§7.1）：target 来自共享时钟公式（与 Editor 同式），
            // current 是本机状态机——两者之差即本机相对"理想播放头"的偏差
            const std::int64_t target =
                ClockProvider::sessionTimeUs(aligned_now_us, session_start_us_);
            const auto d = DriftController::decide(target, current_us_, rate_);
            switch (d.action) {
                case DriftController::Action::Seek:
                    current_us_ = d.seek_target_us; // 硬 seek：跳变换正确性
                    rate_       = d.rate;           // 复位 1.0
                    break;
                case DriftController::Action::None:
                    rate_ = d.rate; // 环内：速率回归 1.0
                    break;
                case DriftController::Action::SpeedUp:
                case DriftController::Action::SlowDown:
                    rate_ = d.rate; // 单步调速（±0.5% 上限）
                    break;
            }
            last_delta_us_ = target - current_us_;
        }
        last_tick_us_ = aligned_now_us;
        ticking_      = true;
    }

    // ---- 3. UDP 上报（节流；锁外发送——避免锁内做 IO）----
    // 【全状态上报 2026-11-20】原仅 Playing 上报——Idle/Paused 的 display
    // 在 Editor 观测表无 UDP 心迹（半开连接/网络单向故障不可见：心跳
    // 超时扫描只对收到过上报的 Player 生效）。改 Playing 按周期（250ms）、
    // 其它态低频（1s）上报——在线观测全覆盖，带宽每端 +1 包/s 可忽略。
    // 【standalone】udp_ 未启动，跳过上报与节流簿记（下方诊断日志
    // 保留——本机模式同样需要播放头观察）；纠偏环在本模式下
    // target=current（同源时钟同式推算）delta≈0，DriftController
    // 恒判环内 None，无害保留。
    {
        const bool playing = state() == PlayState::Playing;
        bool due = false;
        if (!cfg_.standalone) {
            std::lock_guard<std::mutex> lock(state_mutex_);
            // 【负差防护 2026-11-16 实测校准卡死 40s】SNTP 校准写入 offset
            // 瞬间对齐时钟整段回拨（实测 -40.2s）→ aligned_now - last_report_
            // 变负 → 原判定不 due → 上报停摆 |offset| 秒，Editor 端视为
            // display 掉线。负差 = 时钟被回拨过的痕迹，强制立即上报并重锚
            // 簿记（对齐新时钟域）。
            const std::int64_t interval = playing ? cfg_.report_interval_us
                                                  : 1'000'000;
            const std::int64_t since = aligned_now_us - last_report_us_;
            if (since < 0 || since >= interval) {
                last_report_us_ = aligned_now_us;
                due = true;
            }
        }
        if (due) sendReport(aligned_now_us);

        // ---- 4. 同步诊断日志（实现计划 P2"同步诊断日志"；周期节流）----
        // 打点内容：播放头 / 纠偏 delta / 纠偏速率——联测与后续面板的数据源。
        // 【stderr 通道】诊断行走 stderr（Windows 无缓冲即时落盘；stdout
        // 重定向文件是全缓冲，长跑联测中无法周期采样——2026-11-03 本机
        // 同步偏差实测时发现的采集障碍）。tsMs 时戳与状态打点同源，
        // 多端日志可按毫秒时戳对齐比较播放头（端间同步偏差测量的基准）。
        // 【diag_interval_us <= 0 = 关闭】display 端 INI 设
        // diag_interval_ms=0 即静默（日常使用无联测需求时的降噪开关）。
        // 【2026-11-05 瞬时卡顿根因修复：锁内快照 + 锁外打印】此前
        // fprintf 在 state_mutex_ 临界区内执行——Windows 控制台 stderr
        // 是同步阻塞 I/O（窗口滚动/控制台忙时可达几十 ms，鼠标选中文本
        // 时甚至无限期挂起 QuickEdit），渲染线程每帧经
        // session_time_us/state() 抢同一把锁读播放头——主线程持锁写
        // 控制台 = 渲染/解码线程锁等待 = 用户实测"瞬时卡顿总出现在
        // 诊断日志打印前后"（每 5s 一次）。与上方 UDP 上报"锁外发送"
        // 同一原则：临界区内绝不做 I/O。
        // 【仅播放期】Idle/Paused 的低频上报已承担在线观测，诊断日志
        // 静默降噪（停止后不再每 5s 刷一条零信息行）。
        if (playing) {
            std::int64_t cur = 0, delta = 0;
            double rate = 1.0;
            std::uint32_t recon = 0;
            bool due = false;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                if (cfg_.diag_interval_us > 0 &&
                    aligned_now_us - last_diag_us_ >= cfg_.diag_interval_us) {
                    last_diag_us_ = aligned_now_us;
                    due = true;
                    cur   = current_us_;
                    delta = last_delta_us_;
                    rate  = rate_;
                    recon = reconnects_;
                }
            }
            if (due) { // 锁外打印：控制台阻塞只拖慢主线程节拍（10ms 循环
                       // 自愈），不再阻塞渲染/解码线程的播放头读取
                std::fprintf(stderr,
                             "[%lld][player:%s] 诊断: t=%lldus delta=%lldus "
                             "rate=%.4f frame=%u 重连=%u\n",
                             tsMs(),
                             cfg_.player_id.c_str(),
                             static_cast<long long>(cur),
                             static_cast<long long>(delta), rate,
                             static_cast<unsigned>(cur / kFrameSlotUs),
                             recon);
            }
        }
    }

    // ---- 5. 状态变化打点（联测观察命令应用时序；stderr 带时戳）----
    // 播放/暂停/停止/seek 冻结等状态机迁移各打一条——与 editor 端重锚
    // 日志同源时戳对齐，可精确测量"命令入队 → display 应用"的延迟。
    {
        const PlayState now_state = state();
        if (now_state != logged_state_) {
            logged_state_ = now_state;
            std::fprintf(stderr,
                         "[%lld][player:%s] 状态: %d t=%lldus\n",
                         tsMs(),
                         cfg_.player_id.c_str(),
                         static_cast<int>(now_state),
                         static_cast<long long>(session_time_us()));
        }
    }

    // ---- 6. 端间时钟校准探测（2026-11-16；详见头文件注释）----
    // 任意状态（Idle/Playing/Paused）都校——时钟差是基础设施问题，
    // 播放中修正（播放头跳到正确位置）比入会前修正更符合用户预期。
    maybeProbeTick(aligned_now_us);
}

// ---------------------------------------------------------------------------
// UDP 上报帧（锁外调用）
// ---------------------------------------------------------------------------
void PlayerRuntime::sendReport(std::int64_t aligned_now_us) {
    SyncFrame f;
    f.role        = SyncRole::Player;
    f.state       = state();
    f.session_id  = session_id();
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        f.session_start_us = session_start_us_;
        f.session_time_us  = current_us_;
        // 【饱和钳制】int64 → int32 窄化：极端失步（>68 年理论值/时钟异常）
        // 直接截断会回绕成反号大偏差，误导 Editor 观测——钳到 int32 极值
        f.delta_us         = static_cast<std::int32_t>(std::clamp(
            last_delta_us_,
            static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()),
            static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())));
        // 帧号由播放头换算（~60fps）：seek 后自然跟随，不需独立计数
        f.frame_index      = static_cast<std::uint32_t>(current_us_ / kFrameSlotUs);
        ++reports_sent_;
    }
    f.wall_us = aligned_now_us; // 对齐时钟（诊断：Editor 可比对其自身读数）
    udp_.send(f); // 失败静默：UDP 无连接，下个周期即重试
}

// ---------------------------------------------------------------------------
// Ack 回发
// ---------------------------------------------------------------------------
void PlayerRuntime::sendAck(std::int64_t anchor_time_us, bool ok,
                            const char* detail) {
    Envelope env;
    auto* ack = env.mutable_ack();
    ack->set_anchor_time_us(anchor_time_us);
    ack->set_ok(ok);
    ack->set_detail(detail);
    // 经 listener_ 单播给已接入的 Editor。失败打日志：联测出现过 Ack
    // 丢失，失败原因需留痕
    const auto cid = editor_conn_.load();
    if (cid != 0) {
        if (!listener_.send_to(cid, env))
            std::fprintf(stderr, "player: Ack 发送失败 cid=%llu\n",
                         (unsigned long long)cid);
    } else {
        std::fprintf(stderr, "player: Ack 未发送：无 Editor 连接\n");
    }
}

// ---------------------------------------------------------------------------
// 素材后台拉取（§5.5 + 推模型自愈，2026-11-04；recv 线程调用 → 内部起线程）
//
// 【自愈语义】拉取不是"一次性机会"：首轮失败的素材（含 404——editor 端
// 注册稍晚是常态瞬态）进入指数退避轮询（2s→4s→8s→10s 封顶），持续重拉
// 直至就绪/场景替换/stop。旧实现失败即放弃、等下一次场景广播——显示端
// 永久占位、用户被迫手动"下发场景"制造重拉（断点分析①）。文件落盘后
// 渲染层 prepare 每帧重试自动接管，画面即时恢复。
// ---------------------------------------------------------------------------
void PlayerRuntime::fetchAssetsAsync(const std::string& http_host,
                                     std::uint16_t http_port) {
    // ---- 前置检查：未启用素材分发 = 视为就绪（无素材恒 true 的语义）----
    if (cfg_.assets_root.empty() || http_port == 0) {
        assets_ready_.store(true);
        return;
    }

    // ---- 拷贝素材表（值语义）+ 依赖过滤：线程只访问拷贝，避免与 scene_
    // 数据竞争（后续场景重发会在锁内覆盖 scene_，而拉取线程生命周期更长）
    // 素材依赖（§5.5 设计补丁）：只拉"clip 绘制矩形 ∩ 本机 screen 矩形
    // ≠ ∅"的素材——跨屏共享素材各端都拉，单屏素材其余端跳过
    std::vector<Asset> list;
    // 【指派模型修正 2026-11-12】过滤以生效 id（effectiveId()，与渲染
    // 选屏/注册帧同源）为准——此前用 cfg_.player_id（本地标签），Editor
    // 指派换 id 后 screen.player_id 绑的是新 id，本过滤恒不命中 → 素材
    // 全被"不在本机依赖内"跳过（画面黑屏）。effectiveId() 内部取
    // state_mutex_，必须在本临界区外先行捕获（同线程嵌套加锁=死锁）。
    const std::string filter_id = effectiveId();
    // 【2026-11-13 性能/锁纪律】① 依赖过滤由线性 find 改为一次性建 set
    // （千级素材场景下原为锁内 O(N²) 字符串比较）；② 被跳过素材的 printf
    // 移出临界区——Windows 控制台 stderr 是同步阻塞 I/O（见本文件
    // QuickEdit 注释），持锁打印会卡渲染线程 sceneSnapshot() 取快照。
    std::vector<std::pair<std::string, std::string>> skipped_diag; // 锁外诊断
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (!scene_) return;
        const auto deps_vec = computeAssetDeps(*scene_, filter_id);
        const std::unordered_set<std::string> deps(deps_vec.begin(),
                                                   deps_vec.end());
        for (const auto& a : scene_->assets) {
            if (deps.count(a.id) != 0)
                list.push_back(a);
            else
                skipped_diag.emplace_back(a.id, a.path);
        }
    }
    // 诊断：列出被依赖过滤跳过的素材（联测可核对多屏各取所需）——锁外打印
    for (const auto& [diag_id, diag_path] : skipped_diag)
        std::printf("player: 素材不在本机依赖内，跳过: %s (%s)\n",
                    diag_id.c_str(), diag_path.c_str());
    const std::size_t total_assets = list.size(); // 本机相关素材总数（含就绪跳过项）
    assets_total_.store(total_assets);
    // 【OSD/进度会话重置 2026-11-16】新清单到达 = 新同步会话开始——
    // assets_ready_ 此前无任何重置点：首轮同步完成后恒 true，第二轮
    // "同步媒体"（新素材加入）时渲染 OSD 的 active 判定恒 false（进度
    // 永不显示）。此处重置；若下方就绪表全命中/无素材路径会立刻重新
    // 置 true（几 ms 窗口，渲染帧间隔 16~500ms，肉眼无感）。
    assets_ready_.store(false);
    if (list.empty()) { // 场景无素材：无需拉取，直接就绪
        // 【2026-11-13 修正】早退前作废并回收旧拉取线程——原逻辑只在新清单
        // 待拉路径接管（bump+join），此早退会让旧循环对已失效清单无限退避
        // 轮询（每轮 stderr+进度上报），且其 assets_ready_cnt_ 写入会覆盖
        // 本路径的口径。
        if (asset_thread_.joinable()) {
            asset_gen_.fetch_add(1); // 通知旧循环退出（轮间休眠 ≤200ms 感知）
            asset_thread_.join();
        }
        assets_ready_.store(true);
        std::printf("player: 场景无素材，跳过拉取\n");
        return;
    }

    // ---- 就绪去重（2026-11-04 手动同步修订）----
    // 跨循环就绪表（asset_done_，拉取成功时写入）命中且身份一致（size +
    // checksum 同）→ 从待拉列表移除。此前就绪表是拉取线程局部变量，每次
    // 新清单/新场景都清零重扫——所有素材重新做本地 CRC（大文件数秒），
    // 用户观感即"反复自动同步"。现在重复触发只做一次 stat 存在性检查
    // （防文件被外部删除后仍被视作就绪），零 CRC 零网络，瞬间就绪。
    // 【持久化恢复 2026-11-09】首次进入时从 .asset_state.tsv 加载跨会话
    // 就绪记录——display 重启后已就绪素材零 CRC 零网络秒就绪（重复同步
    // 修复链的 display 侧；详情见 AssetReadyTable.hpp 头注释）。
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (!ready_table_loaded_) {
            ready_table_loaded_ = true;
            // emplace：内存已有键优先（本会话刚验证过，磁盘快照可能滞后）
            for (auto& kv : loadAssetReadyTable(cfg_.assets_root +
                                                "/.asset_state.tsv"))
                asset_done_.emplace(kv.first, kv.second);
            if (!asset_done_.empty())
                std::printf(
                    "player: 就绪表已恢复: %zu 项（%s）\n",
                    asset_done_.size(), ".asset_state.tsv");
        }
    }
    // 就绪跳过数提升到函数作用域：拉取线程以它为进度基线（ok_total 初值），
    // 保证 assets_ready_cnt_ 与 MsgAssetProgress 的 done/total 口径连续
    std::size_t skipped = 0;
    {
        for (std::size_t idx = list.size(); idx-- > 0;) {
            const auto& a = list[idx];
            std::pair<std::uint64_t, std::string> mark;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                const auto it = asset_done_.find(a.id);
                if (it == asset_done_.end()) continue;
                mark = it->second;
            }
            // 身份不符（素材更新：大小或 CRC 变了）→ 视为未就绪
            if (mark.first != a.size_bytes || mark.second != a.checksum)
                continue;
            // 自愈兜底：本地文件被外部删除/截断 → stat 失败或大小不符，
            // 留在待拉列表（一轮 stat 成本，换文件丢失可感知）。
            // 【2026-11-13 归集】resolve = 归集副本优先（assets/文件名），
            // 缺失回退原绝对路径——两处任一存在且大小一致即视为就绪。
            // 【u8Path 必经】resolve 结果含中文时窄串构造 path 抛
            // std::system_error（error_code 重载救不了）→ worker 内
            // terminate（与「同步媒体」崩溃同根因）。
            std::error_code sec;
            const auto local = std::filesystem::file_size(
                ms::u8Path(assetResolvePath(cfg_.assets_root, a.path)), sec);
            if (sec || local != a.size_bytes) continue;
            list.erase(list.begin() + static_cast<long>(idx));
            ++skipped;
        }
        if (skipped > 0) {
            assets_ready_cnt_.store(assets_total_.load() - list.size());
            // 【刷屏抑制 2026-11-05】"素材去重"行仅在待拉数变化时打印——
            // editor 每次自动下发场景都会走到这里，常态（全部就绪）下
            // skipped/list 恒定，逐次打印即用户看到的"老是打印同步媒体"。
            if (list.size() != last_pending_assets_) {
                std::printf(
                    "player: 素材去重: %zu 项已就绪跳过，待拉 %zu 项\n",
                    skipped, list.size());
                last_pending_assets_ = list.size();
            }
        }
        if (list.empty()) { // 全部命中就绪表：零线程秒过
            // 【2026-11-13 修正】同"场景无素材"路径——早退前作废并回收
            // 旧拉取线程，防旧循环对失效清单无限轮询并覆盖进度口径。
            if (asset_thread_.joinable()) {
                asset_gen_.fetch_add(1);
                asset_thread_.join();
            }
            assets_ready_.store(true);
            // 【2026-11-05 修订】本地进度回调不再触发——本路径没有任何
            // 实际同步动作（零 CRC 零网络零传输），此前每次场景重发都
            // 触发 asset_progress_fn(finished)，display 终端反复打
            // "[媒体同步] 完成 N/N"、窗口进度条反复闪现（用户报
            // "老是打印同步媒体"的根因）。真实拉取完成时的 finished
            // 打印不受影响（那是实际发生的事件）。editor 侧的
            // "同步中"收口仍需 finished 上报，故仅保留下方 send_to。
            // 【进度上报 2026-11-04】本路径不经拉取线程（emit/report 不可
            // 达）——直接补一帧 finished 上报，editor 侧的"同步中"
            // 标注得以收口（就绪表命中是常规路径：场景重发后素材已在盘上）。
            {
                const auto cid = editor_conn_.load();
                if (cid != 0) {
                    Envelope env;
                    auto* m = env.mutable_asset_progress();
                    m->set_done(static_cast<std::uint32_t>(
                        assets_total_.load()));
                    m->set_total(static_cast<std::uint32_t>(
                        assets_total_.load()));
                    m->set_finished(true);
                    listener_.send_to(cid, env);
                }
            }
            return;
        }
    }

    // ---- 作废旧拉取循环 + 接管（场景替换/新清单推送时旧循环让位）----
    if (asset_thread_.joinable()) {
        asset_gen_.fetch_add(1); // 通知旧循环退出（轮间休眠 ≤200ms 感知）
        asset_thread_.join();
    }
    const auto my_gen = asset_gen_.fetch_add(1) + 1; // 本循环的代数

    asset_thread_ = std::thread(
        [this, my_gen, list = std::move(list), http_host, http_port,
         skipped, total_assets]() {
            // 确保落盘根目录存在（含子目录；幂等）
            // 【u8Path 必经】assets_root 配置可含中文——窄串构造 path
            // 遇非法 GBK 序列即抛（worker 线程内 terminate）
            std::error_code ec;
            std::filesystem::create_directories(ms::u8Path(cfg_.assets_root),
                                                ec);
            if (ec)
                std::fprintf(stderr, "player: 素材目录创建失败: %s (%s)\n",
                             cfg_.assets_root.c_str(), ec.message().c_str());

            // ---- 【进度上报 2026-11-04】素材同步进度 → editor -------------
            // 本地 UI 回调（asset_progress_fn）只反映在 display 自己的预览
            // 窗口；editor 端"同步媒体"后看不到各端拉取状态——此处把同一
            // 份进度经 TCP 控制通道回送 editor（MsgAssetProgress，纯观测
            // 帧：丢失无害下一拍覆盖，未连接时静默丢弃）。节流：完成数/
            // 结束态变化立即发，否则 ≥300ms 才发（64KB 分块回调高频，
            // 110MB 文件 1700 次回调直发会打满控制通道）。
            auto steady_ms = [] {
                return std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now().time_since_epoch())
                    .count();
            };
            std::int64_t last_report_ms  = -1'000'000; // 首帧立即放行
            std::size_t  last_report_done = (std::size_t)-1;
            bool         last_report_fin  = true;
            // 【worker 池 2026-11-09】report 节流状态会被多 worker 并发触碰
            // → 锁在 emit 一层（report 仅由 emit 持锁调用；std::mutex 非递归，
            // report 再加锁即自死锁——首版即踩，T1 直接挂死 terminate）
            std::mutex emit_mx;
            auto report = [&](std::size_t done, const std::string& path,
                              std::uint64_t bytes, std::uint64_t total,
                              bool finished) {
                const std::int64_t now = steady_ms();
                const bool changed =
                    (done != last_report_done) || (finished != last_report_fin);
                if (!finished && !changed &&
                    now - last_report_ms < 300)
                    return; // 节流窗口内：跳过（分块进度合并）
                last_report_ms   = now;
                last_report_done = done;
                last_report_fin  = finished;
                // 【诊断 MS_OSD_DBG=1】上报发送打点（链路自证第一段）
                {
                    static const bool s_osd_dbg =
                        std::getenv("MS_OSD_DBG") != nullptr;
                    if (s_osd_dbg)
                        std::fprintf(stderr,
                                     "osd: report: done=%zu total=%zu "
                                     "fin=%d bytes=%llu/%llu\n",
                                     done, total_assets, finished ? 1 : 0,
                                     static_cast<unsigned long long>(bytes),
                                     static_cast<unsigned long long>(total));
                }
                const auto cid = editor_conn_.load();
                if (cid == 0) return; // 尚无 editor 连接：无从上报
                Envelope env;
                auto* m = env.mutable_asset_progress();
                m->set_done(static_cast<std::uint32_t>(done));
                // 【口径统一 2026-11-09】分母 = 本机相关素材总数（含就绪
                // 跳过项）。此前用待拉 list.size()，与全跳过路径上报的
                // assets_total_ 冲突，editor 端 N/M/P% 会跳变回退
                m->set_total(static_cast<std::uint32_t>(total_assets));
                m->set_current_path(path);
                m->set_current_bytes(bytes);
                m->set_current_total(total);
                m->set_finished(finished);
                listener_.send_to(cid, env); // 失败静默（尽力而为观测帧）
            };

            // ---- 进度上报（"同步媒体"可视化；回调空 = 零开销）----
            // 线程约定：本回调在拉取线程上下文同步调用，订阅方（显示端
            // 预览窗口/控制台）自行加锁快照——本层不存进度状态。
            const bool has_cb = static_cast<bool>(cfg_.asset_progress_fn);
            auto emit = [&](std::size_t done, const std::string& path,
                            std::uint64_t bytes, std::uint64_t total,
                            bool finished) {
                std::lock_guard<std::mutex> lk(emit_mx);
                report(done, path, bytes, total, finished); // → editor（节流）
                // 【渲染 OSD 2026-11-16】当前文件字节百分比（渲染线程采样）
                asset_file_percent_.store(
                    finished || total == 0
                        ? -1
                        : static_cast<int>(std::min<std::uint64_t>(
                              100, bytes * 100 / total)),
                    std::memory_order_relaxed);
                if (!has_cb) return;
                AssetProgress p;
                p.done          = done;
                p.total         = total_assets;
                p.current_path  = path;
                p.current_bytes = bytes;
                p.current_total = total;
                p.finished      = finished;
                cfg_.asset_progress_fn(p);
            };

            // AssetReceiver：每 worker 各自局部构造（无状态类，见 hpp）——
            // 不再持有线程级共享实例（worker 池 2026-11-09）

            // ---- 孤儿 part 清理【2026-11-04】--------------------------------
            // ".part-<pid>" 按进程隔离命名，进程崩溃/被杀后旧 PID 的 part
            // 文件永成孤儿残件（联测实测 media/ 下 9 个 .part 累计约 7.9GB）。
            // 此处清扫 assets_root 下所有 "*.part-<纯数字>" 且最后写入时间
            // 超过 1 小时的文件：正在写的 part 每次追加都会刷新 mtime，本
            // 实例与其他实例的活跃下载都不会被误删；停摆 1 小时的下载本身
            // 也已无续传价值（轮询重试早放弃了）。
            {
                const auto cutoff =
                    std::filesystem::file_time_type::clock::now() -
                    std::chrono::hours(1);
                std::error_code scan_ec;
                // 【u8Path 必经】assets_root 可含中文（同上抛异常风险）
                std::filesystem::directory_iterator it(
                    ms::u8Path(cfg_.assets_root), scan_ec);
                if (!scan_ec) {
                    for (const auto& entry : it) {
                        std::error_code fe;
                        if (!entry.is_regular_file(fe) || fe) continue;
                        // u8string → 窄串按字节直拷（UTF-8）——string()
                        // 走 ACP 宽窄转换，GBK 外字符（生僻字）直接抛
                        const auto u8name =
                            entry.path().filename().u8string();
                        const std::string name(
                            reinterpret_cast<const char*>(u8name.data()),
                            u8name.size());
                        // part 命名 = save_path + ".part-" + pid（save_path
                        // 与 part 同目录）→ 文件名以 ".part-<纯数字>" 结尾
                        const std::size_t mark = name.rfind(".part-");
                        if (mark == std::string::npos) continue;
                        const std::string suffix = name.substr(mark + 6);
                        if (suffix.empty() ||
                            !std::all_of(suffix.begin(), suffix.end(),
                                         [](unsigned char c) {
                                             return std::isdigit(c) != 0;
                                         }))
                            continue; // 非 ".part-<pid>" 模式，不碰
                        std::error_code te;
                        const auto mt = entry.last_write_time(te);
                        if (te || mt > cutoff) continue; // 活跃下载：跳过
                        const std::uintmax_t bytes = entry.file_size(fe);
                        std::error_code rm_ec;
                        std::filesystem::remove(entry.path(), rm_ec);
                        if (!rm_ec)
                            std::fprintf(stderr,
                                         "player: 清理孤儿下载残件: %s"
                                         "（%.1f MB）\n",
                                         name.c_str(),
                                         static_cast<double>(bytes) /
                                             1048576.0);
                    }
                }
            }

            // ---- 同路径折叠（worker 池并发安全前提，2026-11-09）----------
            // 同名文件多次导入会生成多个 asset id 指向同一 path（同源文件
            // → size/checksum 全同）。worker 池并发后两个 worker 同时 fetch
            // 同一 save 路径会互踩 ".part-<pid>" 残件——同 path 只保留首个
            // 为"代表"进拉取（reps 内 path 唯一），其余作跟随项：代表成功
            // 即整组记就绪（文件只有一份，跟随项零传输）。
            std::vector<std::size_t> reps; // 待拉代表下标（path 唯一）
            std::map<std::string, std::vector<std::size_t>> followers;
            {
                std::map<std::string, std::size_t> first_of_path;
                for (std::size_t i = 0; i < list.size(); ++i) {
                    const auto ins = first_of_path.emplace(list[i].path, i);
                    if (ins.second)
                        reps.push_back(i);
                    else
                        followers[list[i].path].push_back(i);
                }
            }
            // 完成标志（原子槽位：多 worker 各写各的槽，无数据竞争）。
            // 【原语义保留】true = 该代表已落盘且校验通过，后续轮次跳过
            std::vector<std::atomic<char>> done(reps.size());
            for (auto& d : done) d.store(0, std::memory_order_relaxed);
            // 全局完成计数（含就绪跳过基线 + 跟随折叠增益；worker 原子累加）。
            // 【计数修复 2026-11-09】基线 = 就绪跳过数——此前从 0 起算，
            // store 会覆盖去重段预置的跳过计数，进度倒退（2/3 → 1/3 → 2/3）
            std::atomic<std::size_t> ok_total{skipped};
            // ---- 轮循环：每轮把未就绪代表任务交给 worker 池并行拉取 ------
            // 【worker 池 2026-11-09】对照 WATCHOUT Runner 的 2 并发 worker
            // ——大素材并行拉取显著缩短首轮同步时长。结构：本线程兼任 0 号
            // worker（省一线程）+ N-1 个子 worker，轮内竞争原子游标取任务；
            // join 后轮末统一落盘/判定/退避（自愈轮语义不变）。
            // 就绪表落盘收口（锁内快照、锁外原子写；多 worker 并发成功时
            // 集中此处串行落盘，避免并发写同一文件互相覆盖）
            auto flush_ready_table = [&]() {
                AssetReadyTable snap;
                {
                    std::lock_guard<std::mutex> lk(state_mutex_);
                    snap = asset_done_;
                }
                saveAssetReadyTable(cfg_.assets_root + "/.asset_state.tsv",
                                    snap);
            };
            for (unsigned round = 0;; ++round) {
                std::atomic<std::size_t> cursor{0}; // 本轮任务游标（竞争取）
                std::mutex fail_mx;
                std::string round_failed; // 本轮失败清单（轮汇总日志，降噪）

                // ---- 单 worker 主体：竞争取代表任务 → 3×500ms 短重试拉取
                auto worker_body = [&](AssetReceiver& rx) {
                    for (;;) {
                        const std::size_t pos =
                            cursor.fetch_add(1, std::memory_order_relaxed);
                        if (pos >= reps.size()) return;
                        const std::size_t idx = reps[pos];
                        const auto& a = list[idx];
                        // done 语义（2026-11-09）：全局完成计数真值（含跳过
                        // 与跟随折叠），多 worker 并发下比旧"下标+1"近似准确
                        emit(ok_total.load(std::memory_order_relaxed), a.path,
                             0, a.size_bytes, false);
                        // 【2026-11-13 归集】保存落点 = assets_root/文件名
                        //（绝对路径素材不再写回原绝对路径——原实现跨机时
                        // 会把下载写到与编辑器相同的绝对路径，路径不一致
                        // 的机器上必然失败；display 目录自此自含分发）
                        const std::string save =
                            assetStorePath(cfg_.assets_root, a.path);
                        // 短重试（网络抖动）：半成品保留，Range 续传；404 交给
                        // 外层自愈轮（注册稍晚窗口 > 1.5s 很常见）。
                        // 【取消注入 round7 S1-1】新清单/场景接管或 stop 时
                        // fetch 内部即刻放弃（≤64KB 内返回）——recv 线程的
                        // join 不再被 GB 级下载剩余时长卡死（原实现 join
                        // 冻结 recv 线程数十秒，SEEK/PAUSE/快照全部排队）。
                        AssetReceiver::Result r =
                            AssetReceiver::Result::NetworkError;
                        AssetReceiver::Stats st;
                        for (int attempt = 1; attempt <= 3; ++attempt) {
                            st = {};
                            r = rx.fetch(
                                http_host, http_port, a.id, save, a.checksum,
                                &st,
                                [&emit, &a, &ok_total](
                                    std::uint64_t now, std::uint64_t total) {
                                    emit(ok_total.load(
                                             std::memory_order_relaxed),
                                         a.path, now, total, false);
                                },
                                [this, my_gen]() {
                                    return asset_gen_.load() != my_gen ||
                                           asset_quit_.load();
                                });
                            if (r == AssetReceiver::Result::Ok ||
                                r == AssetReceiver::Result::AlreadyHave)
                                break;
                            if (r == AssetReceiver::Result::Cancelled ||
                                asset_gen_.load() != my_gen ||
                                asset_quit_.load())
                                return; // 被新循环/stop 接管（join 收齐后统一让位）
                            if (attempt < 3)
                                std::this_thread::sleep_for(
                                    std::chrono::milliseconds(500));
                        }
                        if (r == AssetReceiver::Result::Ok ||
                            r == AssetReceiver::Result::AlreadyHave) {
                            done[pos].store(1, std::memory_order_relaxed);
                            // 【就绪表写入】跨循环持久——后续清单/场景重发
                            // 触发的拉取检查命中即跳过（不重下载不重 CRC）。
                            // 记录身份（size+checksum）：素材更新时身份不符
                            // 自动失效。同 path 跟随项随代表一并记就绪
                            // （同文件同身份，无需传输）。
                            std::size_t gain = 1;
                            {
                                std::lock_guard<std::mutex> lk(state_mutex_);
                                asset_done_[a.id] = {a.size_bytes, a.checksum};
                                const auto fit = followers.find(a.path);
                                if (fit != followers.end())
                                    for (std::size_t f : fit->second) {
                                        asset_done_[list[f].id] = {
                                            a.size_bytes, a.checksum};
                                        ++gain;
                                    }
                            }
                            const std::size_t done_now =
                                ok_total.fetch_add(
                                    gain, std::memory_order_relaxed) +
                                gain;
                            assets_ready_cnt_.store(done_now,
                                                    std::memory_order_relaxed);
                            std::printf(
                                "player: 素材就绪 %zu/%zu: %s → %s (%llu 字节%s)\n",
                                done_now, total_assets, a.id.c_str(),
                                save.c_str(),
                                static_cast<unsigned long long>(
                                    st.bytes_written ? st.bytes_written
                                                     : a.size_bytes),
                                st.idempotent ? ",幂等命中"
                                              : (st.resumed ? ",断点续传"
                                                            : ""));
                        } else {
                            std::lock_guard<std::mutex> lk(fail_mx);
                            if (!round_failed.empty()) round_failed += ", ";
                            round_failed += a.id;
                        }
                        emit(ok_total.load(std::memory_order_relaxed), a.path,
                             0, 0, false);
                    }
                };

                // ---- 起池：workers-1 个子线程 + 本线程兼任 0 号 worker ----
                {
                    const unsigned workers = static_cast<unsigned>(
                        (std::max)(1, (std::min)(cfg_.asset_workers,
                                                 static_cast<int>(reps.size()))));
                    std::vector<std::thread> pool;
                    pool.reserve(workers - 1);
                    for (unsigned w = 1; w < workers; ++w)
                        pool.emplace_back([&]() {
                            AssetReceiver rx; // 每 worker 独立实例（无共享状态）
                            worker_body(rx);
                        });
                    AssetReceiver rx0;
                    worker_body(rx0);
                    for (auto& t : pool) t.join();
                }

                // 接管/停止：让位（本轮已成功的记录落盘，别丢）
                if (asset_gen_.load() != my_gen || asset_quit_.load()) {
                    flush_ready_table();
                    return;
                }

                flush_ready_table(); // 轮末统一落盘（成功项；原子 tmp+rename）

                // 完成判定：全部代表就绪（跟随项随代表自动就绪）
                std::size_t ok_cnt = 0;
                for (const auto& d : done)
                    ok_cnt += d.load(std::memory_order_relaxed);
                if (ok_cnt == reps.size()) {
                    assets_ready_.store(true);
                    std::printf("player: 全部素材就绪: %zu/%zu\n",
                                ok_total.load(std::memory_order_relaxed),
                                total_assets);
                    emit(total_assets, {}, 0, 0, true); // 含跳过项，口径统一
                    return;
                }
                // ---- 轮间退避（2s→4s→8s→10s 封顶）+ 快速退出检查 ----
                // 404（editor 端注册稍晚/文件暂缺）也纳入重试——文件在
                // editor 侧补注册后，下一轮自然拉到（无需任何手动触发）
                const int wait_ms =
                    std::min(2000 * (1 << std::min<unsigned>(round, 3)), 10000);
                std::fprintf(stderr,
                             "player: 素材未全部就绪 %zu/%zu（第 %u 轮自愈，"
                             "%dms 后重试: %s）\n",
                             ok_total.load(std::memory_order_relaxed),
                             total_assets, round + 1, wait_ms,
                             round_failed.c_str());
                for (int slept = 0; slept < wait_ms; slept += 200) {
                    if (asset_gen_.load() != my_gen ||
                        asset_quit_.load())
                        return; // 新清单/场景接管 或 停止（join 快速返回）
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(200));
                }
            }
        });
}

// ---------------------------------------------------------------------------
// 状态观测（全部加锁快照）
// ---------------------------------------------------------------------------
PlayState PlayerRuntime::state() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_;
}

std::int64_t PlayerRuntime::session_time_us() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_us_;
}

double PlayerRuntime::rate() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return rate_;
}

std::uint32_t PlayerRuntime::frame_index() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return static_cast<std::uint32_t>(current_us_ / kFrameSlotUs);
}

std::string PlayerRuntime::session_id() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return session_id_;
}

std::int64_t PlayerRuntime::last_delta_us() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return last_delta_us_;
}

// ---------------------------------------------------------------------------
// 渲染线程输入快照（渲染线程每帧调用；一次锁拷贝全部所需状态）
// ---------------------------------------------------------------------------
std::shared_ptr<const Scene> PlayerRuntime::sceneSnapshot() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return scene_; // shared_ptr 拷贝（原子引用计数），内容免拷贝
}

std::uint64_t PlayerRuntime::sceneSerial() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return scene_serial_;
}

RenderThread::Playhead PlayerRuntime::playheadSnapshot() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    RenderThread::Playhead ph;
    ph.state             = state_;
    ph.session_start_us  = session_start_us_;
    ph.session_time_us   = current_us_;
    return ph;
}

std::int64_t PlayerRuntime::renderClockNow() const {
    // 测试注入的时钟优先（单测使渲染线程与 tick 驱动时间同域）；
    // 运行期用内部复合时钟（monotonic + NTP offset，§4.4）
    if (cfg_.clock_fn) return cfg_.clock_fn();
    return clock_.alignedNowUs();
}

} // namespace ms
