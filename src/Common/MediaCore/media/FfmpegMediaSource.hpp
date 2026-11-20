// ============================================================================
//  FfmpegMediaSource.hpp —— FFmpeg 视频解码源（IMediaSource 首个具体实现）
//
//  设计依据：《media-server-sync-design.md》§4.7 扩展点 + §5.6 渲染管线
//    * 实现核心/渲染引擎 (core/engine/MediaFactory.hpp) 的 IMediaSource 接口：
//      prepare(素材) 打开容器 → produce(media_us) 按素材内部时间产出当前帧
//      → release() 释放全部底层资源；
//    * 上层（渲染调度器 Renderer/Scheduler）对"这是视频"零感知，只按时间
//      取帧——新增图片/音频/文本媒体类型时各自实现 IMediaSource 并注册工厂。
//
//  解码策略（produce 时间语义）：
//    * media_us 为【素材内部时间】（相对媒体起点的微秒，即 clip.in_us 加上
//      播放头偏移后的值——时间轴裁剪由 TimelineEngine 完成，本类只负责
//      "给定素材内时间，给出该时刻画面"）；
//    * 前向播放（media_us 落在当前帧之后）：顺序逐帧解码，跳过的帧不解码
//      内容直接丢弃（避免 seek 频繁击中关键帧搜索；短视频/直播窗口最优）；
//    * 回退（media_us 落在当前帧之前）：av_seek_frame 回退到目标前的最近
//      关键帧 + 解码器冲洗 (avcodec_flush_buffers)，再顺序解码到目标；
//    * 帧内时刻（media_us 落在当前帧显示时长内）：缓存命中，不解码直接返回
//      （30fps 素材在 60fps 渲染时钟下约半数 produce 调用命中缓存）。
//
//  输出格式：统一转换为 RGBA（AV_PIX_FMT_RGBA，8bit 四通道），行主序，
//    供后续 Renderer 直接上传 GPU 纹理或写系统内存画布。
//
//  线程约束：produce()/prepare()/release() 由解码驱动线程单线程串行调用
//    （解码上下文非线程安全，内部解码状态不加锁）；frameView() 供渲染线程
//    任意时刻调用（读原子发布槽，与 produce 并发安全——见 PublishedFrame）。
//
//  头文件卫生：FFmpeg 为 C 库，本头仅前置声明句柄类型（AVRational 拆为
//    两个 int 成员），FFmpeg 头只在 .cpp 中包含——避免污染公共头与
//    MSVC 全局宏泄漏。
// ============================================================================

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "engine/MediaFactory.hpp"   // IMediaSource 基类 + Asset 模型
#include "media/HwAccel.hpp"         // 硬解后端配置（第①阶段硬解抽象层）

// FFmpeg 句柄类型前置声明（完整定义见 libavformat/libavcodec/libswscale）
struct AVFormatContext;
struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVBufferRef;
struct SwsContext;

namespace ms {

// ---------------------------------------------------------------------------
// PublishedFrame —— 已完成解码的不可变帧快照（解码/渲染双线程交付单元）
//
// 【渲染架构：解码与渲染线程分离】produce（解码驱动线程调用）完成一帧后
// 以 shared_ptr 快照发布；frameView（渲染线程调用）读最近发布帧。不可变
// 语义：发布后无人再写该缓冲——渲染线程绘制期间解码线程可继续发布新帧
// （旧帧由 shared_ptr 引用计数保活到绘制结束），零锁零悬垂。
// 代价：每帧一次像素缓冲分配（1080p ≈ 8MB，30fps 下 ~2% 单核）——
// 相比"渲染线程等解码"的结构性阻塞（多路视频时帧率崩塌、首帧延迟秒级）
// 完全值得；GL 后端阶段此处将直接换纹理句柄，分配消失。
// ---------------------------------------------------------------------------
struct PublishedFrame {
    std::vector<std::uint8_t> rgba;   ///< RGBA 像素（行主序，stride 行距；
                                      ///< 仅 CpuRgba 域使用）
    std::vector<std::uint8_t> dxt;    ///< DXT 压缩纹理裸块（行主序块排列；
                                      ///< 仅 CpuDxt 域使用，HAP DXT 直传
                                      ///< 2026-11-17——GPU 侧
                                      ///< glCompressedTexSubImage2D 直传，
                                      ///< 采样时硬件解压，CPU 零像素工作）
    int           width  = 0;         ///< 像素宽
    int           height = 0;         ///< 像素高
    std::uint32_t stride = 0;         ///< 行字节数（CpuRgba = width*4；
                                      ///< GpuCudaNv12 = 亮度平面行距；
                                      ///< CpuDxt = 压缩块行距）
    std::uint32_t dxt_format = 0;     ///< DXT 纹理格式（CpuDxt 域有效；
                                      ///< GL 常量 0x83F0/0x83F3）
    std::int64_t  pts_us = 0;         ///< 帧时间戳（素材内部时间，us）

