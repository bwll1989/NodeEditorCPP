// ============================================================================
//  EditorRuntime.hpp —— Editor 侧运行时装配层（实现计划 P2，§4）
//
//  设计依据：《media-server-sync-design.md》§4.1（Editor）§6（协议）§7.1
//    把 DisplayConnector + SyncUdp + SceneLoader + AnchorCmd 语义装配成
//    "控制权威"闭环（WATCHOUT 模型：Editor 主动连接显示端，本类【不再】
//    监听 TCP 入站——旧 server 兼容通道已随 Player 旧模式退役，2026-11-03）：
//
//      收 MsgRegister ──► 回 Ack（分配 session_id）──► 下发 MsgScene
//                    └─► 记录 player_id → UDP 地址（TCP 对端 IP + 上报端口）
//
//      CLI 命令（play/pause/seek/stop）──► 计算锚点（未来 500ms）──►
//      广播 MsgReAnchor ──► 各 Player 到点同时应用（§6.1 全场同刻）
//
//      心跳单播（250ms）──► 每个 Player 一帧 SyncFrame（携带共享
//      session_start）──► 迟到加入的 Player 软对齐
//
//      收 UDP 上报帧 ──► 按 (来源 IP:端口) 反查 player_id ──► 更新
//      观测表（state/session_time/delta/frame_index）──► status 面板
//
//  【时间注入】同 PlayerRuntime：命令与 tick 均接收外部对齐时钟值，
//  本类不持有时钟——真实运行由 main 供 ClockProvider 读数，测试用手工序列。
//
//  线程模型：
//    * DisplayConnector recv 线程：Register/Ack 回调 → 注册表/观测表更新
//      （锁内）；
//    * UDP recv 线程：Player 上报帧 → 观测表更新（锁内）；
//    * 发送线程（专职出站）：所有出站帧（广播/单发/心跳）经无界队列
//      异步投递——调用方（GUI 的 cmd*/tick、recv 的应答）只做入队
//      （微秒级），socket 阻塞写（对端接收窗口满可无限期）全部离场，
//      界面不再被慢连接拖死；
//    * 调用方线程：cmd* / tick / status（仅入队 + 状态快照）。
//
//  【出站语义（2026-11-03 澄清；2026-11-04 增补 ScenePrepare）】
//    * 广播（ReAnchor/MsgScene）：无条件发——不判断是否有 Player 在线
//      （本地会话状态照常推进；0 端在线 = 帧 0 份送达，天然合法）；
//      重连的 Player 由注册时 re-anchor 并入补齐（§7.1④）；
//    * 单发（protobuf 指定目标）：注册 Ack / 全量场景 / 重连并入重锚 /
//      UDP 心跳——按目标逐个投递；
//    * 【场景下发与 GUI 分离（2026-11-04）】ScenePrepare 任务把"素材注册
//      （幂等 stat + 登记）+ 场景 serialize + socket 写"三步全部移交发送
//      线程：GUI/recv 调用方只做 Scene 快照深拷贝 + 入队（微秒级）——
//      大场景 serialize 与磁盘 stat 不再占用界面线程；
//    * 全部只入队：序列化与 socket 写在发送线程，队列 FIFO 保序
//      （同目标 Ack→Scene 的先后由入队顺序保证）。
// ============================================================================

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "loader/SceneLoader.hpp"
#include "loader/ShowFileIO.hpp" // DisplayEntry（第五迭代显示端登记）
#include "model/SceneModel.hpp"
#include "net/AssetHttpServer.hpp" // 素材 HTTP 分发（Distributor 底座）
#include "net/DisplayConnector.hpp" // 显示端主动连接（第五迭代，唯一 TCP 通道）
#include "net/SyncUdp.hpp"
#include "sync/AnchorScheduler.hpp" // AnchorCmd / Anchor（重锚命令语义）
#include "sync/SyncFrame.hpp"

