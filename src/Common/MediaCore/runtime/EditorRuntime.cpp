// ============================================================================
//  EditorRuntime.cpp —— Editor 侧运行时实现
//
//  锚点计算（§6.1，全场同步的根基）：
//      anchor_time = now + lead           （全场约定的"未来应用时刻"）
//      anchored_session_time = 本次命令的播放头目标
//  广播后各 Player 在各自的锚点调度器里等到 aligned_now >= anchor_time
//  才应用 session_start = anchor_time - anchored_session_time——
//  网络延迟不同、到达时刻不同都无关紧要，应用时刻由共享时钟统一。
//
//  会话状态迁移（Editor 侧权威）：
//      Idle  --play-->  Playing --pause--> Paused --play--> Playing
//      Playing/Paused --seek--> 同状态跳转        任意 --stop--> Idle
//    session_start 仅在 play（从 0 或暂停点续播）与 seek（重设时间基）
//    时重算；pause 不改 session_start（冻结语义 = 播放头停走）。
// ============================================================================

#include "runtime/EditorRuntime.hpp"

#include <chrono>
#include <filesystem>

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "clock/ClockProvider.hpp"
#include "live/LiveHub.hpp" // Live 分发归一化（display 收 Hub 再发布流）
#include "runtime/AssetPath.hpp" // u8Path：中文素材路径防御（2026-11-13）
// （第七迭代：增量补丁机制退役——场景同步一律广播全量，无需 diff/merge）

namespace ms {

// 【排障】stderr 时戳（毫秒）：与 net 层诊断日志对齐时间线
namespace {
long long tsMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
/// 素材寻址（2026-11-12 Flow 编辑器接入）：Asset.path 为绝对路径时直通
///（编辑器空素材根语义——导入探测 probeFile 写入的寻址路径）；
/// 相对路径拼 assets_root（standalone/.show 语义，'/' 分隔）。
std::string assetFullPath(const std::string& assets_root,
                          const std::string& path) {
    if (path.size() >= 2 && path[1] == ':') return path; // Windows 盘符
    if (!path.empty() && (path[0] == '/' || path[0] == '\\'))
        return path;                                     // POSIX / UNC
    if (assets_root.empty()) return path;
    return assets_root + "/" + path;
}
} // namespace

using ms_proto::Envelope;

// ---------------------------------------------------------------------------
// 构造 / 生命周期
// ---------------------------------------------------------------------------
EditorRuntime::EditorRuntime(Config cfg) : cfg_(cfg) {}

bool EditorRuntime::start(const std::string& scene_path) {
    // ---- 1. 场景加载（可选；失败即整体启动失败——控制权威必须有权威数据）----
    // 双格式（第五迭代）：.show = 完整项目（场景 + 显示端登记）；
    // .scene = 纯场景（兼容旧文件/命令行联测脚本）。
    if (!scene_path.empty()) {
        const bool is_show = scene_path.size() > 5 &&
                             scene_path.compare(scene_path.size() - 5, 5,
                                                ".show") == 0;
        if (is_show) {
            std::string err;
            auto doc = ShowFileIO::loadFromFile(scene_path, err);
            if (!doc) {
                std::fprintf(stderr, "[editor] 项目加载失败: %s\n",
                             err.c_str());
                return false;
            }
            scene_ = std::move(doc->scene);
            // 显示端登记 → DisplayConnector（连接由看门狗异步发起；
            // 接入后显示端上报注册，经 handleRegister 走统一下发路径）
            for (const auto& d : doc->displays) {
                if (!d.enabled) continue;
                if (!addDisplay(d.display_id, d.host, d.port,
                                d.pairing_code))
                    std::fprintf(stderr,
                                 "[editor] 显示端登记冲突: %s（跳过）\n",
                                 d.display_id.c_str());
            }
            std::printf("[editor] 项目已加载: %s（%zu 屏 / %zu 轨 / %zu 显示端）\n",
                        scene_path.c_str(), scene_->screens.size(),
                        scene_->timeline.tracks.size(),
                        doc->displays.size());
        } else {
            std::string err;
            auto scene = SceneLoader::loadFromFile(scene_path, err);
            if (!scene) {
                std::fprintf(stderr, "[editor] 场景加载失败: %s\n",
                             err.c_str());
                return false;
            }
            scene_ = std::move(scene);
            std::printf("[editor] 场景已加载: %s（%zu 屏 / %zu 轨）\n",
                        scene_path.c_str(), scene_->screens.size(),
                        scene_->timeline.tracks.size());
        }
    }

    // ---- 2. UDP 心跳（收 Player 上报 + 发单播心跳）----
    // 【2026-11-03 兼容通道退役】Editor 不再监听 TCP 入站——display 一律
    // 由 DisplayConnector 主动连接（WATCHOUT 模型），TCP 入站监听已成
    // 死路径（display 入口硬编码 listen_mode=true），连同 Config.tcp_port
    // 一并移除。
    if (!udp_.start(cfg_.udp_port)) {
        std::fprintf(stderr, "[editor] UDP 绑定失败\n");
        return false;
    }
    udp_.set_on_frame([this](const SyncFrame& f,
                             const Poco::Net::SocketAddress& from) {
        onUdpFrame(f, from);
    });

    // ---- 2.5 素材 HTTP 分发（assets_root 非空即启用监听）----
    // Asset.path 为相对路径（'/' 分隔）→ 拼接到 assets_root 下。
    // 注册失败的素材打警告但不阻断启动（Player 端拉取会 404 并打日志）。
    // 【GUI 模式兜底（裸启动无素材修复）】此前启动条件带 `&& scene_`——
    // GUI 模式启动时场景为空（素材是运行中添加的）→ assets_http_ 永不
    // 创建 → Ack 报 http_port=0、cmdSyncMedia 空转，显示端无从拉取素材。
    // 现改为只要素材根目录非空就起监听；素材注册延后到"同步媒体"/"场景
    // 发布"（cmdPublishScene 已改为发布即幂等注册），启动时无场景不再
    // 意味着分发能力缺席。
    // 【顺序约束 2026-11-04】必须先于 displays_.start()：display 注册的
    // Ack 里携带 http_port 且【只在注册时发一次】——若 display 在素材
    // HTTP 就绪前注册，Ack 报 0 且再无机会更正（display 端视为"未启用
    // 分发"永久跳过拉取）。素材服务先起，注册连接拿到的一定是真实端口。
    if (!cfg_.assets_root.empty()) {
        assets_http_ = std::make_unique<AssetHttpServer>();
        // CRC 缓存持久化（先于任何注册——加载只并入内存表缺失键，越早越省）
        // editor 重启后零重算：场景/清单从第一帧起带真值 checksum，
        // 显示端不再因"空 checksum 窗口"重传已就绪文件（重复同步修复链①）
        assets_http_->enableCacheFile(cfg_.assets_root + "/.crc_cache.tsv");
        if (!assets_http_->start(cfg_.http_port)) {
            std::fprintf(stderr, "[editor] 素材 HTTP 监听失败 port=%u\n",
                         cfg_.http_port);
            udp_.stop();
            return false;
        }
        std::size_t ok_cnt = 0;
        std::size_t total  = 0;
        if (scene_) {
            // start 阶段 server 尚未启动（无并发访问 scene_），可直接回填
            for (auto& a : scene_->assets) {
                // Text 素材无文件（text_content 随场景 pb 下发），跳过注册
                if (a.kind == AssetKind::Text) continue;
                ++total;
                // std::filesystem 兼容 '/' 分隔（Windows 也接受）；
                // 绝对路径直通（Flow 编辑器空素材根语义）
                const std::string full =
                    assetFullPath(cfg_.assets_root, a.path);
                if (assets_http_->registerAsset(a.id, full)) {
                    ++ok_cnt;
                    // 【注册表权威·无脑覆盖】场景随下发携带的 checksum 一律
                    // 以注册表（当前磁盘文件）为准：CRC 未算完（后台任务）
                    // 时【清空】而不是保留场景旧值——过期 CRC 会让显示端
                    // 拉取后校验必败（CrcMismatch），空值只是降级"只比长度"。
                    AssetHttpServer::AssetEntry info;
                    if (assets_http_->assetInfo(a.id, info) &&
                        !info.crc32_hex.empty())
                        a.checksum = "crc32:" + info.crc32_hex;
                    else
                        a.checksum.clear(); // CRC 未就绪：宁可空值不过期值
                } else
                    std::fprintf(stderr,
                                 "[editor] 警告: 素材文件缺失 %s (id=%s)\n",
                                 full.c_str(), a.id.c_str());
            }
        }
        std::printf(
            "[editor] 素材分发: http=%u 注册 %zu/%zu 个%s\n",
            assets_http_->port(), ok_cnt, total,
            scene_ ? "（Text 素材随场景下发不占文件）"
                   : "（启动时无场景，待发布场景/同步媒体时注册）");

        // ---- CRC 就绪 → 防抖自动补推清单（2026-11-09，重复同步修复链③）----
        // on_crc_ready 在 CRC 后台线程触发，这里只置位事件（微秒级）；
        // 快照/注册重探/推送全部在专职防抖线程（crcPushLoop）
        assets_http_->on_crc_ready = [this] {
            {
                std::lock_guard<std::mutex> lock(crc_push_mutex_);
                crc_push_pending_ = true;
            }
            crc_push_cv_.notify_all();
        };
        crc_push_thread_ = std::thread([this] { crcPushLoop(); });
    }

    // ---- 3. 显示端主动连接器（第五迭代）：回调装配 + 看门狗启动 ----
    // 入站消息（display 上报的 Register/Ack）走
    // handleRegister/handleAck 业务语义；连接状态变化只打日志（UI 经
    // displayEntries() 轮询刷新，避免跨线程触碰 Qt）。
    // 【顺序约束】见上：素材 HTTP 必须先就位，display 注册 Ack 才能带
    // 有效 http_port（Ack 只在注册时发一次，错过即永久 0）。
    displays_.set_on_envelope([this](const std::string& did,
                                     const Envelope& env) {
        onDisplayEnvelope(did, env);
    });
    displays_.set_on_state([this](const std::string& did, bool up) {
        onDisplayState(did, up);
    });
    displays_.start();

    // ---- 4. 出站发送线程（GUI/recv 与网络解耦，2026-11-03）----
    // 所有出站帧（广播/单发/心跳）经队列由本线程投递：cmd*/tick/应答闭包
    // 只入队（微秒级），socket 阻塞写全部离场 GUI（见 sendWorkerLoop）。
    send_thread_ = std::thread([this] { sendWorkerLoop(); });

    std::printf("[editor] 已启动: udp=%u 显示端=登记制 场景=%s\n",
                udp_.port(),
                scene_ ? "已加载" : "无");
    return true;
}

void EditorRuntime::stop() {
    // 【停止顺序关键】先终结所有会回调进本对象的网络线程，最后拆素材
    // 服务——displays_/udp_ 的 recv 线程回调（handleRegister 读
    // assets_http_->port() / onUdpFrame）会解引用 assets_http_，若先
    // reset 该指针而 recv 线程仍在跑，即是 use-after-free。
    {
        // -1) 先停 CRC 防抖补推线程（引用 assets_http_ 与发送队列，最先行）
        crc_push_quit_.store(true);
        crc_push_cv_.notify_all();
        if (crc_push_thread_.joinable()) crc_push_thread_.join();
    }
    {
        // 0) 先停发送线程：它引用 displays_/udp_ 的 socket，
        //    必须在这些组件析构/停用前安静（置退出标志 + 唤醒 + join）
        {
            std::lock_guard<std::mutex> lock(send_mutex_);
            send_quit_ = true;
        }
        send_cv_.notify_all();
        if (send_thread_.joinable()) send_thread_.join();
    }
    displays_.stop(); // 1) 显示端连接器（其回调引用本对象其他成员）
    udp_.stop();      // 2) 终结 UDP recv 线程（onUdpFrame）
    if (assets_http_) {
        assets_http_->stop(); // 4) 线程全部安静后再停 HTTP 分发并释放
        assets_http_.reset();
    }
}

void EditorRuntime::setScene(const Scene& scene) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    scene_ = scene;
}