    // ---- 帧域（第③阶段 GPU 帧交付）---------------------------------------
    FrameDomain domain = FrameDomain::CpuRgba; ///< CPU RGBA / GPU NV12 / CPU DXT
    /// GPU 原生帧引用持有者（GpuCudaNv12 域有效）：类型擦除的 shared_ptr，
    /// 实际指向 AVFrame（av_frame_clone 引用 +1 的硬件帧容器，像素留显存；
    /// shared_ptr 释构时 av_frame_free 引用归零 → 解码器可复用该表面）。
    /// 渲染端（GL 后端 CUDA interop）经 static_pointer_cast<const AVFrame>
    /// 取帧——display 层可见 FFmpeg 头，core 层保持类型擦除零依赖。
    std::shared_ptr<const void> native;
};

class FfmpegMediaSource final : public IMediaSource {
public:
    // \param assets_root 素材根目录（UTF-8，可空）；非空时与 asset.path
    //        （相对路径，'/' 分隔）拼接为完整路径；为空时 asset.path 原样
    //        使用（测试直接传绝对路径/相对工作目录路径）。
    // \param kind 素材种类（工厂注册键）：视频与图片统一由 FFmpeg 解码
    //        （PNG/JPG/BMP/GIF 本质是"单帧媒体"——image2 demuxer + 对应
    //        解码器走与视频完全相同的打开/解码/转 RGBA 路径），kind 仅用于
    //        工厂注册键与调度器日志；单帧媒体的越界钳制末帧语义（produce
    //        任意时刻返回该帧）天然适配静态图"任意时长显示"的需求。
    explicit FfmpegMediaSource(std::string assets_root = "",
                               AssetKind kind = AssetKind::Video,
                               HwAccelConfig hw = {});

    // 析构释放全部 FFmpeg 资源（RAII 兜底，正常路径应显式 release()）
    ~FfmpegMediaSource() override;

    // ---- IMediaSource 接口 -------------------------------------------------
    AssetKind kind() const noexcept override { return kind_; }
    bool prepare(const Asset& asset) override;
    bool produce(std::int64_t media_us) override;
    bool frameView(FrameView& out) const override;
    void release() override;

    // ---- 当前帧视图（produce 返回 true 后有效；Renderer 消费）--------------
    // 【发布槽语义】下列访问器均读"最近发布帧"（与 frameView 同源）——
    // 解码线程化后像素所有权随发布移交，rgba_ 成员只是解码工作缓冲。
    // 指针有效期至下一次 produce 发布覆盖（渲染线程应经 frameView 的
    // keep 保活；测试/单线程路径 produce 后立即读安全）。

    /// 当前帧像素宽/高（prepare 后即有值；动态分辨率流以最近解码帧为准）
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }

    /// 最近发布帧 RGBA 数据指针（行主序；空指针表示尚无帧——未 produce 过）
    const std::uint8_t* rgba_data() const noexcept {
        const auto pf = published_.load(std::memory_order_acquire);
        return pf ? pf->rgba.data() : nullptr;
    }

    /// 每行字节数（stride = width * 4，无行尾对齐填充）
    std::size_t rgba_stride() const noexcept {
        const auto pf = published_.load(std::memory_order_acquire);
        return pf ? pf->stride : 0;
    }

    /// 最近发布帧的时间戳（素材内部时间，us）——与 produce(media_us) 同一坐标系
    std::int64_t frame_pts_us() const noexcept {
        const auto pf = published_.load(std::memory_order_acquire);
        return pf ? pf->pts_us : 0;
    }

    /// 素材总时长（us；容器未报告时为 0，调用方以 Asset.duration_us 兜底）
    std::int64_t duration_us() const noexcept { return duration_us_; }