namespace ms {

class EditorRuntime {
public:
    // -----------------------------------------------------------------------
    // 启动配置（对应 app/main.cpp 参数）
    // -----------------------------------------------------------------------
    struct Config {
        std::uint16_t udp_port = 17001;   ///< UDP 心跳端口（0=随机，测试用）
        std::uint16_t http_port = 17002;  ///< 素材 HTTP 分发端口（0=随机）
        /// 素材根目录（空 = 不启用素材分发）。注册素材时按
        /// assets_root + "/" + Asset.path 拼绝对路径（Asset.path 为
        /// '/' 分隔的相对路径，见 SceneModel）
        std::string assets_root;
        /// 重锚提前量（us）：anchor_time = now + lead——预留命令传播 +
        /// 调度抖动余量（§6.1；过大增加操作延迟，过小跨机时易错过锚点）。
        /// 【2026-11-03 用户调优】500ms 起播/停止延迟体感明显，默认收紧
        /// 到 10ms（本机/低延迟局域网部署；跨高延迟广域网可经 ini 调回）
        std::int64_t reanchor_lead_us = 10'000;
        /// Editor → Player 心跳单播周期（us）
        std::int64_t beat_interval_us = 250'000;
        /// Player 心跳超时（us）：超过此间隔无 UDP 上报 → 标 OFFLINE
        /// （§7.1③ 半开连接检测；TCP 断开是另一条独立检测路径）
        std::int64_t player_timeout_us = 3'000'000;
    };

    explicit EditorRuntime(Config cfg = {});

    ~EditorRuntime() { stop(); }
    EditorRuntime(const EditorRuntime&) = delete;
    EditorRuntime& operator=(const EditorRuntime&) = delete;

    // ---- 生命周期 ----------------------------------------------------------

    /// 启动：加载场景（可选）→ 装配显示端连接器 → 绑 UDP 心跳
    /// \param scene_path 可空（纯控制、无场景下发的运行模式）
    bool start(const std::string& scene_path = {});
    void stop();

    // ---- 端口查询 -----------------------------------------------------------
    std::uint16_t udp_port() const { return udp_.port(); }
    /// 素材 HTTP 分发端口（未启用返回 0）
    std::uint16_t http_port() const {
        return assets_http_ ? assets_http_->port() : 0;
    }
    /// 已注册到 HTTP 分发的素材数（诊断）
    std::size_t asset_count() const {
        return assets_http_ ? assets_http_->asset_count() : 0;
    }

    // ---- 场景注入（测试用；生产路径走 --scene 文件加载）---------------------
    /// 直接设置场景（下次注册的 Player 会立即收到下发）
    void setScene(const Scene& scene);

    // ---- 播放控制命令（CLI → 锚点广播；now 由调用方注入）-------------------

    /// 播放：anchor = now + lead，anchored = 暂停点续播 / 停止态定位点
    /// （cmdSeek 记录）/ 从头（三者取先发生者）
    bool cmdPlay(std::int64_t now_us);
    /// 暂停：冻结于"锚点时刻"的播放头位置
    bool cmdPause(std::int64_t now_us);
    /// 跳转：【状态保持语义（2026-11-03 修订）】SEEK 只重定位播放头，不
    /// 改变播放状态——Playing 中跳转续播；Paused 中跳转并冻结于新位置
    /// （不再"隐式恢复播放"）；Idle（未起播/已停止）中不广播（display
    /// 无会话上下文），只记录定位点——随后 cmdPlay 从该点起播。
    /// （旧语义"SEEK 一律转 Playing"导致编辑器定位操作把暂停/停止中的
    /// display 强行拉回播放——用户报告"拖动就自动播放/暂停后仍在跑"）
    bool cmdSeek(std::int64_t now_us, std::int64_t session_time_us);
    /// 停止：回 Idle（各 Player 清空调度队列与状态）
    bool cmdStop(std::int64_t now_us);