// ---------------------------------------------------------------------------
// 显示端管理（第五迭代：editor 主动连接显示端，WATCHOUT 模型）
// ---------------------------------------------------------------------------
bool EditorRuntime::addDisplay(const std::string& display_id,
                               const std::string& host, std::uint16_t port,
                               const std::string& pairing_code) {
    return displays_.add(display_id, host, port, pairing_code);
}

bool EditorRuntime::removeDisplay(const std::string& display_id) {
    // 注销连接器槽位 + 清理注册表观测条目（UDP 反查表条目无害保留：
    // display 重新启用注册时会按新地址覆盖）
    if (!displays_.remove(display_id)) return false;
    std::lock_guard<std::mutex> lock(state_mutex_);
    for (auto it = players_.begin(); it != players_.end(); ++it) {
        if (it->second.display_id == display_id) {
            players_.erase(it);
            break;
        }
    }
    return true;
}

std::vector<DisplayEntry> EditorRuntime::displayEntries() const {
    // DisplayConnector::Entry（含运行状态）→ ms::DisplayEntry（登记数据，
    // .show 保存用）：取登记四字段，运行状态由 UI 另查 displays_online()
    std::vector<DisplayEntry> out;
    for (const auto& e : displays_.snapshot()) {
        DisplayEntry d;
        d.display_id = e.display_id;
        d.host       = e.host;
        d.port       = e.port;
        d.enabled    = e.enabled;
        d.pairing_code = e.pairing_code;
        out.push_back(std::move(d));
    }
    return out;
}

/// 全量场景重发（第五迭代）：对全部在线显示端重发当前权威场景
/// （一次广播入队——快照 1 份、serialize 1 次，投递逐台在线槽位）。
/// 用途：新项目加载/新建后（场景 id 或画布变更，不可增量）让已接入的
/// 显示端整体切换到新项目内容，无需等显示端重新注册。
/// 全量场景重发（第五迭代）：对全部在线显示端重发当前权威场景。
/// 用途：新项目加载/新建后（场景 id 或画布变更，不可增量）让已接入的
/// 显示端整体切换到新项目内容，无需等显示端重新注册。
std::size_t EditorRuntime::resendSceneToDisplays() {
    std::size_t online = 0;
    for (const auto& e : displays_.snapshot())
        if (e.connected && e.enabled) ++online;
    if (online == 0) return 0;
    // 【与 GUI 分离 2026-11-04】【广播合一 2026-11-04】一次入队广播
    // ScenePrepare（display_id 空）：快照深拷贝 1 份、serialize 1 次，
    // 投递由 displays_.broadcast 内部快照在线槽位逐台发送。原先逐台
    // 入队等价于 N 份全场景深拷贝 + N 次 serialize，纯浪费（ScenePrepare
    // 入队不会失败，返回值语义与逐台版一致：online 台全部已提交）。
    Scene snap;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (!scene_) return 0; // 无场景：无事可做
        snap = *scene_;
    }
    SendTask t;
    t.kind = SendTask::Kind::ScenePrepare; // display_id 空 = 广播在线槽位
    t.scene = std::move(snap);
    enqueueSend(std::move(t));
    std::printf("[editor] 全量场景重发 → 已提交 %zu 个在线显示端"
                "（发送线程异步投递）\n", online);
    return online;
}

// ---------------------------------------------------------------------------
// 显示端入站（DisplayConnector recv/看门狗线程上下文）
// ---------------------------------------------------------------------------
void EditorRuntime::onDisplayEnvelope(const std::string& display_id,
                                      const Envelope& env) {
    // 应答闭包 = 入队 DisplayUnicast（socket 写在发送线程；recv 线程零阻塞）
    const auto reply = [this, display_id](const Envelope& out) {
        SendTask t;
        t.kind       = SendTask::Kind::DisplayUnicast;
        t.display_id = display_id;
        t.env        = out;
        enqueueSend(std::move(t));
    };

    // ---- 显示端的重锚/补丁确认 ----
    if (env.has_ack()) {
        handleAck(env.ack(), reply);
        return;
    }

    // ---- 【素材同步进度 2026-11-04】纯观测帧：存快照供场景树标注 ----
    if (env.has_asset_progress()) {
        handleAssetProgress(display_id, env.asset_progress());
        return;
    }

    // ---- 注册（显示端被接入后立即上报；§6.2 语义复用）----
    if (!env.has_register_()) return;
    const auto& reg = env.register_();
    // 【指派模型 2026-11-06】任意注册帧即置位"已注册"——身份不再由
    // display 自报对账，而是 Editor 在 Ack 中指派（assigned_player_id =
    // 槽位 id）；display 采用后以指派 id 重新注册（走同一路径自然换键）。
    // 自报 id ≠ 槽位只是提示信息，不是错误：同一块屏接了谁就用谁。
    displays_.setRegistered(display_id, true);
    if (reg.player_id() != display_id)
        std::fprintf(stderr,
                     "[editor] 显示端身份指派: 槽位=%s 注册自报=%s"
                     " → Ack 下发指派 id，display 采用后自动换 id 重注册\n",
                     display_id.c_str(), reg.player_id().c_str());
    // Ack.udp_host：Editor 侧本端 IP（DisplayConnector 连接成功时从本地
    // socket 学到；同机联测 127.0.0.1，跨机为编辑器所在网卡 IP）。
    // 空串回退 127.0.0.1（本机部署的最低可用值）。
    std::string ack_host = displays_.localHostOf(display_id);
    if (ack_host.empty()) ack_host = "127.0.0.1";
    handleRegister(reg.player_id(), reg.udp_report_port(),
                   displays_.peerHostOf(display_id), ack_host,
                   display_id, reply);
}