    /// 容器级时长探测（us；静态工具，不建解码上下文）。纯音频文件无视频
    /// 流，prepare 的 av_find_best_stream(VIDEO) 必失败——音频导入探测
    /// 不能复用实例路径，走这里只开容器读头部（与 Video 分支"打开容器
    /// 即返回"同一容错策略：失败/未报告返回 0，不阻断导入）。
    /// \param utf8_path 文件完整路径（UTF-8 字节流，与 prepare 同约定）
    static std::int64_t probeDurationUs(const std::string& utf8_path);

    /// 诊断：累计成功解码的帧数（测试验证"缓存命中不重复解码"）
    std::uint64_t decoded_count() const noexcept { return decoded_count_; }

    /// 诊断：硬解是否实际生效（prepare 后可查——配置了硬解但探测失败
    /// 自动回退软解时为 false；false = 行为与纯软解完全一致）
    bool hw_active() const noexcept { return hw_active_; }

    /// 诊断：实际生效的硬解后端（hw_active() 为 true 时有意义）
    HwAccelType hwBackend() const noexcept { return hw_type_; }

    /// 诊断：HAP DXT 直传是否实际生效（prepare 后可查——HAP 素材 + 开关
    /// 开启时为 true；false = 行为与既有 avcodec 全解压路径完全一致）
    bool dxt_active() const noexcept { return hap_active_; }

private:
    /// 解码下一帧并转换为 RGBA 缓存；\return false 表示流结束/解码失败
    ///（EOF 时置 at_eof_，produce 据此扩展缓存命中区间到播尾）
    bool decodeNext();

    /// seek 到素材内部时间 target_us 之前的最近关键帧（并冲洗解码器）。
    /// seek 失败但旧帧仍有效时恢复旧帧（fail-safe 停留，不黑屏）
    bool seekTo(std::int64_t target_us);

    /// 把解码出的原始帧（frame_）转换为 RGBA 写入 rgba_ 缓冲
    /// \return false = 转换器创建/像素转换失败（当前帧不可用）
    bool convertToRgba();

    /// 当前帧补发布（produce/seekTo 的重发布点统一入口，域分发）：
    /// HAP 直传帧在 dxt_ 工作缓冲（HapDecode 无 AVFrame 可回溯，重发布=
    /// 重发 dxt_ 拷贝）；avcodec 路径帧在 frame_（重转换写 rgba_ 再发布）。
    /// \return false = 发布失败（HAP 恒成功；RGBA 路径=转换失败）
    bool publishCurrentFrame();

    /// 第①阶段硬解：按 hw_cfg_.type 探测并挂接硬解设备（在 avcodec_open2
    /// 之前调用）。任一步失败静默清理并保持软解路径（不改变既有行为）。
    /// \param dec 已找到的解码器（枚举其 hw_configs 判定后端支持）
    void tryEnableHwAccel(const AVCodec* dec);

    /// 硬解连续失败自愈（2026-11-20 视频不刷新排查）：硬解生效期间解码/
    /// 下载连续失败达阈值 → 禁用硬解（意图改 Soft）并用保存的素材重建
    /// 解码上下文（软解自愈）——覆盖驱动持续 transfer 失败 / NVDEC 表面
    /// 池异常 / 跨 GPU 拷贝坏等机器相关问题（软解多线程永远能跑）。
    /// 软解/HAP 路径不计数不触发；阈值内单帧失败仍按既有语义丢弃重试。
    /// \param why 触发原因（日志归因）
    void handleHwFailure(const char* why);

    // ---- HAP DXT 直传（2026-11-17 阶段1）-----------------------------------
    // HAP 全 I 帧 + GPU 压缩纹理：av_read_frame 的包即 HAP 帧比特流，
    // Vidvox HapDecode() 直接产出裸 DXT 块（跳过 avcodec_open2/解码器/
    // sws 全链），渲染端 glCompressedTexSubImage2D 直传 GPU。解码侧
    // 收益实测 8.6~15.6×（2026-11-17 阶段 0 对拍验证，工具已移除）。
    //
    /// 把当前 packet（HAP 帧比特流）经 HapDecode 解到 dxt_ 工作缓冲。
    /// \return false = HapDecode 失败（坏帧/缓冲不足；调用方按解码失败处理）
    bool hapDecodePacket();