    /// 场景发布（第七迭代架构重定位：广播实时全量，增量补丁机制退役；
    /// 【与 GUI 分离 2026-11-04】prepare 全部移交发送线程）：
    ///   1. 调用方（GUI 线程）只做：Scene 快照拷贝 + 权威推进 + 入队
    ///      ScenePrepare 任务（微秒级）——大场景 serialize 与素材磁盘
    ///      stat 不再占用界面线程；
    ///   2. 发送线程执行 ScenePrepare：ensureAssetsRegistered 幂等注册
    ///      缺失素材 + CRC 回填（推模型 2026-11-04，忘点"同步媒体"不
    ///      404）→ serialize → 广播 MsgScene 全量（全量即自描述：无需
    ///      基线比对、无版本连续性依赖，漏收端再广播一次即追平）；
    ///   3. CRC 写回权威场景（backfillAssetChecksums，version 匹配才
    ///      覆盖）——权威场景素材表携带 CRC 的行为保持不变。
    /// \return true = 已入队（异步下发；广播语义无条件，0 端在线 =
    ///          0 份送达天然合法）；false 仅在内部异常时出现
    bool cmdPublishScene(const Scene& next);

    /// 媒体文件同步（推模型，2026-11-04 行为升级）：
    ///   1. 遍历 source 的非 Text 素材 → 注册进 HTTP 分发服务器（幂等：
    ///      同 id 同路径同大小同 mtime 跳过；路径/文件内容变化重注册并
    ///      重算 CRC——后台计算）；
    ///   2. 【推清单】向全部在线显示端广播 MsgAssetSync（素材 id/大小/
    ///      CRC 清单）——display 收到即与本地比对，缺失/变化的重拉。
    ///      【行为升级】旧版本命令只更新注册表、不发任何网络消息，用户
    ///      点击后显示端毫无反应（语义断层，2026-11-04 断点分析③）；
    ///      现在注册 + 推清单一步到位，显示端立即开始拉取。
    /// \return 本次实际执行注册动作的素材数（跳过的不计）
    std::size_t cmdSyncMedia(const Scene& source);

    /// 心跳单播节拍（主循环每帧调用；内部按 beat_interval 节流）
    void tick(std::int64_t now_us);

    // ---- 显示端管理（第五迭代：editor 主动连接显示端）-----------------------

    /// 登记并连接一个显示端（异步：看门狗 ≤400ms 内发起连接；连接建立后
    /// 显示端上报 Register → 本类回 Ack + 下发场景，随后播放/补丁自动同步）
    /// \param pairing_code 配对码（空 = 不认证；非空 = 连接后先发 MsgAuth，
    ///        须与显示端 config.ini [display] pairing_code 一致）
    /// \return false = 参数非法或 display_id 重复
    bool addDisplay(const std::string& display_id, const std::string& host,
                    std::uint16_t port,
                    const std::string& pairing_code = {});
    /// 注销并断开（UI 线程调用；其下连接与注册表条目一并清理）
    bool removeDisplay(const std::string& display_id);
    /// 单显示端启用/禁用（场景树右键"连接/断开"入口，2026-11-04）：
    /// 禁用 = 立即断开该端 TCP 且看门狗不再自动重连（多屏演出中临时
    /// 摘除单屏，其余端不受影响）；启用 = 恢复 watchdog 自动连接。
    /// \return false = display_id 未登记
    bool setDisplayEnabled(const std::string& display_id, bool enabled) {
        return displays_.setEnabled(display_id, enabled);
    }
    /// 全部显示端登记快照（含连接状态；状态栏/UI 遍历用）
    /// 【注意】按值返回（每次调用重建 vector + 逐条 string 拷贝）——
    /// 仅低频 UI 路径使用；高频/只取数量的场景勿在循环内反复调用
    /// （曾引发"迭代两个临时容器"堆越界，见 EditorWindow 同名注释）。
    std::vector<DisplayEntry> displayEntries() const;
    /// 显示端运行状态快照（连接测试控件用）：DisplayConnector::Entry 含
    /// TCP 在线标志/显示端 IP/重连退避——比 displayEntries() 多运行态
    std::vector<DisplayConnector::Entry> displayStatusSnapshot() const {
        return displays_.snapshot();
    }
    /// 【素材同步进度 2026-11-04】各显示端拉取进度快照（锁内值拷贝）：
    /// display_id → 进度视图；场景树屏幕节点标注"⤓ 同步 N/M · P%"。
    /// display 断连时条目在 onDisplayState 清除；UI 侧另有时间老化
    /// （last_update_us 距今 >10s 且未 finished 视为停滞不显示）。
    struct AssetSyncView {
        std::size_t  done = 0;    ///< 已完成素材数
        std::size_t  total = 0;   ///< 本轮素材总数
        std::string  current_path; ///< 正在拉取的素材（Asset.path）
        std::uint64_t current_bytes = 0; ///< 当前文件已传字节
        std::uint64_t current_total = 0; ///< 当前文件总字节（0=未知）
        bool         finished = false; ///< 本轮全部结束
        std::int64_t last_update_us = 0; ///< 最近上报时刻（steady 域，老化判据）
    };
    std::map<std::string, AssetSyncView> assetSyncSnapshot() const;
    /// 在线显示端数（状态栏摘要）
    std::size_t displays_online() const { return displays_.connected_count(); }
    /// 全量场景重发到全部在线显示端（新项目加载后调用——场景 id/画布
    /// 变更不可增量，走 MsgScene 整体切换）。\return 送达的显示端数
    std::size_t resendSceneToDisplays();