void EditorRuntime::onDisplayState(const std::string& display_id, bool up) {
    // 回调在 Connector 线程上下文：只打日志 + 维护注册表哨兵，
    // UI 经 displayEntries() 轮询刷新（禁止跨线程触碰 Qt）
    if (!up) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        for (auto& [pid, e] : players_) {
            if (e.display_id == display_id && e.conn_id != 0) {
                e.conn_id = 0; // 在线哨兵清零（registered_count 立即回落）
                break;
            }
        }
        // 【素材同步进度 2026-11-04】断连即清该端进度标注——离线端不再
        // 显示"同步中"（重连后新清单/重发场景会重新上报）
        asset_sync_.erase(display_id);
    }
    // 【走 stderr】连接状态变化是关键事件，不能被 stdout 全缓冲延迟
    std::fprintf(stderr, "[editor] 显示端 %s: %s\n", display_id.c_str(),
                 up ? "已连接（等待其上报注册）" : "连接断开");
    std::fflush(stderr);
}

// ---------------------------------------------------------------------------
// 注册公共处理（display 通道；recv 线程上下文）
// ---------------------------------------------------------------------------
void EditorRuntime::handleRegister(
    const std::string& pid, std::uint32_t udp_report_port,
    const std::string& player_udp_host, const std::string& ack_udp_host,
    const std::string& display_slot,
    const std::function<void(const Envelope&)>& reply) {
    if (pid.empty()) {
        // 无效注册：拒绝（Player 收到 accepted=false 会打日志）
        Envelope out;
        auto* ack = out.mutable_register_ack();
        ack->set_accepted(false);
        ack->set_reason("empty player_id");
        reply(out);
        return;
    }

    // 会话分配：首个注册者建立会话，后续注册者沿用（迟到位走心跳软对齐）。
    // 【注意】必须把会话号写回 session_id_，否则每个 Player 各得独立会话号。
    std::string sid;
    bool is_reconnect = false; // 重连识别（§7.1④：同一 player_id 二次注册）
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (session_id_.empty())
            session_id_ = "s-" + std::to_string(++next_session_seq_);
        sid = session_id_;

        // 重连判定：注册表已有该 player_id 的条目 = 二次注册（掉线重连或
        // 进程重启）。首次注册 find 不到 → 新条目。
        // 【指派模型 2026-11-06】display 采用指派 id 后会以指派 id 重新注册；
        // 同槽位旧自报 id 的条目是换键前的残影（连接归属本槽位、旧条目的
        // 在线哨兵已无意义），此处清理并回收其 UDP 反查表项，避免注册表
        // 残留幽灵条目（心跳路由/在线计数被旧条目污染）。
        for (auto it = players_.begin(); it != players_.end();) {
            if (it->first != pid && it->second.display_id == display_slot) {
                for (auto uit = udp_addr_to_player_.begin();
                     uit != udp_addr_to_player_.end();) {
                    if (uit->second == it->first)
                        uit = udp_addr_to_player_.erase(uit);
                    else
                        ++uit;
                }
                std::printf("[editor] 换键清理: 移除同槽位旧条目 player=%s"
                            "（槽位=%s 已改用 %s）\n",
                            it->first.c_str(), display_slot.c_str(),
                            pid.c_str());
                it = players_.erase(it);
            } else {
                ++it;
            }
        }
        auto it = players_.find(pid);
        is_reconnect = (it != players_.end());
        auto& e      = players_[pid];
        if (is_reconnect) ++e.reconnects;
        e.conn_id    = 1;                       // 在线哨兵（断开由 onDisplayState 清 0）
        e.display_id = display_slot;            // connector 槽位 id
        e.udp_host   = player_udp_host;         // UDP 心跳目标主机（对端 IP）
        e.udp_port   = static_cast<std::uint16_t>(udp_report_port);
        e.offline    = false; // 重连即恢复健康观测窗口
        e.last_beat_us = -1;  // 重置观测窗口（等待新连接的上报）
    }

    // ---- Ack（含会话号 + Editor 的 UDP 心跳地址 + 素材 HTTP 端口 + 指派 id）----
    // ack_udp_host = 连接的本端 IP（Editor 主动方，最准确）
    Envelope ack_env;
    {
        auto* ack = ack_env.mutable_register_ack();
        ack->set_accepted(true);
        ack->set_session_id(sid);
        ack->set_udp_host(ack_udp_host);
        ack->set_udp_port(udp_.port());
        ack->set_http_port(assets_http_ ? assets_http_->port() : 0);
        // 【指派模型 2026-11-06】工程槽位 id 随 Ack 下发：display 自报 id
        // 只是"本地标签"，真正的屏身份由 Editor 指派——display 收到后
        // 采用（非空且 ≠ 自报时换 id 并重新注册，换键即自然对齐）。
        if (!display_slot.empty())
            ack->set_assigned_player_id(display_slot);
    }
    reply(ack_env);

    // ---- 场景下发（有场景才发）----
    // 【与 GUI 分离 2026-11-04】入队 ScenePrepare（快照+serialize+socket
    // 写在发送线程）——recv 线程只做快照，不再被大场景 serialize 阻塞。
    const bool scene_sent = sendSceneToDisplay(display_slot);

    // ---- 记录 UDP 反查表（"ip:port" → player_id；§6 学到的来源地址）----
    // 日志字段一并锁内拷出（players_ 可被其他线程并发修改，禁止无锁读）
    std::string   log_udp_host;
    std::uint16_t log_udp_port = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        const auto& e = players_[pid];
        if (e.udp_port != 0 && !e.udp_host.empty())
            udp_addr_to_player_[e.udp_host + ":" + std::to_string(e.udp_port)] = pid;
        log_udp_host = e.udp_host;
        log_udp_port = e.udp_port;
    }

    std::printf("[editor] 注册: player=%s udp=%s:%u session=%s%s%s\n",
                pid.c_str(),
                log_udp_host.empty() ? "?" : log_udp_host.c_str(),
                static_cast<unsigned>(log_udp_port),
                sid.c_str(),
                scene_sent ? "（场景已下发）" : "",
                is_reconnect ? "【重连】" : "");

    // ---- 重连并入（§7.1④"跳到现在"）----
    // 会话已在播放/暂停时，向重连的 Player 单播当前状态锚点：
    //   Playing → PLAY（锚点时刻的播放头 → Player 迟到补偿自动落在 target）
    //   Paused  → PAUSE（冻结于暂停点）
    // 【2026-11-03 SEEK 语义修订】Playing 并入从 CMD_SEEK 改为 CMD_PLAY：
    // SEEK 已改为"状态保持"（编辑器定位语义，Idle 中忽略）——重连并入
    // 需要"无中生有进入播放"，正是 PLAY 的语义（从 anchored 位置起播）。
    // 二者锚点数学完全等价（session_start = anchor - anchored，迟到补偿
    // 落在共享时间轴当前位置），只是不依赖 Player 自身既有状态。
    if (is_reconnect) {
        // 锁内只读状态并算好锚点，【发送放锁外】——reply 走 TCP 阻塞写，
        // 持 state_mutex_ 发送会冻结所有状态读者（观测表/补丁/下发）。
        bool          need_reanchor = false;
        AnchorCmd     cmd           = AnchorCmd::Pause;
        std::int64_t  anchor_time   = 0;
        std::int64_t  anchored      = 0;
        std::string   ra_sid;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (state_ != PlayState::Idle && !session_id_.empty()) {
                need_reanchor = true;
                cmd = (state_ == PlayState::Playing) ? AnchorCmd::Play
                                                     : AnchorCmd::Pause;
                // recv 线程（无调用方注入时钟）→ 用真实对齐时钟（ClockProvider
                // 复合时钟与 Player 端同域：系统单调钟 + NTP offset）
                anchor_time = ClockProvider().alignedNowUs() + cfg_.reanchor_lead_us;
                // 锚点时刻播放头应处的位置（全场同式同值）
                anchored = ClockProvider::sessionTimeUs(anchor_time,
                                                        session_start_us_);
                ra_sid = session_id_;
            }
        }
        if (need_reanchor) {
            Envelope ra;
            auto* m = ra.mutable_re_anchor();
            m->set_anchor_time_us(anchor_time);
            m->set_anchored_session_time_us(anchored);
            m->set_session_id(ra_sid);
            m->set_cmd(cmd == AnchorCmd::Play
                           ? ms_proto::MsgReAnchor::CMD_PLAY
                           : ms_proto::MsgReAnchor::CMD_PAUSE);
            reply(ra);
            std::printf("[editor] 重连并入: player=%s cmd=%s anchored=%lldus "
                        "（跳到现在，不补放缺失段落）\n",
                        pid.c_str(), cmd == AnchorCmd::Seek ? "SEEK" : "PAUSE",
                        static_cast<long long>(anchored));
        }
    }
}