    /// 发布当前 HAP 帧（dxt_ 工作缓冲 → 发布槽；拷贝语义——HAP 帧数据
    /// 小（~0.3-1.6MB），拷贝 <0.3ms，保留源侧数据供 produce 补发布重发，
    /// 与 rgba 路径的 move 语义不同：HapDecode 无 AVFrame 可回溯重转换）
    void publishDxtFrame();

    /// HAP 直传开关（静态）：环境变量 MS_DXT_PASSTHROUGH=0 关闭（部署端
    /// 零配置默认开启；出问题设 0 回退既有 avcodec 全解压路径）。
    /// 【静态】环境读取一次缓存——所有实例同策略，避免逐实例 getenv。
    static bool dxtPassthroughEnabled();
    // 注：HapDecode 的 chunk 回调（串行同步执行）为纯实现细节，类型
    // HapDecodeWorkFunction 是第三方 typedef——按本头"FFmpeg 等第三方 C 头
    // 只进 .cpp"的卫生原则，回调为 .cpp 内 static 自由函数，不入头文件。

    // ---- 装配参数 ----
    std::string assets_root_;   ///< 素材根目录（UTF-8；空 = 直接用 asset.path）
    AssetKind   kind_;          ///< 素材种类（工厂注册键；Video/Image 共用本解码器）
    HwAccelConfig hw_cfg_;      ///< 硬解意图（实际生效见 hw_active_）

    // ---- 硬解失败自愈状态（2026-11-20；仅解码驱动线程触碰，无锁）----
    Asset last_asset_;         ///< 最近一次 prepare 的素材（自愈降级重建快照）
    bool has_last_asset_ = false; ///< last_asset_ 是否有效（prepare 后为 true）
    int  hw_fail_streak_ = 0;  ///< 硬解生效期间连续解码/下载失败计数
                               ///<（成功一帧清零；软解路径恒 0 不触发）

    // ---- FFmpeg 上下文（句柄，完整类型仅 .cpp 可见）----
    AVFormatContext* fmt_ = nullptr;     ///< 容器封装上下文
    AVCodecContext*  codec_ = nullptr;   ///< 视频解码器上下文
    AVFrame*         frame_ = nullptr;   ///< 解码输出原始帧（复用；硬解时为
                                         ///< GPU 帧容器，像素在显存）
    AVPacket*        pkt_ = nullptr;     ///< 读取的压缩包（复用）
    SwsContext*      sws_ = nullptr;     ///< 像素格式转换（源格式 → RGBA）
    int              sws_src_fmt_ = -1;  ///< 转换器当前源格式（硬解=NV12/
                                         ///< 软解=yuv420p 变化时重建转换器）
    AVBufferRef*     hw_device_ = nullptr; ///< 硬解设备上下文引用（空=软解）
    AVFrame*         sw_frame_ = nullptr; ///< 硬解帧下载缓冲（transfer 目标，
                                          ///< 复用；仅 hw_active_ 时使用）
    int  hw_pix_fmt_ = 0;                ///< 实际选定的硬解像素格式
                                         ///< （AVPixelFormat，int 存储保持头文件干净）
    bool hw_active_ = false;             ///< 硬解是否实际生效（探测失败=软解）
    HwAccelType hw_type_ = HwAccelType::Soft; ///< 实际生效的后端

    // ---- HAP DXT 直传状态（hap_active_ = true 时启用；互斥于 hw/软解路径）----
    bool hap_active_ = false;            ///< HAP 直传生效（prepare 探测置位；
                                         ///< 解码走 HapDecode 旁路，不建
                                         ///< avcodec 解码器上下文）
    std::vector<std::uint8_t> dxt_;      ///< 当前帧裸 DXT 块工作缓冲
                                         ///<（发布为拷贝语义，常驻复用）
    std::uint32_t dxt_format_ = 0;       ///< 当前帧 DXT 纹理格式（GL 常量）
    std::uint32_t dxt_stride_ = 0;       ///< 压缩块行距（字节）
    std::size_t   dxt_max_bytes_ = 0;    ///< 输出缓冲容量（块数×16B 上限）