    // ---- 状态观测 -----------------------------------------------------------

    /// 单个 Player 的观测快照（status 面板 / 测试断言）
    struct PlayerView {
        std::string  player_id;
        std::uint64_t conn_id = 0;       ///< TCP 连接号（0 = 已断开）
        bool         has_udp = false;    ///< UDP 地址是否已知
        bool         offline = false;    ///< 心跳超时标记（§7.1③）
        PlayState    state = PlayState::Idle;   ///< 最近上报的播放状态
        std::int64_t session_time_us = 0;///< 最近上报的播放头
        std::int64_t delta_us = 0;       ///< 最近上报的纠偏偏差
        std::uint32_t frame_index = 0;   ///< 最近上报的帧号
        std::uint64_t reports = 0;       ///< 累计收到上报帧数
        std::uint32_t reconnects = 0;    ///< 累计重连次数（Editor 观测）
    };

    /// Editor 侧会话状态
    PlayState   state() const;             ///< 当前播放状态（Idle/Playing/Paused）
    std::string session_id() const;        ///< 当前会话（空 = 无会话）
    std::int64_t session_time_us() const;  ///< Editor 侧推算的播放头
                                            ///< （Playing 时 = now - start；
                                            ///<  需调用方传 now）
    std::int64_t session_time_us(std::int64_t now_us) const;
    std::size_t  registered_count() const; ///< 已注册 Player 数
    std::size_t  offline_count() const;    ///< 心跳超时的在线 Player 数（诊断）

    /// 按 player_id 查询观测快照（不存在返回 false）
    bool playerView(const std::string& player_id, PlayerView& out) const;

    /// 当前权威场景版本（空 = 无场景；status 面板 / 补丁测试断言用）
    std::string scene_version() const;

    /// 全部已注册 player_id（观测表 UI 遍历用；含已断开条目）
    std::vector<std::string> player_ids() const;

    /// 权威场景快照（锁内深拷贝；UI 回显/编辑工作副本初始化用）。
    /// \return false = 无场景（未加载 / 未 start）
    bool sceneSnapshot(Scene& out) const;

private:
    // ---- 网络回调（recv 线程上下文）----------------------------------------
    void onUdpFrame(const SyncFrame& f, const Poco::Net::SocketAddress& from);

    // ---- 显示端入站（DisplayConnector recv/看门狗线程上下文；第五迭代）------
    void onDisplayEnvelope(const std::string& display_id,
                           const ms_proto::Envelope& env);
    void onDisplayState(const std::string& display_id, bool now_connected);