// ---------------------------------------------------------------------------
// Ack 公共处理（server/display 两通道共用；recv 线程上下文）
// ---------------------------------------------------------------------------
void EditorRuntime::handleAck(
    const ms_proto::MsgAck& a,
    const std::function<void(const Envelope&)>& reply) {
    (void)reply;
    // 仅日志留痕：Ack 目前只承载重锚确认（调试/超时观测用）。
    // 【第七迭代】场景全量广播无 Ack 应答语义——增量补丁的"失败补发"
    // 机制已随补丁退役；漏收场景的显示端由下一次广播直接追平（全量
    // 自描述，无版本连续性依赖）。
    std::printf("[editor] Ack: anchor=%lldus ok=%d %s\n",
                static_cast<long long>(a.anchor_time_us()),
                a.ok(), a.detail().c_str());
}

// ---------------------------------------------------------------------------
// 场景单播（通道无关：注册应答/重发按钮路径专用；§6.2）
// 主动广播入口在 cmdPublishScene（第七迭代：全量广播 + CRC 回填）
// ---------------------------------------------------------------------------
bool EditorRuntime::sendSceneToDisplay(const std::string& display_id) {
    // 锁内快照（深拷贝）——【与 GUI 分离 2026-11-04】调用方（GUI 的
    // 重发按钮/打开项目、recv 的注册应答）只做快照 + 入队（微秒级）：
    // 素材注册（磁盘 stat×N）、serialize、socket 写全部在发送线程执行。
    // 【断点修复②】prepare 阶段仍先 ensureAssetsRegistered（幂等，文件
    // 未变零成本）——editor 重启后素材注册表已清空，display 重连拿到
    // 场景去拉素材必 404；prepare 保证注册先行（推模型，见 sendWorkerLoop
    // 的 ScenePrepare 分支）。
    Scene snap;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (!scene_) return false; // 无场景：无事可做
        snap = *scene_;
    }
    SendTask t;
    t.kind       = SendTask::Kind::ScenePrepare;
    t.display_id = display_id; // 非空 = 单播该槽位（FIFO 同目标保序 Ack→Scene）
    t.scene      = std::move(snap);
    enqueueSend(std::move(t));
    // 【手动同步修订 2026-11-04】场景下发不自动推素材清单——媒体同步
    // 由用户显式点"同步媒体"触发（cmdSyncMedia）。素材注册在发送线程
    // 的 prepare 中完成（拉取前提），清单不再自动广播——避免编辑防抖/
    // 重发场景时 display 端反复收到清单触发拉取检查。
    return true; // 已入队（异步下发）
}

// ---------------------------------------------------------------------------
// 【素材同步进度 2026-11-04】display 上报落快照（recv 线程上下文）
// 纯观测：写 asset_sync_ 供 UI 轮询标注，不参与任何同步语义。
// ---------------------------------------------------------------------------
void EditorRuntime::handleAssetProgress(
    const std::string& display_id, const ms_proto::MsgAssetProgress& p) {
    AssetSyncView v;
    v.done          = p.done();
    v.total         = p.total();
    v.current_path  = p.current_path();
    v.current_bytes = p.current_bytes();
    v.current_total = p.current_total();
    v.finished      = p.finished();
    v.last_update_us = std::chrono::duration_cast<std::chrono::microseconds>(
                           std::chrono::steady_clock::now().time_since_epoch())
                           .count();
    std::lock_guard<std::mutex> lock(state_mutex_);
    asset_sync_[display_id] = std::move(v);
}

// ---------------------------------------------------------------------------
// 【素材同步进度 2026-11-04】进度快照（GUI 线程轮询；锁内值拷贝）
// ---------------------------------------------------------------------------
std::map<std::string, EditorRuntime::AssetSyncView>
EditorRuntime::assetSyncSnapshot() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return asset_sync_;
}

// ---------------------------------------------------------------------------
// Player 上报帧归位（UDP recv 线程上下文）
// ---------------------------------------------------------------------------
void EditorRuntime::onUdpFrame(const SyncFrame& f,
                               const Poco::Net::SocketAddress& from) {
    if (f.role != SyncRole::Player) return; // 只关心 Player 上行

    // ---- 按"来源 ip:port"反查 player_id（§6 来源地址学习）----
    const std::string key = from.host().toString() + ":" +
                            std::to_string(from.port());
    // 【两段式】第一段锁内：反查 + 观测表更新 + 回应所需会话状态拷出；
    // 第二段锁外：send_to 网络发送（不得持 state_mutex_ 做 IO）
    std::string  sid;             // 锁内填充（此处不得无锁读 session_id_）
    PlayState    st    = PlayState::Idle;
    std::int64_t start = 0;
    bool         known = false;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        const auto it_id = udp_addr_to_player_.find(key);
        if (it_id == udp_addr_to_player_.end()) return; // 未注册来源：丢弃
        const auto it = players_.find(it_id->second);
        if (it == players_.end()) return;

        // ---- 更新观测表（status 面板 / 失同步检测的数据源）----
        auto& e      = it->second;
        e.state      = f.state;
        e.session_time_us = f.session_time_us;
        e.delta_us   = f.delta_us;
        e.frame_index = f.frame_index;
        ++e.reports;
        // 健康观测（§7.1③）：wall_us 是 Player 的对齐时钟读数（与 Editor
        // tick 的 now_us 同域可比）；任何上报都恢复 online
        e.last_beat_us = f.wall_us;
        e.offline      = false;

        // 逐帧回应心跳：携带共享 session_start（迟到加入/软重锚的数据源）。
        // 【注意】这里按"收到上报即回应"（每 Player ~4Hz），高于 Editor 主动
        // 单播周期——两条路都通，Player 任一途径都能学到锚点。
        sid   = session_id_;
        st    = state_;
        start = session_start_us_;
        known = true;
    }
    if (!known) return; // 不可达（防御性：上面两分支已 return）

    SyncFrame beat;
    beat.role             = SyncRole::Editor;
    beat.session_id       = sid;
    beat.state            = st;
    beat.session_start_us = start;
    beat.wall_us          = 0; // 回应帧不带墙钟（Player 不消费此字段）
    // 【发送解耦】UDP 发送也走队列（UDP sendTo 虽非阻塞语义，统一走
    // 发送线程——出站路径单一化，排查/限流/日志都只在一处）
    SendTask t;
    t.kind   = SendTask::Kind::UdpSend;
    t.beat   = std::move(beat);
    t.udp_to = from;
    enqueueSend(std::move(t));
}

// ---------------------------------------------------------------------------
// 播放控制命令（CLI 语义 → 锚点广播）
// ---------------------------------------------------------------------------
bool EditorRuntime::cmdPlay(std::int64_t now_us) {
    std::int64_t anchored = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        // 幂等：已在 Playing 则不重复广播
        if (state_ == PlayState::Playing) return true;
        // Paused → 从暂停点续播（冻结值，非 now 推算——暂停期间共享时钟
        // 仍在走，直接推算会跳过暂停时长）；Idle → 停止态定位点优先
        //（cmdSeek 在 Idle 记录的定位：停止后拖播放头再播放，从定位处
        // 起播——专业编辑器行为），无定位点则从头播。
        if (state_ == PlayState::Paused) {
            anchored = paused_time_us_;
        } else if (idle_seek_us_ >= 0) {
            anchored    = idle_seek_us_;
            idle_seek_us_ = -1; // 定位点一次性消费
        }
    }
    return broadcastReAnchor(now_us, AnchorCmd::Play, anchored);
}

bool EditorRuntime::cmdPause(std::int64_t now_us) {
    std::int64_t anchored = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (state_ != PlayState::Playing) return true; // 幂等
        // 冻结点 = 发命令时刻的播放头（全场各端以此为准立即冻结；
        // 端间差 = 命令送达延迟 1~5ms，画面瞬跳不可见——见
        // broadcastReAnchor 的"STOP/PAUSE 立即到点语义"注释）
        anchored = ClockProvider::sessionTimeUs(now_us, session_start_us_);
        paused_time_us_ = anchored; // 记录冻结点（status 展示 + 续播起点）
    }
    return broadcastReAnchor(now_us, AnchorCmd::Pause, anchored);
}

bool EditorRuntime::cmdSeek(std::int64_t now_us, std::int64_t session_time_us) {
    // 【Idle 拦截（拖动定位不再"自动播放"修复）】未起播/已停止时，拖播放
    // 头只是本地定位：display 无会话上下文（session_start 无效），广播
    // SEEK 会把它拉进无意义状态（旧实现还会转 Playing = "拖动就自动播放"
    // 的根因之一）。只记录定位点——随后 cmdPlay 从该点起播（idle_seek_us_）。
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (state_ == PlayState::Idle) {
            idle_seek_us_ = std::max<std::int64_t>(0, session_time_us);
            return true; // 本地定位完成（预览由 EditorWindow 直接驱动）
        }
    }
    // SEEK 语义（状态保持，见头文件）：锚点时刻播放头 = 目标位置；
    // Player 端 applyAnchorLocked 按其当前状态应用（Playing 续播 /
    // Paused 冻结新位置）——协议 §6.1 的"SEEK=Play+重定位"已修订
    return broadcastReAnchor(now_us, AnchorCmd::Seek, session_time_us);
}