    // ---- 流元数据（prepare 时填充）----
    int stream_index_ = -1;      ///< 视频流在容器中的索引
    int width_  = 0;             ///< 视频像素宽
    int height_ = 0;             ///< 视频像素高
    int tb_num_ = 0;             ///< 流时间基分子（AVRational 拆存，保持头文件干净）
    int tb_den_ = 0;             ///< 流时间基分母
    std::int64_t start_pts_ = 0; ///< 媒体起点时间戳（流时间基单位；start_time 校正）
    std::int64_t duration_us_ = 0;  ///< 素材总时长（us）
    std::int64_t frame_span_us_ = 33'333; ///< 单帧显示时长（us，由帧率推得）

    // ---- 解码状态（produce/decodeNext 维护）----
    std::vector<std::uint8_t> rgba_;  ///< 当前帧 RGBA 像素缓冲
    std::size_t  stride_ = 0;         ///< RGBA 行字节数
    std::int64_t frame_pts_us_ = 0;   ///< 当前帧时间戳（素材内部时间，us）
    bool         has_frame_ = false;  ///< 是否已持有有效当前帧
    bool         flushed_ = false;    ///< 解码器已进入 drain 状态（EOF 后置位）
    bool         at_eof_ = false;     ///< 已消费到流尾（此后无更多帧；
                                       ///<  produce 缓存命中区间扩展为 [pts,+∞)）
    std::int64_t seek_base_us_ = 0;   ///< 无 PTS 流的 seek 递推基准（us）：
                                      ///<  seekTo 目标——无 PTS 首帧以此建立
                                      ///<  递推起点（误差 ≤ 一个 GOP）
    bool         seek_pending_ = false; ///< seek 后尚未用基准建立首帧 pts
    /// 单图素材旗标（2026-11-05 图片不显示根因修复）：png_pipe/jpeg_pipe
    /// 等管道 demuxer 与 image2（单文件命中）——整流仅一帧、无时间轴，
    /// av_seek_frame 不可靠（png_pipe 仅 ts=0 能回卷；image2 返回假成功
    /// 不回卷）。置位后：prepare 期直接解出唯一一帧，produce 期任意
    /// media_us 恒命中该帧（与"播尾停留"语义一致，绕过全部 seek 路径）。
    /// 动画 GIF（gif demuxer）不在判定内，保留逐帧动画语义。
    bool         single_frame_media_ = false;
    std::uint64_t decoded_count_ = 0; ///< 累计解码帧数（诊断）
    bool         suppress_publish_ = false; ///< 发布抑制（2026-11-05 暂停
                        ///< "快放"修复）：produce 补齐循环期间置位——
                        ///< 中间帧只推进时间轴（PTS/has_frame_），不转
                        ///< 像素不进发布槽，渲染端不会看到逐帧"快进"；
                        ///< 循环结束对目标帧统一转换+发布。仅写端线程
                        ///< 触碰（produce 与 decodeNext 同线程串行）。

    // ---- 帧发布槽（解码→渲染的跨线程交付；见 PublishedFrame 注释）----
    // 写端：produce/convertToRgba（解码驱动线程，或测试单线程）；
    // 读端：frameView（渲染线程任意时刻）。atomic<shared_ptr>（C++20）：
    // store(release) / load(acquire) 无锁交接快照所有权。
    // 解码状态成员（rgba_/frame_pts_us_/has_frame_ 等）仅写端线程触碰，
    // 与本槽无共享（发布 = 所有权 move，写端不再读已发布缓冲）。
    // 【Android 兼容 2026-11-21】NDK libc++ 未实现 std::atomic<shared_ptr>
    // （P0718，非 trivially copyable 直接 static_assert 拒绝）——退化为
    // 互斥保护的快照槽，接口兼容（order 参数收下但由锁保证序）。
#ifdef __ANDROID__
    class AtomicSharedSlotPub {
    public:
        void store(std::shared_ptr<const PublishedFrame> v,
                   std::memory_order = std::memory_order_release) {
            std::lock_guard<std::mutex> lk(m_);
            v_ = std::move(v);
        }
        std::shared_ptr<const PublishedFrame> load(
            std::memory_order = std::memory_order_acquire) const {
            std::lock_guard<std::mutex> lk(m_);
            return v_;
        }
    private:
        mutable std::mutex m_;
        std::shared_ptr<const PublishedFrame> v_;
    };
    AtomicSharedSlotPub published_;
#else
    std::atomic<std::shared_ptr<const PublishedFrame>> published_;
#endif
};

} // namespace ms