    // ---- 注册 / Ack 公共处理（display 通道专用；recv 线程上下文）------------
    /// 注册处理：建注册表条目 → 回 Ack（含会话号/UDP 心跳地址/HTTP 端口）→
    /// 下发场景 → UDP 反查表登记 → 重连并入锚点（§7.1④）。
    /// \param pid              显示端上报的 player_id
    /// \param udp_report_port  其 UDP 上报端口（0=未启用）
    /// \param player_udp_host  该端 IP（心跳单播目标；连接记录的对端 IP）
    /// \param ack_udp_host     Ack.udp_host（我方心跳地址告知对端；= 连接本端 IP）
    /// \param display_slot     display 通道的 connector 槽位 id
    /// \param reply            应答发送函数（入队 DisplayUnicast）
    void handleRegister(const std::string& pid, std::uint32_t udp_report_port,
                        const std::string& player_udp_host,
                        const std::string& ack_udp_host,
                        const std::string& display_slot,
                        const std::function<void(const ms_proto::Envelope&)>& reply);
    /// Ack 处理：日志留痕（重锚确认；场景全量广播无 Ack 语义——
    /// 漏收端靠再次广播追平，无需失败补发机制）
    void handleAck(const ms_proto::MsgAck& a,
                   const std::function<void(const ms_proto::Envelope&)>& reply);
    /// 【素材同步进度 2026-11-04】display 上报的拉取进度落快照
    /// （asset_sync_，state_mutex_ 保护；纯观测——不参与任何同步语义，
    /// 丢帧无害，display 端节流后每 ≥300ms 一帧覆盖）
    void handleAssetProgress(const std::string& display_id,
                             const ms_proto::MsgAssetProgress& p);
    /// 向指定显示端下发当前权威场景快照（注册应答/重发按钮路径专用——
    /// 主动广播走 cmdPublishScene）。【与 GUI 分离 2026-11-04】调用方
    /// （GUI/recv 线程）只做锁内 Scene 快照拷贝 + 入队 ScenePrepare 任务：
    /// 素材注册、serialize、socket 写全部移交发送线程——快照深拷贝是
    /// 调用方唯一成本（微秒级）。【推模型 2026-11-04】prepare 阶段仍先
    /// ensureAssetsRegistered：注册补发/重发按钮路径同样保证素材在分发侧
    /// 就位 + CRC 回填——旧实现直接序列化权威快照，editor 重启后注册表
    /// 已清空，重连端按场景拉取必 404（断点分析②）。
    /// \return false = 无权威场景（无事可做）；true = 已入队（异步下发）
    bool sendSceneToDisplay(const std::string& display_id);

    // ---- 素材就位与清单推送（推模型，2026-11-04）----------------------------
    /// 素材注册 + CRC 回填（sendWorkerLoop 的 ScenePrepare 与 cmdSyncMedia
    /// 的公共前置；【与 GUI 分离 2026-11-04】起在发送线程执行——注册表
    /// mutex 保护 + registerAsset 立即返回（CRC 后台算），线程安全且轻量）：
    /// 逐个非 Text 素材幂等注册进 HTTP 分发（同 id 同路径同大小同 mtime
    /// 跳过——文件被外部替换时 size/mtime 变化会重注册重算 CRC，即
    /// WATCHOUT 式"修改自动检测"），并把注册表 CRC 回填进场景素材表
    /// （无 CRC 清空 > 保留过期值——过期 CRC 导致显示端校验必败）。
    /// \return 本次实际注册的素材数
    std::size_t ensureAssetsRegistered(Scene& target);
    /// ScenePrepare 在发送线程完成素材注册（CRC 回填快照）后，把快照的
    /// checksums 写回权威场景——保持"权威场景素材表携带 CRC"（.show 保存
    /// 行为不变）。仅在 id + version 与快照一致时覆盖：期间 GUI 已发布
    /// 新版本（version 变化）则放弃，绝不回写旧数据。
    void backfillAssetChecksums(const Scene& snap);
    /// 向全部在线显示端广播 MsgAssetSync 素材清单（推模型触发帧；只入队）。
    /// \return 清单条目数（0 = 无素材或分发未启用）
    std::size_t pushAssetSync(const Scene& source);
    /// CRC 就绪 → 防抖自动补推素材清单（2026-11-09，"重复同步媒体"修复
    /// 链的 editor 侧收口）：AssetHttpServer::on_crc_ready 每算完一个 CRC
    /// 置位事件，本专职线程以 500ms 防抖窗口聚合批量完成事件后，重探注册
    /// 表（幂等+缓存命中，微秒级）并补推一次清单——显示端就绪表从"空
    /// checksum"一次对齐到真值，此后所有清单推送静默命中，不再出现
    /// "CRC 就绪后要手动再点同步媒体才补真值"的二次拉取窗口。
    /// 场景为空（GUI 启动后未发布）时事件被丢弃（无从推送，场景发布路径
    /// 的 ScenePrepare 自会注册+回填）。
    void crcPushLoop();