bool EditorRuntime::cmdStop(std::int64_t now_us) {
    (void)now_us;
    const bool ok = broadcastReAnchor(0, AnchorCmd::Stop, 0);
    // STOP 立即生效语义（不等锚点）：锚点时间填 0 = 立即到点
    return ok;
}

// ---------------------------------------------------------------------------
// 场景发布（第七迭代架构重定位：广播实时全量）
//
// 职责分离模型（推模型修订 2026-11-04）：
//   场景同步 = 本命令——广播 MsgScene 全量（结构自描述，增量补丁退役）；
//   媒体同步 = 素材注册（ensureAssetsRegistered）+ 清单推送（pushAssetSync
//   广播 MsgAssetSync）——发布后双保险：场景消息触发 display 拉取，清单
//   消息兜底（CRC 后台算完后的下一次清单会带真值）。
// ---------------------------------------------------------------------------
bool EditorRuntime::cmdPublishScene(const Scene& next) {
    // ---- 1. 权威场景推进（调用方/GUI 线程仅有的一份深拷贝，微秒级）----
    // 【与 GUI 分离 2026-11-04】素材注册（磁盘 stat×N）+ 场景 serialize
    // 全部移交发送线程（ScenePrepare 任务）——大场景/多素材时界面线程
    // 零卡顿。调用方成本 = next 一份拷贝（推进权威）+ 一份入队载荷。
    // 【顺序关键】先推进权威再入队：ScenePrepare 执行窗口内新注册的
    // Player 经 sendSceneToDisplay 拿到已推进的权威场景（版本 ≥ 广播
    // 版本），两条路径收敛一致，无中间态（原"先推进再广播"语义保持）。
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        scene_ = next;
    }

    // ---- 2. 入队 ScenePrepare：素材注册 + serialize + 广播（发送线程）----
    // display_id 留空 = 广播语义（0 端在线 = 0 份送达，天然合法；漏收端
    // 由注册下发或下一次广播追平——全量自描述，无版本连续性依赖）。
    SendTask td;
    td.kind  = SendTask::Kind::ScenePrepare; // display_id 空 = 广播
    td.scene = next;
    enqueueSend(std::move(td));
    std::printf(
        "[editor] 场景发布: id=%s version=%s（%zu 屏 / %zu 素材 / "
        "%zu 轨）→ 已提交发送线程\n",
        next.id.c_str(), next.version.c_str(),
        next.screens.size(), next.assets.size(),
        next.timeline.tracks.size());

    // 【手动同步修订 2026-11-04】发布不自动推素材清单（媒体同步由用户
    // 点"同步媒体"显式触发）——素材注册仍在发送线程的 prepare 中完成
    // （display 拉取的前提）。
    return true; // 已入队（异步下发；广播语义，在线判定不参与成功性）
}

// ---------------------------------------------------------------------------
// 素材注册 + CRC 回填（推模型公共前置；ScenePrepare[发送线程] / cmdSyncMedia 共用）
//
// 幂等规则（2026-11-04 升级）：同 id + 同路径 + 同大小 + 同 mtime → 跳过；
// 路径或文件内容（size/mtime 身份）变化 → 重注册重算 CRC——文件被外部
// 工具替换后，下一次发布/同步媒体自动感知（WATCHOUT 式修改检测）。
// CRC 回填：【注册表权威·无脑覆盖】注册表有 CRC 用之，没有（后台未算完）
// 则清空——绝不保留场景携带的旧值：过期 CRC 会让显示端拉取后校验必败
// （CrcMismatch 反复重下）；空值降级为"只比长度"，安全且可自愈。
// ---------------------------------------------------------------------------
std::size_t EditorRuntime::ensureAssetsRegistered(Scene& target) {
    if (!assets_http_) return 0;
    std::size_t registered = 0;
    for (auto& a : target.assets) {
        // Text/Live 无磁盘文件（Live 帧随 Hub 再发布分发）——Live 走下方
        // 文件 stat 路径会拿空 path 误报"素材注册失败"（2026-11-10 修正）
        if (a.kind == AssetKind::Text || a.kind == AssetKind::Live) continue;
        // 绝对路径直通（Flow 编辑器空素材根语义）；相对拼 assets_root
        const std::string full = assetFullPath(cfg_.assets_root, a.path);
        // 幂等快判：已注册 + 路径同 + 大小同 + mtime 同 → 只回填 CRC
        // （mtime 检测覆盖"文件被外部替换但路径未变"——同 id 同路径不同
        // 内容时旧 CRC 是错的，必须重注册重算）
        // 【u8Path 必经】中文素材名窄串构造 path 抛 std::system_error
        //（与 display 拉取端同根因，2026-11-13 补防）
        std::error_code ec;
        const auto mtime = std::filesystem::last_write_time(ms::u8Path(full),
                                                            ec);
        // [round6 S3-2] size 回填（磁盘为准）：.show/外部导入链可能携带
        // size_bytes=0——显示端就绪表身份 = (size, checksum)，0 会让
        // "就绪表命中"永远失配，退化成每次发布都全文件 CRC（GB 级读盘）
        // + .asset_state.tsv size 恒 0。文件在位时以 stat 纠正。
        if (!ec) {
            std::error_code sec;
            const auto sz = std::filesystem::file_size(ms::u8Path(full), sec);
            if (!sec && sz > 0 && a.size_bytes != static_cast<std::uint64_t>(sz))
                a.size_bytes = static_cast<std::uint64_t>(sz);
        }
        AssetHttpServer::AssetEntry info;
        if (!ec && assets_http_->assetInfo(a.id, info) &&
            info.file_path == full && info.size_bytes == a.size_bytes &&
            info.mtime == mtime) {
            if (!info.crc32_hex.empty())
                a.checksum = "crc32:" + info.crc32_hex; // Player 校验用
            else
                a.checksum.clear(); // CRC 未就绪：清空 > 过期（降级只比长度）
            continue;
        }
        // 新注册 / 路径变化 / 文件内容变化（size/mtime 不符）：重注册
        // （注册表覆盖语义，CRC 后台重算——缓存命中路径零重算成本）
        if (assets_http_->registerAsset(a.id, full)) {
            ++registered;
            AssetHttpServer::AssetEntry fresh;
            if (assets_http_->assetInfo(a.id, fresh) &&
                !fresh.crc32_hex.empty())
                a.checksum = "crc32:" + fresh.crc32_hex;
            else
                a.checksum.clear(); // CRC 未就绪：先降级，下一次注册回填
            std::printf("[editor] 素材就位: 已注册 %s (%s)\n",
                        a.path.c_str(), a.id.c_str());
        } else {
            std::fprintf(stderr,
                         "[editor] 警告: 素材注册失败（文件缺失？）"
                         "%s (id=%s)\n",
                         full.c_str(), a.id.c_str());
            a.checksum.clear();
        }
    }
    return registered;
}

// ---------------------------------------------------------------------------
// CRC 写回权威场景（发送线程上下文；ScenePrepare 的收尾步骤）
//
// 【与 GUI 分离 2026-11-04】素材注册的 CRC 回填进的是快照（ScenePrepare
// 载荷），权威场景在 cmdPublishScene 推进时还不含 CRC——此处把快照的
// checksums 写回权威，保持"权威场景素材表携带 CRC"（.show 保存行为不变）。
// 仅在 id + version 与快照一致时覆盖：期间 GUI 已发布新版本（version
// 变化）则整体放弃，绝不把旧素材表的 CRC 盖到新场景上。
// ---------------------------------------------------------------------------
void EditorRuntime::backfillAssetChecksums(const Scene& snap) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!scene_) return;
    if (scene_->id != snap.id || scene_->version != snap.version)
        return; // 期间已发布新场景：快照已过时，放弃回写
    for (const auto& sa : snap.assets) {
        auto it = std::find_if(scene_->assets.begin(), scene_->assets.end(),
                               [&sa](const Asset& a) { return a.id == sa.id; });
        if (it != scene_->assets.end()) it->checksum = sa.checksum;
    }
}

// ---------------------------------------------------------------------------
// 【Live 分发归一化 2026-11-10】display 下发前把 Live 素材改写为 Hub 再发布流
//
// 架构定稿（LiveHub.hpp 头注释）的最后一环：display 端 LiveMediaSource 恒
// NDI 直收角色（Hub 不启用），收到的 live_source_name 若指向本机独占资源
// （摄像头/窗口/显示器捕获）则直开必失败——设备已被 editor 进程的 Hub 收端
// 占用（MF Source Reader 独占）、HWND/HMONITOR 句柄不跨机。此前场景广播
// 从未改写 Live 载荷，display 拿原始源名直连本机资源 = 摄像头/窗口捕获
// display 黑屏根因（预览端走 Hub Feed 进程内直用，不受影响）。
//
// 归一化（发送线程上下文；只改快照——权威场景不动，编辑端预览/.show 持久化
// 仍按原始源）：
//   1) ensureAsset：保证素材的 Ingest + Feed + NDI 再发布 sender 就位
//      （幂等；源变化指纹检测在此触发收端重建——低频发布路径，锁内
//      join ≤500ms 可接受）；
//   2) live_source_name ← "ms-live/<project>/<asset_id>"（与 LiveHub
//      发布名同源同格式；display 端 NdiIngest 归一化匹配可命中）；
//   3) live_ingest ← Ndi(0)（display 按 NDI 收再发布流）。
// Hub 未启用（单测直构 EditorRuntime）保持原样 = 旧直连语义，存量测试不破。
// ---------------------------------------------------------------------------
void prepareLiveForDisplay(Scene& snap) {
    LiveHub& hub = LiveHub::instance();
    if (!hub.enabled()) return;
    const std::string& project = hub.projectName();
    for (auto& a : snap.assets) {
        if (a.kind != AssetKind::Live) continue;
        hub.ensureAsset(a, project); // 收端/发布器统一由 editor Hub 持有
        a.live_source_name = "ms-live/" +
            (project.empty() ? std::string("project") : project) +
            "/" + a.id;
        a.live_ingest = static_cast<std::int32_t>(LiveIngest::Ndi);
    }
}

// ---------------------------------------------------------------------------
// 素材清单推送（推模型触发帧；cmdPublishScene / cmdSyncMedia 公共尾部）
//
// 广播 MsgAssetSync 给全部在线显示端（无条件广播语义，同场景广播——
// 0 端在线 = 0 份送达天然合法）。display 收到后与本地文件比对（存在 +
// 大小 + CRC），缺失/不一致的立即拉取并自愈重试。清单 KB 级，广播成本
// 可忽略；文件字节仍走 HTTP（大文件不占信令通道）。
// ---------------------------------------------------------------------------
std::size_t EditorRuntime::pushAssetSync(const Scene& source) {
    if (!assets_http_) return 0; // 素材分发未启用（cfg_.assets_root 为空）
    Envelope env;
    auto* m = env.mutable_asset_sync();
    m->set_scene_version(source.version);
    std::size_t n = 0;
    for (const auto& a : source.assets) {
        if (a.kind == AssetKind::Text) continue; // Text 无文件，随场景下发
        auto* e = m->add_entries();
        e->set_asset_id(a.id);
        e->set_path(a.path);
        e->set_size_bytes(a.size_bytes);
        e->set_checksum(a.checksum); // ensureAssetsRegistered 已回填
        ++n;
    }
    if (n == 0) return 0; // 无文件类素材：无需触发拉取
    SendTask td;
    td.kind = SendTask::Kind::DisplayBroadcast;
    td.env  = std::move(env);
    enqueueSend(std::move(td));
    std::printf("[editor] 素材清单推送: %zu 项 → 已广播给在线显示端\n", n);
    return n;
}

// ---------------------------------------------------------------------------
// 媒体文件同步（推模型 2026-11-04：注册 + 推清单两步）
//
// 旧版本只注册不通知——"同步媒体"按钮点击后显示端毫无反应（语义断层，
// 用户被迫再点"下发场景"制造重拉）。现注册完成后立即广播清单，显示端
// 收到即开始拉取/重拉，一步到位。
// ---------------------------------------------------------------------------
std::size_t EditorRuntime::cmdSyncMedia(const Scene& source) {
    if (!assets_http_) return 0; // 素材分发未启用（cfg_.assets_root 为空）
    // 注册（幂等：文件未变化的素材跳过；注册失败已在
    // ensureAssetsRegistered 内打警告）+ CRC 回填到 source 拷贝
    Scene probe = source;
    const std::size_t registered = ensureAssetsRegistered(probe);
    // 推清单（无论是否有新注册——确保"点按钮必有效"：display 端幂等
    // 比对，已就位的素材零网络成本）
    pushAssetSync(probe);
    return registered;
}

// ---------------------------------------------------------------------------
// CRC 就绪 → 防抖自动补推素材清单（2026-11-09）
//
// 【解决什么】CRC 后台就绪前，场景/清单携带的 checksum 是空值（安全降级
// "只比长度"）。旧实现要等用户再点一次"同步媒体"清单才补真值——显示端
// 就绪表里记录的空 checksum 与真值清单身份不符，触发一轮"本地 CRC 幂等"
// 重扫（无传输但大文件秒级/个，观感即"重复同步媒体"）。现在每个 CRC 算
// 完即置位事件（on_crc_ready），本线程 500ms 防抖聚合批量完成事件后重探
// 注册表并补推一次清单——显示端身份一次对齐，此后所有推送静默命中。
// 【与"手动同步修订"的关系】2026-11-04 取消的是"发布场景即推清单"（编辑
// 防抖期高频触发拉取检查）；本补推由 CRC 就绪驱动，每次会话只发生一轮
// （新注册素材才有新 CRC），且 display 端就绪比对零成本——语义正交。
// ---------------------------------------------------------------------------
void EditorRuntime::crcPushLoop() {
    for (;;) {
        // ---- 等事件（防抖窗口可被 stop 打断）----
        {
            std::unique_lock<std::mutex> lock(crc_push_mutex_);
            crc_push_cv_.wait(lock, [this] {
                return crc_push_quit_.load() || crc_push_pending_;
            });
            if (crc_push_quit_.load()) return;
            crc_push_pending_ = false; // 消费本次事件
            // 防抖：500ms 聚合后续完成事件（批量注册的多个 CRC 相继算完
            // 只触发一轮推送；期间新事件再置位 → 下一轮再推，幂等无害）
            crc_push_cv_.wait_for(lock, std::chrono::milliseconds(500), [this] {
                return crc_push_quit_.load();
            });
            if (crc_push_quit_.load()) return;
        }
        // ---- 重探注册表 + 补推清单（场景空 = GUI 未发布：事件丢弃）----
        Scene probe;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (!scene_) continue;
            probe = *scene_;
        }
        ensureAssetsRegistered(probe); // 幂等 + 缓存命中：刚算完的 CRC 此时可得
        pushAssetSync(probe);
        std::printf("[editor] CRC 就绪，已补推素材清单\n");
    }
}