    // ---- 内部工具 -----------------------------------------------------------
    /// 广播重锚命令并切换本地会话状态（cmd* 的公共尾部）；
    /// 网络侧只入队（无条件广播，不判断在线——语义见文件头）
    bool broadcastReAnchor(std::int64_t now_us, AnchorCmd cmd,
                           std::int64_t anchored_session_time_us);
    /// 心跳单播节拍（tick 内按 beat_interval 节流；仅构建载荷+入队）
    void sendBeat(std::int64_t now_us);
    /// 出站任务入队（任何线程安全；有界防护见成员区 kSendQueueLimit）
    struct SendTask; // 嵌套类型前向声明（完整定义在成员区"出站发送队列"）
    void enqueueSend(SendTask&& t);
    /// 专职发送线程主循环：出队 → （ScenePrepare：素材注册+serialize）→
    /// socket 写（阻塞不伤调用方）
    void sendWorkerLoop();

    // ---- 成员 ---------------------------------------------------------------
    Config cfg_;
    SyncUdp       udp_;
    /// 显示端主动连接器（第五迭代）：连接/重连/收发由它承担，
    /// 注册与场景下发的业务语义由本类 onDisplayEnvelope 承接
    DisplayConnector displays_;
    std::optional<Scene> scene_;    ///< 待下发场景（start 时加载）
    /// 素材 HTTP 分发（cfg_.assets_root 非空时创建；unique_ptr 延迟构造
    /// 避免未启用时的线程池开销）。不依赖 scene_ 非空：GUI 模式启动时
    /// 场景为空，素材注册延后到"同步媒体"/"场景发布"（发布即幂等注册）
    std::unique_ptr<AssetHttpServer> assets_http_;

    // ---- 会话状态（state_mutex_ 保护）--------------------------------------
    mutable std::mutex state_mutex_;
    std::string  session_id_;             ///< "s-<时间戳>"
    PlayState    state_ = PlayState::Idle;
    std::int64_t session_start_us_ = 0;   ///< 会话起点（对齐时钟域）
    std::int64_t paused_time_us_ = 0;     ///< 暂停点冻结的播放头（status/续播用）
    /// 停止态定位点（us）：Idle 中 cmdSeek 只记录不广播——随后 cmdPlay
    /// 从该点起播（专业编辑器行为：停止后拖播放头定位，播放从定位处开始；
    /// 用后清零，cmdPause/cmdStop 也清零——定位点只对"下一次播放"有效）
    std::int64_t idle_seek_us_ = -1;
    /// 本段锚定的会话时间（Play/Seek/Pause 锚点值；重锚提前量窗口内
    /// 会话时钟尚未启动，session_time_us 冻结于此值——防游标回跳）
    std::int64_t segment_anchored_us_ = 0;
    std::int64_t last_beat_us_ = 0;       ///< 上次心跳单播时刻（节流）
    std::uint64_t next_session_seq_ = 0;  ///< 会话号序列（单调递增）

    /// 已注册 Player 注册表（player_id → 连接与 UDP 地址）
    /// 【在线哨兵模型】conn_id 表达"在线"：display 通道注册置 1，连接断开
    /// 由 onDisplayState 清 0——既有在线判定（registered_count/tick/
    /// sendBeat/offline_count）零改动。
    struct PlayerEntry {
        std::uint64_t conn_id = 0;         ///< 在线哨兵（1=在线 / 0=断开）
        std::string   display_id;          ///< connector 槽位 id（= Register.player_id）
        std::string   udp_host;             ///< UDP 心跳目标主机（对端 IP）
        std::uint16_t udp_port = 0;         ///< MsgRegister.udp_report_port
        // 观测数据（UDP 上报帧更新）
        PlayState     state = PlayState::Idle;
        std::int64_t  session_time_us = 0;
        std::int64_t  delta_us = 0;
        std::uint32_t frame_index = 0;
        std::uint64_t reports = 0;
        // 健康检测（§7.1③）
        std::int64_t  last_beat_us = -1;   ///< 最近上报的对齐时钟（-1=从未收到）
        bool          offline = false;     ///< 心跳超时标记（恢复上报即清除）
        std::uint32_t reconnects = 0;      ///< 累计重连次数（注册时识别）
    };
    std::map<std::string, PlayerEntry> players_;