/// 广播重锚 + 切换本地会话状态（cmd* 公共尾部）
bool EditorRuntime::broadcastReAnchor(std::int64_t now_us, AnchorCmd cmd,
                                      std::int64_t anchored_session_time_us) {
    // 【诊断 MS_CANVAS_DBG=1】重锚命令流——定位"播放中 state 反复回
    // Paused"的调用源：正常播放期应零 Pause/Seek 干扰（echo 拍 33ms
    // 读 state 80% 是 Paused 的矛盾在此抓现行）
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg) {
            static int s_n = 0;
            if (++s_n <= 200) {
                const char* name = cmd == AnchorCmd::Play   ? "Play"
                                   : cmd == AnchorCmd::Pause ? "Pause"
                                   : cmd == AnchorCmd::Seek  ? "Seek"
                                                             : "Stop";
                std::fprintf(stderr,
                             "[rt-dbg] reAnchor#%d cmd=%s t=%lldus\n", s_n,
                             name, (long long)anchored_session_time_us);
                std::fflush(stderr);
            }
        }
    }
    // ---- 1. 计算锚点 / 会话号（无会话时先分配）----
    std::int64_t anchor_time;
    std::string  sid;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (session_id_.empty())
            session_id_ = "s-" + std::to_string(++next_session_seq_);
        sid = session_id_;
        // 【STOP/PAUSE 立即到点语义（2026-11-03 修订）】anchor=0 → 各
        // Player 收到即在下一 tick 立即应用（不等未来时刻）。
        //   * STOP 立即：停止不等锚点（原设计）；
        //   * PAUSE 立即（本轮修订）：暂停/停止的"全场同时"由命令送达差
        //     保证（LAN 1~5ms ≪ 一帧 40ms，视觉无感）；未来锚点的 lead
        //     窗口对 PAUSE 有害——display 在窗口内继续推进，到点被冻结
        //     回 anchored，画面"倒回 lead 时长再停"（用户报告"倒回来播
        //     一会儿再停"，lead=500ms 时回跳肉眼可见）。
        // PLAY/SEEK 保留未来锚点：起播/跳转需各端同时切入，且等待期画面
        // 静止或在旧位置正常推进（含迟到补偿），无回跳问题。
        anchor_time = (cmd == AnchorCmd::Stop || cmd == AnchorCmd::Pause)
                          ? 0 : now_us + cfg_.reanchor_lead_us;

        // Editor 侧状态与时间基同步切换（与 Player 应用逻辑同式）
        switch (cmd) {
            case AnchorCmd::Play:
                state_            = PlayState::Playing;
                session_start_us_ = anchor_time - anchored_session_time_us;
                segment_anchored_us_ = anchored_session_time_us;
                break;
            case AnchorCmd::Pause:
                state_ = PlayState::Paused;
                // session_start 不变（冻结语义）
                segment_anchored_us_ = anchored_session_time_us;
                idle_seek_us_ = -1; // 停止态定位点作废（已进播放会话）
                break;
            case AnchorCmd::Seek:
                // 【状态保持语义（2026-11-03 修订）】SEEK 只重定位播放头，
                // 不改变播放状态：
                //   * Playing → 跳转续播（session_start 重算，与旧一致）；
                //   * Paused → 冻结点移到新位置（不再"隐式恢复播放"——
                //     旧语义下编辑器拖播放头定位会把暂停中的 display 强行
                //     拉回播放，"暂停失效"错觉的根因）；
                //   * Idle → 理论不可达（cmdSeek 已拦截），防御性保持。
                if (state_ == PlayState::Playing) {
                    session_start_us_ = anchor_time - anchored_session_time_us;
                    segment_anchored_us_ = anchored_session_time_us;
                } else if (state_ == PlayState::Paused) {
                    paused_time_us_      = anchored_session_time_us;
                    segment_anchored_us_ = anchored_session_time_us;
                }
                break;
            case AnchorCmd::Stop:
                state_            = PlayState::Idle;
                session_start_us_ = 0;
                paused_time_us_   = 0;
                segment_anchored_us_ = 0;
                idle_seek_us_ = -1; // 旧定位点作废（停止后需重新拖动定位）
                break;
            default:
                return false;
        }
    }

    // ---- 2. 组帧广播（TCP；Player 数量个位数，广播开销可忽略）----
    Envelope env;
    auto* m = env.mutable_re_anchor();
    m->set_anchor_time_us(anchor_time);
    m->set_anchored_session_time_us(anchored_session_time_us);
    m->set_session_id(sid);
    switch (cmd) {
        case AnchorCmd::Play:  m->set_cmd(ms_proto::MsgReAnchor::CMD_PLAY);  break;
        case AnchorCmd::Pause: m->set_cmd(ms_proto::MsgReAnchor::CMD_PAUSE); break;
        case AnchorCmd::Seek:  m->set_cmd(ms_proto::MsgReAnchor::CMD_SEEK);  break;
        case AnchorCmd::Stop:  m->set_cmd(ms_proto::MsgReAnchor::CMD_STOP);  break;
        default: return false;
    }
    // display 通道广播（旧 server 入站通道已退役）。
    // 【发送解耦】只入队（socket 写在发送线程；GUI 零阻塞）。
    // 【广播语义】无条件发——不判断是否有 Player 在线：本地会话状态
    // 已按锚点推进（上方 switch），0 端在线 = 0 份送达，天然合法；
    // 错过锚点的 Player 由注册时 re-anchor 并入补齐（§7.1④）。
    {
        SendTask td;
        td.kind = SendTask::Kind::DisplayBroadcast;
        td.env  = std::move(env);
        enqueueSend(std::move(td));
    }
    std::fprintf(stderr, "[%lld][editor] 重锚广播: cmd=%d anchor=+%lldus anchored=%lldus → 已入队\n",
                tsMs(),
                static_cast<int>(cmd),
                static_cast<long long>(anchor_time),
                static_cast<long long>(anchored_session_time_us));
    // 本地状态已推进 + 广播已入队 = 命令生效（网络送达不参与成功性判定）
    return true;
}

// ---------------------------------------------------------------------------
// 心跳单播节拍
// ---------------------------------------------------------------------------
void EditorRuntime::tick(std::int64_t now_us) {
    bool due = false;
    std::vector<std::string> newly_offline; // 锁外打日志
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (now_us - last_beat_us_ >= cfg_.beat_interval_us) {
            last_beat_us_ = now_us;
            due = true;
        }
        // ---- 心跳超时扫描（§7.1③）：TCP 在线但 UDP 上报停了（半开连接/
        // 网络单向故障）→ 标 OFFLINE。只对收到过上报的 Player 检测
        // （last_beat_us=-1 表示尚无上报窗口，不判超时）。
        for (auto& [pid, e] : players_) {
            if (e.conn_id == 0 || e.offline) continue; // 已断/已标
            if (e.last_beat_us >= 0 &&
                now_us - e.last_beat_us > cfg_.player_timeout_us) {
                e.offline = true;
                newly_offline.push_back(pid);
            }
        }
    }
    // 【诊断 MS_NO_BEAT=1】禁用心跳——二分"播放期定时器 250ms 恒定挡顿"
    // 是否 sendBeat（beat_interval_us=250ms 恰与尖峰节奏同步）因果：禁用
    // 后尖峰消失 → 深挖 sendBeat 内的阻塞点；仍 250ms → 排除（节奏巧合）。
    if (due) {
        static const bool s_no_beat = std::getenv("MS_NO_BEAT") != nullptr;
        if (!s_no_beat) sendBeat(now_us);
    }
    for (const auto& pid : newly_offline)
        std::printf("[editor] 心跳超时: player=%s（>%lldus 无上报 → OFFLINE）\n",
                    pid.c_str(),
                    static_cast<long long>(cfg_.player_timeout_us));
}