    /// 【素材同步进度 2026-11-04】各显示端拉取进度（display_id → 视图；
    /// state_mutex_ 保护；handleAssetProgress 写 / assetSyncSnapshot 读，
    /// onDisplayState 断连清除）。last_update_us 用 steady_clock（纯本地
    /// 老化判据，不参与同步语义）。
    std::map<std::string, AssetSyncView> asset_sync_;

    /// UDP 来源地址 → player_id 反查表（上报帧无 player_id 字段，
    /// 按 "ip:port" 键归位——设计 §6"学到注册时的来源地址"）
    std::map<std::string, std::string> udp_addr_to_player_;

    // ---- 出站发送队列与专职发送线程（send_mutex_ 保护）----------------------
    /// 所有出站帧先入队再由 sendWorkerLoop 投递：调用方（GUI 的 cmd*/tick、
    /// recv 的应答闭包）零阻塞——socket 阻塞写（对端接收窗口满可无限期）
    /// 全部发生在发送线程，界面不再被慢/断连接拖死。
    struct SendTask {
        /// 四类通道（语义见文件头【出站语义】）
        enum class Kind {
            DisplayBroadcast, ///< TCP 广播：display 通道全部在线条目
            DisplayUnicast,   ///< TCP 单发：指定 display 槽位（Ack/重连并入）
            UdpSend,          ///< UDP 单发：心跳帧（beat + 目标地址）
            ScenePrepare,     ///< 场景准备+下发（2026-11-04）：素材注册 +
                              ///< serialize + 广播/单发全部在发送线程——
                              ///< 调用方（GUI/recv）只做 Scene 快照入队；
                              ///< display_id 空 = 广播，非空 = 单播该槽位
        };
        Kind kind = Kind::DisplayBroadcast;
        std::string   display_id;         ///< DisplayUnicast / ScenePrepare
                                          ///< （非空时）：connector 槽位
        ms_proto::Envelope env;           ///< TCP 各类载荷（移动传递）
        Scene scene;                      ///< ScenePrepare 载荷（快照深拷贝）
        SyncFrame beat;                   ///< UdpSend 载荷
        Poco::Net::SocketAddress udp_to;  ///< UdpSend 目标地址
        /// 【下发延迟探针】入队时刻（steady ms；enqueueSend 统一盖章）。
        /// 用于把"编辑 → display 收到"的端到端延迟拆成三段：
        /// 排队等待（本值 → 出队）/ 准备（素材注册+serialize）/ 投递（socket 写）
        long long enqueue_ms = 0;
    };
    std::mutex              send_mutex_;
    std::condition_variable send_cv_;
    std::deque<SendTask>    send_queue_;   ///< FIFO（同目标保序：Ack→Scene）
    bool                    send_quit_ = false; ///< 退出标志（send_mutex_ 保护）
    std::thread             send_thread_;  ///< 专职发送线程（start 末启动）
    /// 软上限：积压超过此值丢 UDP 心跳类任务（可再生的观测帧），
    /// TCP 语义帧（ReAnchor/ScenePrepare/Ack）不丢——命令低频，正常不触顶
    static constexpr std::size_t kSendQueueLimit = 4096;

    // ---- CRC 就绪防抖补推（2026-11-09；crc_push_mutex_ 保护）--------------
    std::mutex              crc_push_mutex_;
    std::condition_variable crc_push_cv_;
    bool                    crc_push_pending_ = false; ///< 待补推事件（有新 CRC 就绪）
    std::atomic<bool>       crc_push_quit_{false};     ///< 退出标志（stop 置位）
    std::thread             crc_push_thread_;          ///< 防抖补推线程（start 启动）

    std::atomic<bool> running_{false};
};

} // namespace ms