void EditorRuntime::sendBeat(std::int64_t now_us) {
    // 快照会话状态（短锁）
    std::string sid;
    PlayState   st = PlayState::Idle;
    std::int64_t start = 0;
    std::int64_t stime = 0;
    const auto beat_t0 = std::chrono::steady_clock::now(); // 【诊断】分段计时
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        sid   = session_id_;
        st    = state_;
        start = session_start_us_;
        stime = ClockProvider::sessionTimeUs(now_us, session_start_us_);
    }
    const auto beat_t1 = std::chrono::steady_clock::now(); // 快照段止
    // 锚点未到（play 锚点在 future）：播放头尚未生效——心跳必须发 Idle，
    // 否则迟到的 Player 会从负播放头软对齐"提前开播"
    if (stime < 0) st = PlayState::Idle;
    if (sid.empty()) return; // 无会话：无心跳可发

    SyncFrame beat;
    beat.role             = SyncRole::Editor;
    beat.session_id       = sid;
    beat.state            = st;
    beat.session_start_us = start;
    beat.session_time_us  = stime;
    beat.wall_us          = now_us;

    // 向所有已知 UDP 地址的 Player 单播（短锁拷贝地址表；【发送解耦】
    // 每目标入队一个 UdpSend 任务，socket 写在发送线程——tick 来自
    // GUI 定时器，零阻塞）
    std::vector<std::pair<std::string, std::uint16_t>> targets;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        for (const auto& [pid, e] : players_) {
            if (e.conn_id != 0 && e.udp_port != 0 && !e.udp_host.empty())
                targets.emplace_back(e.udp_host, e.udp_port);
        }
    }
    for (const auto& [host, port] : targets) {
        SendTask t;
        t.kind  = SendTask::Kind::UdpSend;
        t.beat  = beat;
        t.udp_to = Poco::Net::SocketAddress(host, port);
        enqueueSend(std::move(t));
    }
    // 【诊断 MS_CANVAS_DBG=1】sendBeat 分段耗时——快照锁段 / 目标拷贝段 /
    // 全程（含 enqueue 唤醒）。定位"播放期 GUI 每 250ms 挡顿 ~200ms"是否
    // 心跳内部阻塞（beat_interval_us=250ms 与尖峰节奏同步已由 MS_NO_BEAT
    // 禁用实验实锤因果）。
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg) {
            static int s_n = 0;
            if (++s_n <= 400) {
                const auto us = [](auto a, auto b) {
                    return std::chrono::duration_cast<
                        std::chrono::microseconds>(b - a).count();
                };
                std::fprintf(stderr,
                             "[rt-dbg] beat#%d total=%lldus snap=%lldus "
                             "targets=%zu\n",
                             s_n, (long long)us(beat_t0,
                                                std::chrono::steady_clock::now()),
                             (long long)us(beat_t0, beat_t1), targets.size());
                std::fflush(stderr);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 出站发送队列（专职发送线程；GUI/recv 与网络解耦，2026-11-03）
//
// 【为什么需要】DisplayConnector/SyncUdp 的 sendBytes 是阻塞
// 语义——对端接收窗口满（半开连接/对端卡死）时可无限期阻塞。此前
// cmd*/tick（GUI 线程）与 recv 线程应答直接调它们：一条卡死的连接就
// 能冻住整个编辑器界面。本队列把 socket 写全部收敛到专职发送线程，
// 调用方只入队（微秒级拷贝+通知）。
//
// 【通道分类（对齐协议语义）】
//   广播（ReAnchor/Patch）：DisplayBroadcast 一帧，
//     无条件入队——不判断是否有 Player 在线（0 端在线 = 0 份送达）；
//   单发（protobuf 指定目标）：DisplayUnicast(display_id) / UdpSend(心跳)。
//   FIFO 保序：同目标的 Ack→Scene 先后由入队顺序保证（§6.2 注册应答
//   顺序不可乱）。
// ---------------------------------------------------------------------------
void EditorRuntime::enqueueSend(SendTask&& t) {
    t.enqueue_ms = tsMs(); // 【下发延迟探针】统一盖入队时戳（发送线程量排队时长）
    {
        std::lock_guard<std::mutex> lock(send_mutex_);
        // 有界防护：正常流量（个位数 Player × 4Hz 心跳 + 低频命令）远触
        // 不到上限；网络长时间瘫痪时丢可再生的 UDP 心跳（Player 靠
        // 周期上报恢复对齐），TCP 语义帧不丢（命令/应答珍贵）
        if (send_queue_.size() >= kSendQueueLimit &&
            t.kind == SendTask::Kind::UdpSend)
            return;
        send_queue_.push_back(std::move(t));
    }
    send_cv_.notify_one();
}

void EditorRuntime::sendWorkerLoop() {
    for (;;) {
        SendTask t;
        {
            std::unique_lock<std::mutex> lock(send_mutex_);
            // 有任务即醒；100ms 兜底轮询（信号丢失时自然消化队列）
            send_cv_.wait_for(lock, std::chrono::milliseconds(100),
                              [this] { return send_quit_ || !send_queue_.empty(); });
            if (send_queue_.empty()) {
                if (send_quit_) return;
                continue; // 空转唤醒：无任务，回去等
            }
            t = std::move(send_queue_.front());
            send_queue_.pop_front();
        }
        // 锁外发送：序列化 + socket 写（阻塞/慢连接只影响本线程，
        // 调用方与接收路径全部不受影响）
        switch (t.kind) {
            case SendTask::Kind::DisplayBroadcast:
                displays_.broadcast(t.env);
                break;
            case SendTask::Kind::DisplayUnicast:
                displays_.sendTo(t.display_id, t.env);
                break;
            case SendTask::Kind::UdpSend:
                udp_.send_to(t.beat, t.udp_to);
                break;
            case SendTask::Kind::ScenePrepare: {
                // 【与 GUI 分离 2026-11-04】场景准备（素材注册 + serialize）
                // 与 socket 写同线程执行：GUI/recv 调用方只入队 Scene 快照。
                // 【异常护栏 2026-11-04】prepare 链路（文件系统 stat /
                // serialize 大场景分配）可能抛异常——本函数是常驻发送线程
                // 的线程函数，异常逃出即 std::terminate 杀死整个进程；
                // 捕获后记日志、丢弃本帧继续循环（下一帧/下次发布自然重试）。
                const long long deq_ms = tsMs(); // 出队（= 排队结束）时刻
                try {
                    // 1) 素材就位（幂等；重注册路径 registerAsset 立即返回，
                    //    CRC 后台算——发送线程零阻塞等待；注册表 mutex 保护，
                    //    线程安全；CRC 回填进快照）
                    Scene snap = std::move(t.scene);
                    ensureAssetsRegistered(snap);
                    // 【Live 归一化 2026-11-10】display 收 Hub 再发布流：
                    // ensureAsset（sender 就位）+ 快照内改写 ms-live 名
                    prepareLiveForDisplay(snap);
                    const long long prep_ms = tsMs();
                    // 2) CRC 写回权威（version 匹配才覆盖——期间新发布则放弃）
                    backfillAssetChecksums(snap);
                    // 3) 组全量帧（Player 收到即整体切换，播放不中断）
                    Envelope env;
                    auto* m = env.mutable_scene();
                    m->set_scene_id(snap.id);
                    m->set_version(snap.version);
                    m->set_payload(SceneLoader::serialize(snap));
                    const std::size_t pub_bytes = env.scene().payload().size();
                    const long long ser_ms = tsMs();
                    // 4) 投递：display_id 空 = 广播（cmdPublishScene 路径），
                    //    非空 = 单播该槽位（注册应答/重发按钮路径）
                    if (t.display_id.empty()) {
                        displays_.broadcast(env);
                        std::printf(
                            "[editor] 场景下发(全量): id=%s version=%s（%zu 字节）"
                            "→ 已广播在线显示端\n",
                            snap.id.c_str(), snap.version.c_str(), pub_bytes);
                    } else {
                        displays_.sendTo(t.display_id, env);
                        std::printf(
                            "[editor] 场景下发(全量): id=%s version=%s（%zu 字节）"
                            "→ 单播 %s\n",
                            snap.id.c_str(), snap.version.c_str(), pub_bytes,
                            t.display_id.c_str());
                    }
                    // 【下发延迟探针】三段分解（stderr 带墙钟戳，与 display
                    // "[player] 场景已接收" 同钟域可直接相减）：
                    //   排队 = 入队→出队（GUI 侧的发布频率 vs 发送线程吞吐）
                    //   准备 = 素材注册 stat/flush（磁盘抖动在这里暴露）
                    //   序列化 + 投递 = prototype 编码与 TCP 写（对端窗口满时
                    //   会拖到 send 超时上限）
                    std::fprintf(
                        stderr,
                        "[%lld][pub-lat] version=%s 排队=%lldms 准备=%lldms "
                        "序列化=%lldms 投递=%lldms bytes=%zu\n",
                        tsMs(), snap.version.c_str(),
                        deq_ms - t.enqueue_ms, prep_ms - deq_ms,
                        ser_ms - prep_ms, tsMs() - ser_ms, pub_bytes);
                    std::fflush(stderr);
                } catch (const std::exception& e) {
                    std::printf("[editor] 场景下发失败（已丢弃本帧）: %s\n",
                                e.what());
                } catch (...) {
                    std::printf("[editor] 场景下发失败（未知异常，已丢弃本帧）\n");
                }
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 状态观测
// ---------------------------------------------------------------------------
PlayState EditorRuntime::state() const {
    // 【诊断 MS_CANVAS_DBG=1】锁等待计时——echo 每拍调 state()，若本锁
    // 被其它线程长持 ~200ms，这里直接量出等待时长与发生时刻（配合
    // probe/worker 日志的 wall 对齐，锁定持锁者身份）
    const auto lk_t0 = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(state_mutex_);
    const auto lk_us = std::chrono::duration_cast<
        std::chrono::microseconds>(
        std::chrono::steady_clock::now() - lk_t0).count();
    if (lk_us > 10000) { // 等待 >10ms：异常，逐次打印
        static int s_n = 0;
        if (++s_n <= 200) {
            const auto wall = std::chrono::duration_cast<
                std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            std::fprintf(stderr,
                         "[rt-dbg] stateLockWait#%d %lldus wall=%lld\n",
                         s_n, (long long)lk_us, (long long)wall);
            std::fflush(stderr);
        }
    }
    return state_;
}

std::string EditorRuntime::session_id() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return session_id_;
}

std::int64_t EditorRuntime::session_time_us() const {
    // 无参版本：用真实对齐时钟（运行期便捷入口；测试走带 now 的重载）
    return session_time_us(ClockProvider().alignedNowUs());
}

std::int64_t EditorRuntime::session_time_us(std::int64_t now_us) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    switch (state_) {
        case PlayState::Playing: {
            // 重锚提前量窗口（anchor_time = now+lead > now）内，共享时钟
            // 推算值 = 锚定值 - lead剩余 —— 小于锚定值。本地显示冻结在
            // 锚定值（与 Player "到点应用锚点"的观测一致），到点起推进。
            // 不冻结则点播放/拖播放头后游标会先回跳 lead 时长（500ms/12帧）。
            const std::int64_t t =
                ClockProvider::sessionTimeUs(now_us, session_start_us_);
            return t > segment_anchored_us_ ? t : segment_anchored_us_;
        }
        case PlayState::Paused:
            return paused_time_us_; // 冻结于暂停点（与 Player 行为一致）
        case PlayState::Idle:
        default:
            return 0;
    }
}

std::size_t EditorRuntime::registered_count() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    std::size_t n = 0;
    for (const auto& [pid, e] : players_)
        if (e.conn_id != 0) ++n;
    return n;
}

std::size_t EditorRuntime::offline_count() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    std::size_t n = 0;
    for (const auto& [pid, e] : players_)
        if (e.conn_id != 0 && e.offline) ++n;
    return n;
}

bool EditorRuntime::playerView(const std::string& player_id,
                               PlayerView& out) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    const auto it = players_.find(player_id);
    if (it == players_.end()) return false;
    const auto& e = it->second;
    out.player_id      = player_id;
    out.conn_id        = e.conn_id;
    out.has_udp        = e.udp_port != 0 && !e.udp_host.empty();
    out.offline        = e.offline;
    out.state          = e.state;
    out.session_time_us = e.session_time_us;
    out.delta_us       = e.delta_us;
    out.frame_index    = e.frame_index;
    out.reports        = e.reports;
    out.reconnects     = e.reconnects;
    return true;
}

std::string EditorRuntime::scene_version() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return scene_ ? scene_->version : std::string();
}

std::vector<std::string> EditorRuntime::player_ids() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    std::vector<std::string> ids;
    ids.reserve(players_.size());
    for (const auto& [pid, e] : players_) ids.push_back(pid);
    return ids;
}

bool EditorRuntime::sceneSnapshot(Scene& out) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!scene_) return false;
    out = *scene_; // 值语义深拷贝（UI 编辑工作副本，与权威解耦）
    return true;
}

} // namespace ms
