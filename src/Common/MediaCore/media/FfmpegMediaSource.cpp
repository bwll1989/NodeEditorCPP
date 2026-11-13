// ============================================================================
//  FfmpegMediaSource.cpp —— FFmpeg 视频解码源实现
//
//  FFmpeg 8.x C API 使用要点（对照代码）：
//    * 现代解码循环（send_packet / receive_frame，3.x 起标准范式）：
//          avcodec_send_packet(codec, pkt)   —— 喂入压缩数据
//          avcodec_receive_frame(codec, fr)  —— 取出解码帧
//      两端均可能返回 EAGAIN（对端需先动作）——本实现的 decodeNext()
//      以 receive 为驱动：EAGAIN 时读包喂入，EOF 时 flush 解码器收尾。
//    * 时间基（time_base）：流级 time_base 是最小时间单位（如 1/15360），
//      本类对外统一【微秒】坐标系，边界处用 av_rescale_q 整数换算
//      （避免浮点误差累积）。
//    * seek 精度：H.264 等编码只能从关键帧起完整解码——av_seek_frame
//      携带 AVSEEK_FLAG_BACKWARD 落到目标前最近关键帧，再顺序解码补齐。
//    * 帧时间戳：优先 best_effort_timestamp（FFmpeg 依容器信息推得的
//      可靠 PTS），缺失退化 frame_->pts，再缺失按帧时长递推。
//
//  日志：诊断统一 fprintf(stderr, "[media] ...")（与 net/asset 层一致；
//  spdlog 引入后统一迁移）。
// ============================================================================

#include "media/FfmpegMediaSource.hpp"

#include "runtime/AssetPath.hpp" // 素材归集寻址（2026-11-13）

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <system_error>

// FFmpeg C 头（自身带 __BEGIN_DECLS 的 C++ 保护；显式包裹 extern "C" 更保险）
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
}

namespace ms {

// 前向顺序解码的最大跨度（帧数）：media_us 距当前帧 ≤12 帧时直接顺序解码
// 补齐而不 seek——短步进 seek 命中关键帧搜索 + 解码器冲洗反而更贵
// （约 0.4s@30fps）。超过该跨度走 seekTo 关键帧回退分支。
constexpr std::int64_t kMaxForwardDecodeFrames = 12;

// 【回退容差（2026-11-05"瞬发卡顿、越往后越严重"根因修复）】
// 两套时间换算（会话栅格 av_rescale_q vs FFmpeg PTS best_effort 换算）
// 的 floor 舍入可差 1us——实测 media_us=166666 vs pts=166667（差 -1us）
// 时，下方"media_us >= frame_pts_us_"两处判定全部失效，1 微秒的"回退"
// 被当成真回退送进 seekTo：BACKWARD 落 GOP 头关键帧后顺序重解到目标
// （CrabRave GOP=250 帧，实测补齐 5→44 帧随播放位置线性增长，解码拍
// 60~190ms 持续超限）——渲染端补齐期间停旧帧 = 每 200~300ms 一次的
// 瞬发卡顿，且越往后越严重。HAP（全 I 帧）seekTo 落点即目标帧、补齐
// 0 帧，故用户实测 HAP 免疫。
// 容差语义：目标比当前帧 pts 旧 ≤ kBackwardTolUs 时视为"仍在当前帧"
// （显示当前帧，视觉误差 ≤ 4ms 远低于感知阈值），不重解。
constexpr std::int64_t kBackwardTolUs = 4'000;

// ---------------------------------------------------------------------------
// 【瞬卡探针 2026-11-05】produce 分支观测
//
// 定位目标："越往后越严重的瞬时卡顿、HAP 免疫"。
// 稳态播放的期望形态 = 绝大多数调用走缓存命中，每越过一个帧界一次
// 前向小步（补 1~2 帧，1~4ms）；若出现 seekTo，说明 media_us 相对当前
// 帧跳差超过 12 帧（200ms@60fps）——要么会话时钟/播放头跳变，要么解码
// 吞吐追不上实时（差值持续累积 → 每拍全 GOP 重解 → 越追越远，与用户
// 症状完全吻合）。
//
// 输出两种打点（全部 MS_CANVAS_DBG=1 门控，常态零开销）：
//   * [probe] 5s 汇总：三分支增量计数（seek 增量 >0 即异常信号）；
//   * [probe] seek 单次详打（200ms 节流防风暴）：跳差/补齐帧数/耗时
//     ——跳差一次性大跳 = 时钟跳变；持续中小跳差 = 追帧恶性循环。
// 计数为进程级（多源实例共享——观测总量足够）。
// ---------------------------------------------------------------------------
const bool g_probe_on = std::getenv("MS_CANVAS_DBG") != nullptr;
std::atomic<std::uint64_t> g_prod_cache{0}; ///< 缓存命中次数（窗口增量）
std::atomic<std::uint64_t> g_prod_fwd{0};   ///< 前向小步次数
std::atomic<std::uint64_t> g_prod_seek{0};  ///< seekTo 次数（稳态应≈0）

/// 当前单调时钟毫秒（探针节流/计时共用）
static std::int64_t probeNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

/// 5s 窗口汇总打点（produce 三分支末尾各调用一次；CAS 抢窗口防并发重复）
static void probeSummaryTick() {
    static std::atomic<std::int64_t> s_last_ms{0};
    const std::int64_t now = probeNowMs();
    std::int64_t expected = s_last_ms.load(std::memory_order_relaxed);
    if (now - expected < 5000) return;
    // CAS 抢窗口：多实例并发调用时只有一个线程真正打印（其余正常计数）
    if (!s_last_ms.compare_exchange_strong(expected, now)) return;
    const std::uint64_t c = g_prod_cache.exchange(0, std::memory_order_relaxed);
    const std::uint64_t f = g_prod_fwd.exchange(0, std::memory_order_relaxed);
    const std::uint64_t s = g_prod_seek.exchange(0, std::memory_order_relaxed);
    std::fprintf(stderr,
                 "[probe] 5s产出分支: cache=%llu fwd=%llu seek=%llu%s\n",
                 static_cast<unsigned long long>(c),
                 static_cast<unsigned long long>(f),
                 static_cast<unsigned long long>(s),
                 s > 0 ? "  ←★稳态不应出现 seek！" : "");
    std::fflush(stderr);
}

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------
FfmpegMediaSource::FfmpegMediaSource(std::string assets_root, AssetKind kind,
                                     HwAccelConfig hw)
    : assets_root_(std::move(assets_root)), kind_(kind), hw_cfg_(hw) {}

FfmpegMediaSource::~FfmpegMediaSource() {
    release(); // RAII 兜底：全部 FFmpeg 句柄安全释放
}

// ---------------------------------------------------------------------------
// get_format 回调 —— 解码器从候选像素格式中选出硬解输出格式
//
// codec->opaque 携带目标 hw 像素格式（int 化的 AVPixelFormat）：回调在
// avcodec_open2 期间被解码器调用一次，从驱动实际支持的候选列表里挑出
// 我们要的后端。未命中时返回列表首个格式（软件格式）——驱动侧异常时
// 自然退化为软解输出，而非打开失败（尽力播放优先）。
// ---------------------------------------------------------------------------
static enum AVPixelFormat pickHwPixFmt(AVCodecContext* ctx,
                                       const enum AVPixelFormat* fmts) {
    const int target = static_cast<int>(
        reinterpret_cast<intptr_t>(ctx->opaque));
    for (const enum AVPixelFormat* p = fmts; *p != AV_PIX_FMT_NONE; ++p)
        if (static_cast<int>(*p) == target) return *p; // 命中硬解格式
    // 兜底①：选第一个【软件】格式而非盲目 fmts[0]——候选列表里硬件
    // 格式通常排在前面，fmts[0] 可能是"另一个"硬解格式（如请求 CUDA
    // 但候选只给了 VAAPI）：返回它帧会落进未预期的 GPU 容器，下游
    // convertToRgba 按软解路径 sws 处理必然失败。软件格式才是安全回退
    // （尽力播放优先：驱动侧异常时自然退化为软解输出）。
    for (const enum AVPixelFormat* p = fmts; *p != AV_PIX_FMT_NONE; ++p) {
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(*p);
        if (desc && !(desc->flags & AV_PIX_FMT_FLAG_HWACCEL)) return *p;
    }
    return fmts[0]; // 理论不可达：解码器候选列表必含软件格式（最后防线）
}

// ---------------------------------------------------------------------------
// tryEnableHwAccel —— 第①阶段硬解抽象层：探测并挂接硬解设备
//
// 步骤（任一步失败 → 清理并静默回退软解，不改既有行为）：
//   1. 由 hw_cfg_.type 构造候选链（Auto = 平台探测链，逐个尝试）；
//   2. 遍历解码器 avcodec_get_hw_config：确认其支持该后端
//      （HW_CONFIG_METHOD_HW_DEVICE_CTX + 像素格式匹配）；
//   3. av_hwdevice_ctx_create 创建设备上下文（FFmpeg 动态加载驱动侧
//      API——D3D11VA 用系统 d3d11.dll、CUDA 用驱动 nvcuvid/libcuda，
//      无需任何 SDK 安装，部署机装显卡驱动即可）；
//   4. 挂接 codec_->hw_device_ctx（引用 +1）与 get_format 回调。
// 生效后 decodeNext 收到的 frame_->format == hw_pix_fmt_（GPU 帧容器），
// 由 convertToRgba 统一下载回系统内存——渲染侧零感知。
// ---------------------------------------------------------------------------
void FfmpegMediaSource::tryEnableHwAccel(const AVCodec* dec) {
    if (hw_cfg_.type == HwAccelType::Soft || dec == nullptr) return;

    // ---- 1. 候选链：显式类型单候选；Auto 按平台展开探测顺序 ----
    HwAccelType chain[2] = {HwAccelType::Soft, HwAccelType::Soft};
    int chain_n = 0;
    if (hw_cfg_.type == HwAccelType::Auto) {
#if defined(_WIN32)
        // Windows：D3D11VA（全厂商）优先，NVIDIA 机器再试 CUDA/NVDEC
        chain[chain_n++] = HwAccelType::D3D11VA;
        chain[chain_n++] = HwAccelType::Cuda;
#elif defined(__ANDROID__)
        chain[chain_n++] = HwAccelType::MediaCodec; // Android 唯一系统后端
#else
        // Linux/其他：VAAPI（Intel/AMD）优先，NVIDIA 机器退 CUDA
        chain[chain_n++] = HwAccelType::Vaapi;
        chain[chain_n++] = HwAccelType::Cuda;
#endif
    } else {
        chain[chain_n++] = hw_cfg_.type; // 显式指定：单候选
    }

    // 逐候选失败原因留痕（两类"不可用"处置完全不同）：
    //   A. 解码器不提供该硬解后端（avcodec_get_hw_config 枚举不到）——
    //      编码格式的固有属性（如 HAP 仅 CPU 解码、MPEG2 无 D3D11VA 变体），
    //      换 hw_accel 配置也无效；
    //   B. 设备创建失败（av_hwdevice_ctx_create）——驱动/环境问题
    //      （无 N 卡配 cuda、驱动过旧、会话不支持），换后端或装驱动可解。
    char why[256] = "无候选后端";
    for (int i = 0; i < chain_n; ++i) {
        // ---- 后端 → FFmpeg 像素格式 / 设备类型映射 ----
        enum AVPixelFormat pix = AV_PIX_FMT_NONE;
        enum AVHWDeviceType dev = AV_HWDEVICE_TYPE_NONE;
        switch (chain[i]) {
            case HwAccelType::D3D11VA:
                pix = AV_PIX_FMT_D3D11; dev = AV_HWDEVICE_TYPE_D3D11VA; break;
            case HwAccelType::Cuda:
                pix = AV_PIX_FMT_CUDA;  dev = AV_HWDEVICE_TYPE_CUDA;    break;
            case HwAccelType::Vaapi:
                pix = AV_PIX_FMT_VAAPI; dev = AV_HWDEVICE_TYPE_VAAPI;   break;
            case HwAccelType::MediaCodec:
                pix = AV_PIX_FMT_MEDIACODEC;
                dev = AV_HWDEVICE_TYPE_MEDIACODEC;                      break;
            default:
                continue; // Soft/Auto 不作为具体候选
        }

        // ---- 2. 解码器能力：是否提供该后端的 hw 配置 ----
        bool supported = false;
        for (int k = 0;; ++k) {
            const AVCodecHWConfig* cfg = avcodec_get_hw_config(dec, k);
            if (cfg == nullptr) break; // 枚举尽：解码器不支持该后端
            if ((cfg->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) != 0 &&
                cfg->pix_fmt == pix) {
                supported = true;
                break;
            }
        }
        if (!supported) {
            std::snprintf(why, sizeof(why),
                          "解码器 %s 不提供 %s 硬解（该编码格式通常仅能"
                          " CPU 解码，换 hw_accel 配置无效）",
                          dec->name != nullptr ? dec->name : "?",
                          HwAccelConfig::typeName(chain[i]));
            continue;
        }

        // ---- 3. 创建设备上下文（失败 = 环境无该硬解能力，试下一候选）----
        AVBufferRef* dev_ref = nullptr;
        if (av_hwdevice_ctx_create(&dev_ref, dev, nullptr, nullptr, 0) < 0) {
            std::snprintf(why, sizeof(why),
                          "%s 设备创建失败（驱动/环境无该硬解能力，"
                          "可换 hw_accel 后端或更新显卡驱动）",
                          HwAccelConfig::typeName(chain[i]));
            continue;
        }

        // ---- 4. 挂接解码上下文（调用点在 alloc/parameters 之后、open2 之前）----
        // hw_device_ctx 引用 +1（codec 释放时自动 unref）；get_format 经
        // opaque 携带目标像素格式；设备引用另存 hw_device_ 供 release 兜底。
        hw_device_              = dev_ref;
        hw_pix_fmt_             = static_cast<int>(pix);
        hw_type_                = chain[i];
        hw_active_              = true;
        codec_->hw_device_ctx   = av_buffer_ref(hw_device_);
        codec_->get_format      = pickHwPixFmt;
        codec_->opaque = reinterpret_cast<void*>(
            static_cast<intptr_t>(hw_pix_fmt_));
        std::fprintf(stderr, "[media] 解码路径: %s 硬解（%s）\n",
                     HwAccelConfig::typeName(hw_type_),
                     av_hwdevice_get_type_name(dev));
        return;
    }

    // 全部候选失败：保持软解（行为与未配置硬解完全一致）——
    // 日志带具体原因 + 配置值，用户一眼可辨"格式不支持"还是"环境不支持"
    std::fprintf(stderr, "[media] 解码路径: 软解（%s；当前 hw_accel=%s）\n",
                 why, HwAccelConfig::typeName(hw_cfg_.type));
}

// ---------------------------------------------------------------------------
// prepare —— 打开素材并建立解码上下文（可重复调用：先释放旧上下文）
// ---------------------------------------------------------------------------
bool FfmpegMediaSource::prepare(const Asset& asset) {
    release(); // 幂等重置（AVFrame/AVPacket 复用缓冲随上下文一起重建）

    // ---- 1. 拼接完整路径：素材根目录 + 相对路径（'/' 在 Windows 下通用）----
    // 【2026-11-13 归集寻址】收敛进 runtime/AssetPath.hpp::assetResolvePath
    //——绝对路径素材先探 display 归集副本（assets_root/文件名，下载保存
    // 落点），命中即读副本（display 目录自含分发）；未命中回退原绝对路径
    //（同机常态——原文件在 → 零下载直读）。相对路径仍拼 assets_root
    //（.show/standalone 语义）。原"盘符/UNC 直通"裸分支退役（四处手写
    // 判定收敛单一真相；编码坑统一在 AssetPath.hpp 的 char8_t 构造处理）。
    const std::string full = ms::assetResolvePath(assets_root_, asset.path);

    // ---- 2. 存在性预检（给出比 avformat_open_input 更明确的失败原因）----
    //    protobuf string 恒为 UTF-8：以 char8_t 语义构造 path，
    //    绕开 MSVC 窄字符默认 ANSI 编码的隐式转换坑。
    const std::filesystem::path fsp(reinterpret_cast<const char8_t*>(full.c_str()));
    std::error_code ec;
    if (!std::filesystem::exists(fsp, ec) || ec) {
        std::fprintf(stderr, "[media] 素材文件不存在: %s\n", full.c_str());
        return false;
    }

    // ---- 3. 打开容器 + 探测流信息（编解码参数/时长/帧率在探测后可信）----
    int ret = avformat_open_input(&fmt_, full.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char err[128];
        av_strerror(ret, err, sizeof(err));
        std::fprintf(stderr, "[media] 打开容器失败(%s): %s\n", err, full.c_str());
        return release(), false;
    }
    if (avformat_find_stream_info(fmt_, nullptr) < 0) {
        std::fprintf(stderr, "[media] 流信息探测失败: %s\n", full.c_str());
        return release(), false;
    }

    // ---- 4. 选最优视频流（多视频流时取缺省首选）并查找解码器 ----
    const AVCodec* dec = nullptr;
    stream_index_ = av_find_best_stream(fmt_, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (stream_index_ < 0 || !dec) {
        std::fprintf(stderr, "[media] 无可用视频流/解码器: %s\n", full.c_str());
        return release(), false;
    }
    const AVStream* st = fmt_->streams[stream_index_];

    // ---- 5. 建解码上下文：参数从流 side-data 拷贝（extradata 等自动带上）----
    codec_ = avcodec_alloc_context3(dec);
    if (!codec_) return release(), false;
    if (avcodec_parameters_to_context(codec_, st->codecpar) < 0) {
        std::fprintf(stderr, "[media] 解码器参数拷贝失败(%s): %s\n",
                     dec->name, full.c_str());
        return release(), false;
    }

    // ---- 4.6 硬解探测与挂接（第①阶段硬解抽象层；失败静默回退软解）----
    // 在 avcodec_open2 之前：向解码上下文挂 hw_device_ctx + get_format 回调。
    // 任一步失败（无独显/驱动旧/后端不支持该 codec）均清理干净，保持与
    // 既有软解路径逐位一致的行为。
    tryEnableHwAccel(dec);

    // ---- 4.7 多线程软解（2026-11-05 seek 跳帧修复）----------------------
    // 跳转/暂停的"停顿后瞬移"体感 = 从关键帧顺序补齐到目标帧的真实耗时
    // （1080p GOP 250 实测 224~237ms/次——单帧 2~4ms × 最多百余帧补齐）。
    // frame-level 多线程把补齐流水线并行化（4~6× 吞吐），跳转停顿等比
    // 缩短，是"跳帧感"的最大单项改善。
    //   * 仅软解路径启用：硬解（hw_active_）由 GPU 解码（单帧微秒级）无
    //     此瓶颈，且 FFmpeg 硬解与 frame-threading 不兼容（open2 直接拒绝）；
    //   * thread_count=0 = 按 CPU 核数自动；FF_THREAD_FRAME 对"从关键帧
    //     连续补齐"的 seek 场景收益最大，代价 n-1 帧流水线延迟——补齐
    //     循环按 pts 判断收尾、输出帧序与串行一致，语义零变化。
    if (!hw_active_) {
        codec_->thread_count = 0; // 0 = 自动（按 CPU 核数）
        codec_->thread_type  = FF_THREAD_FRAME;
    }

    if (avcodec_open2(codec_, dec, nullptr) < 0) {
        std::fprintf(stderr, "[media] 解码器初始化失败(%s): %s\n", dec->name, full.c_str());
        return release(), false;
    }

    // ---- 6. 提取流元数据（尺寸/时间基/帧率/时长/媒体起点）----
    width_  = codec_->width;
    height_ = codec_->height;
    tb_num_ = st->time_base.num;
    tb_den_ = st->time_base.den;
    if (width_ <= 0 || height_ <= 0 || tb_den_ <= 0) {
        std::fprintf(stderr, "[media] 流元数据非法(w=%d h=%d tb=%d/%d)\n",
                     width_, height_, tb_num_, tb_den_);
        return release(), false;
    }
    const AVRational tb{tb_num_, tb_den_};

    // 单帧显示时长：avg_frame_rate 优先（反映真实节拍），退化 r_frame_rate；
    // 均无效时按 30fps 兜底（33.3ms）——仅影响缓存命中判断的精度。
    const AVRational fps = st->avg_frame_rate.num > 0 ? st->avg_frame_rate
                                                      : st->r_frame_rate;
    frame_span_us_ = fps.num > 0
        ? av_rescale_q(1, av_inv_q(fps), AVRational{1, AV_TIME_BASE})
        : 33'333;

    // 时长：流级（time_base 单位）优先，退化容器级（AV_TIME_BASE 微秒）
    if (st->duration != AV_NOPTS_VALUE && st->duration > 0)
        duration_us_ = av_rescale_q(st->duration, tb, AVRational{1, AV_TIME_BASE});
    else if (fmt_->duration != AV_NOPTS_VALUE && fmt_->duration > 0)
        duration_us_ = fmt_->duration;

    // 媒体起点（容器 start_time，AV_TIME_BASE 微秒单位）→ 流时间基，
    // 供 seek 目标换算与帧 PTS → 素材内部时间的偏移校正。
    // testsrc 等常规封装 start_time=0；部分 TS/MOV 流起点非 0，必须校正。
    start_pts_ = (fmt_->start_time != AV_NOPTS_VALUE)
        ? av_rescale_q(fmt_->start_time, AVRational{1, AV_TIME_BASE}, tb)
        : 0;

    // ---- 6.5 单图判定（2026-11-05 图片不显示根因修复）--------------------
    // 管道 demuxer（png_pipe/jpeg_pipe/bmp_pipe/webp_pipe/tiff_pipe…）与
    // image2（单文件 .jpg 常命中）都是"整流仅一帧、无时间轴"的封装：
    //   * av_seek_frame 对 png_pipe 仅 ts=0 能回卷（ts>0 返回失败）；
    //   * 对 image2 更是"返回 0 假成功但流位置仍在 EOF"（最小复现实证）。
    // 后果：播放中首次 produce(media_us>0)（has_frame_=false）走 seekTo
    // 分支 → 必然失败且无旧帧可保 → 图片片段在 display 端永不发布帧
    // （composeFrame 恒 no_frame 占位）。单图特判见 produce() 顶部——
    // 完全绕过缓存区间/补齐/seek 三分支。动画 GIF（gif demuxer）不在
    // 判定内，保留逐帧动画语义。image2 序列（logo_%03d.png）本工程
    // 不导入（makeAssetFromFile 仅单文件选择），不受影响。
    // 【与 kind 的关系】kind_==Image 是必要条件（防御：Video 类素材
    // 理论上不会命中这些 demuxer，但判定保持保守）。
    {
        const char* dmx =
            (fmt_->iformat != nullptr && fmt_->iformat->name != nullptr)
                ? fmt_->iformat->name : "";
        single_frame_media_ = (kind_ == AssetKind::Image) &&
                              (std::strstr(dmx, "image2") != nullptr ||
                               std::strstr(dmx, "_pipe") != nullptr);
    }

    // ---- 7. 复用缓冲：解码帧与压缩包各一块，全程复用（零逐帧分配）----
    frame_ = av_frame_alloc();
    pkt_   = av_packet_alloc();
    if (!frame_ || !pkt_) return release(), false;

    // ---- 8. 单图首帧预解码（2026-11-05）----------------------------------
    // find_stream_info 已消费掉唯一的包（流位置在 EOF），而 av_seek_frame
    // 对 image2 假成功不回卷、对 png_pipe 仅 ts=0 有效——统一用 avio 直卷
    // 文件头（文件式 AVIO 恒可 seek），再解出唯一一帧并发布。此后
    // produce 任意 media_us 恒缓存命中（单图快速路径），图片"任意时长
    // 显示"语义从此处一步到位。prepare 失败 = 素材真不可用（坏图），
    // 显式报错而非静默黑屏。
    if (single_frame_media_) {
        bool ok = false;
        if (fmt_->pb != nullptr && avio_seek(fmt_->pb, 0, SEEK_SET) >= 0) {
            avcodec_flush_buffers(codec_); // 解码器复位（丢弃探测期残留）
            flushed_  = false;
            at_eof_   = false;
            ok = decodeNext(); // 转换 + 发布（suppress_publish_ 未置位）
        }
        if (!ok) {
            std::fprintf(stderr, "[media] 图片首帧解码失败: %s\n",
                         full.c_str());
            return release(), false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// produce —— 取素材内部时间 media_us 的当前帧（缓存命中零解码）
//
// 三分支（按代价递增）：
//   1. 缓存命中：target ∈ 当前帧显示区间 [pts, pts+span) → 直接返回；
//   2. 前向小步（≤kMaxForwardDecodeFrames 帧）：顺序解码补齐，无 seek 开销；
//   3. seekTo：BACKWARD 落目标前最近关键帧 + 解码器冲洗 + 顺序补齐。
//
// 时间语义（一致性保证）：CFR（恒定帧率）素材下帧显示区间精确为
//   [i/fps, (i+1)/fps)，无论调用方经"顺序推进"还是"seek 跳变"到达同一
//   media_us，解出的帧唯一（test_seek_consistency.cpp 逐编码固化断言）。
// VFR（可变帧率）已知限制：span 取平均帧率——帧间隔不均时可能多解一帧
//   停在比目标晚的帧上（画面仍取该时刻最近显示帧，不会花屏/卡死）；
//   VFR 素材在媒体服务器场景罕见，精度损失可接受（文档化，不引入
//   逐帧索引的复杂度）。
// ---------------------------------------------------------------------------
bool FfmpegMediaSource::produce(std::int64_t media_us) {
    if (!fmt_ || !codec_) return false; // 未 prepare

    // 越界钳制到 [0, duration)：正常路径由 TimelineEngine 保证在 clip 区间内，
    // 此处防御调用方越界时退化为播尾帧而非返回失败。
    if (media_us < 0) media_us = 0;
    if (duration_us_ > 0 && media_us >= duration_us_) media_us = duration_us_ - 1;

    // ---- 0. 单图快速路径（2026-11-05 图片不显示根因修复）------------------
    // png_pipe/image2 等单图 demuxer 无时间轴（判定与理由见 prepare 步骤
    // 6.5）：prepare 期已解出并发布唯一一帧——任意 media_us 恒显示该帧，
    // 与"播尾停留"EOF 扩展语义一致。绝不走下方缓存区间/前向补齐/seek
    // 三分支：seekTo 对 png_pipe 仅 ts=0 有效、对 image2 假成功——播放中
    // 首次 produce(t>0)（has_frame_=false）会在这里静默失败，图片从此
    // 永不显示（display 端 composeFrame 恒 no_frame，E2E 实证）。
    if (single_frame_media_) {
        if (has_frame_) return true;      // prepare 已发布：恒命中
        if (!decodeNext()) return false;  // 防御：prepare 未解出时补解
        return has_frame_;
    }

    // 缓存命中：当前帧显示区间 [pts-tol, pts+span) 覆盖目标 → 不解码。
    // 【tol 回退容差】两套时间换算的舍入可差 1us（见 kBackwardTolUs 注）：
    // 目标比 pts 旧 ≤ tol 时直接显示当前帧（视觉误差 ≤ 4ms 无感），避免
    // 微量回退被踢进 seekTo 全 GOP 重解。
    // 【EOF 特例】已消费到流尾后无更多帧，显示区间扩展为 [pts, +∞)——
    // 单帧媒体（图片视频）/播尾停留不再每帧重复走 seek+decode 失败路径
    if (has_frame_ &&
        ((at_eof_ && media_us >= frame_pts_us_ - kBackwardTolUs) ||
         (media_us >= frame_pts_us_ - kBackwardTolUs &&
          media_us < frame_pts_us_ + frame_span_us_))) {
        if (g_probe_on) {
            g_prod_cache.fetch_add(1, std::memory_order_relaxed);
            probeSummaryTick();
        }
        return true;
    }

    // 前向小步（≤kMaxForwardDecodeFrames 帧）：顺序解码补齐——
    // 短步进 seek 命中关键帧搜索反而更贵；大跳跃走 seekTo 分支。
    // 【发布抑制（2026-11-05 暂停"快放"修复）】补齐期间中间帧不发布：
    // 此前每解一帧立即进发布槽，渲染端 paint 连续拉到中间帧——暂停/
    // 跳转后画面表现为"先停早一点、再快速播放追到目标位"（HAP 全 I
    // 帧零补齐故无感，H.264 补齐帧数越多快放越明显）。抑制后渲染端
    // 停在旧帧直到目标帧一次性发布（视觉 = 停顿后瞬跳，与时间码语义
    // 一致）。附带收益：中间帧跳过 sws_scale/GPU clone，补齐更快。
    if (has_frame_ && media_us >= frame_pts_us_ &&
        media_us - frame_pts_us_ <= frame_span_us_ * kMaxForwardDecodeFrames) {
        suppress_publish_ = true;
        std::int64_t probe_frames = 0; // 探针：本次补齐帧数
        while (has_frame_ && frame_pts_us_ + frame_span_us_ <= media_us) {
            if (!decodeNext()) break; // EOF：保留最后一帧
            ++probe_frames;
        }
        suppress_publish_ = false;
        if (g_probe_on) {
            g_prod_fwd.fetch_add(1, std::memory_order_relaxed);
            // 补齐 >2 帧 = 解码吞吐追不上实时（稳态每帧界只补 1~2 帧）——
            // 配合 5s 汇总的 fwd 频率判断是否进入"追帧恶性循环"
            if (probe_frames > 2) {
                static std::atomic<std::int64_t> s_last{0};
                const std::int64_t now = probeNowMs();
                std::int64_t prev = s_last.load(std::memory_order_relaxed);
                if (now - prev >= 500 &&
                    s_last.compare_exchange_strong(prev, now)) {
                    std::fprintf(stderr,
                                 "[probe] 前向补齐 %lld 帧（>2 = 解码追不上"
                                 "实时，差 %lldus）\n",
                                 static_cast<long long>(probe_frames),
                                 static_cast<long long>(media_us -
                                                        frame_pts_us_));
                    std::fflush(stderr);
                }
            }
            probeSummaryTick();
        }
        if (has_frame_) convertToRgba(); // 目标帧（最后解出的帧）补发布
        return has_frame_;
    }

    // 回退 / 首次取帧 / 大步前跳：seek 关键帧 + 顺序解码到目标
    // 【关键帧先行（2026-11-05 seek"瞬卡"修复）】GOP 250 帧（CrabRave 实测
    // 关键帧间隔 8.3s）下，seek 目标平均落在 GOP 深处——BACKWARD 落关键帧
    // 后顺序补齐（实测 101 帧 / 563ms，RTX 3050Ti 1080p cuda）：补齐期间
    // 渲染端停在旧帧，用户感知为拖播放头后"顿 ~0.5s 才动"。现改为：
    //   ① seekTo 解出关键帧后【立即复位抑制并发布】——渲染端先显示
    //      GOP 头画面（内容比目标旧 ≤ 一个 GOP，但**立即**有画面）；
    //   ② 再置抑制进入补齐循环——中间帧不发布（不闪快进）；
    //   ③ 到达目标帧统一发布——画面切到精确位置。
    // 视觉：拖动后"瞬间出帧（内容略旧）→ 数百 ms 内追到目标"，与
    // mpv/专业播放器的 seek 体验一致。HAP（全 I 帧）补齐 0 帧，两帧
    // 实为同一帧，行为不变。
    // 【探针】稳态播放不应进入本分支——进入即记录跳差/补齐量/耗时，
    // 区分两类根因：一次性大跳差 = 时钟/播放头跳变（重锚/纠偏 seek）；
    // 持续中等跳差 = 解码追不上实时（差值随时间放大 → 恶性循环）。
    if (g_probe_on) {
        g_prod_seek.fetch_add(1, std::memory_order_relaxed);
        // 【进入即打】seekTo 分支的完整进入状态——跳差/旧帧/原值三项
        // 齐全才能判定"为何缓存命中/前向小步没拦住"（200ms 节流）。
        static std::atomic<std::int64_t> s_enter_last_ms{0};
        const std::int64_t now_ms0 = probeNowMs();
        std::int64_t prev_ms0 = s_enter_last_ms.load(std::memory_order_relaxed);
        if (now_ms0 - prev_ms0 >= 200 &&
            s_enter_last_ms.compare_exchange_strong(prev_ms0, now_ms0)) {
            std::fprintf(stderr,
                         "[probe] →seekTo 进入: media_us=%lld pts=%lld "
                         "跳差=%lldus has_frame=%d span=%lldus\n",
                         static_cast<long long>(media_us),
                         static_cast<long long>(frame_pts_us_),
                         static_cast<long long>(media_us - frame_pts_us_),
                         has_frame_ ? 1 : 0,
                         static_cast<long long>(frame_span_us_));
            std::fflush(stderr);
        }
    }
    const std::int64_t probe_seek_t0 = g_probe_on ? probeNowMs() : 0;
    const bool probe_had_frame = has_frame_;
    const std::int64_t probe_old_pts = frame_pts_us_;
    suppress_publish_ = true;
    if (!seekTo(media_us)) { // seek 失败：有帧则续用旧帧
        suppress_publish_ = false;
        return has_frame_;
    }
    if (has_frame_) {
        // ① 关键帧先行发布：复位抑制 → 转换+发布 GOP 头帧（渲染端立即
        // 可见）；再置抑制保护后续补齐循环的中间帧。
        suppress_publish_ = false;
        convertToRgba();
        suppress_publish_ = true;
    }
    std::int64_t probe_seek_frames = 0; // 探针：seek 后补齐帧数
    while (has_frame_ && frame_pts_us_ + frame_span_us_ <= media_us) {
        if (!decodeNext()) break;
        ++probe_seek_frames;
    }
    suppress_publish_ = false;
    if (g_probe_on) {
        // 单次详打（200ms 节流）：跳差与补齐规模直接判定恶性循环形态
        static std::atomic<std::int64_t> s_last_ms{0};
        const std::int64_t now = probeNowMs();
        std::int64_t prev = s_last_ms.load(std::memory_order_relaxed);
        if (now - prev >= 200 && s_last_ms.compare_exchange_strong(prev, now)) {
            const std::int64_t diff =
                probe_had_frame ? media_us - probe_old_pts : 0;
            std::fprintf(stderr,
                         "[probe] seekTo: 跳差=%+lldms 补齐=%lld 帧 "
                         "耗时=%lldms (media=%lldms pts=%s)\n",
                         static_cast<long long>(probe_had_frame
                                                    ? diff / 1000
                                                    : 0),
                         static_cast<long long>(probe_seek_frames),
                         static_cast<long long>(now - probe_seek_t0),
                         static_cast<long long>(media_us),
                         probe_had_frame ? "有旧帧" : "无帧(首次)");
            std::fflush(stderr);
        }
        probeSummaryTick();
    }
    if (has_frame_) convertToRgba(); // 目标帧补发布
    return has_frame_;
}

// ---------------------------------------------------------------------------
// seekTo —— 回退到 target_us 前最近关键帧并冲洗解码器
// ---------------------------------------------------------------------------
bool FfmpegMediaSource::seekTo(std::int64_t target_us) {
    if (!fmt_ || !codec_) return false;
    const AVRational tb{tb_num_, tb_den_};

    // 素材内部时间 → 流时间戳（补媒体起点偏移）
    const std::int64_t target_ts =
        start_pts_ + av_rescale_q(target_us, AVRational{1, AV_TIME_BASE}, tb);

    // 【fail-safe】记住旧帧有效性：seek 冲洗后若解不出新帧（单帧媒体
    // 尾部 seek / 目标超尾 EOF），恢复旧帧停留而非黑屏——rgba_ 缓冲
    // 未被破坏，恢复标志即可让 produce 继续返回最后一帧。
    const bool had_frame = has_frame_;

    // AVSEEK_FLAG_BACKWARD：落到 <= target 的最近关键帧——
    // 保证后续 B/P 帧的参考链完整（向前 seek 会解出花屏）
    if (av_seek_frame(fmt_, stream_index_, target_ts, AVSEEK_FLAG_BACKWARD) < 0)
        return false; // seek 本身失败：旧帧未动，produce 层续用
    // 冲洗解码器内部缓冲（丢弃 seek 点前的旧参考帧/缓存帧）
    avcodec_flush_buffers(codec_);
    flushed_   = false;     // 重开输入流：flush 标志复位
    at_eof_    = false;     // 新位置重新解码：EOF 状态作废
    has_frame_ = false;     // 旧帧作废（解出新帧前 frameView 不可用）
    // 【无 PTS 流基准】记录 seek 目标：无 PTS 流解出的帧没有可靠时间戳，
    // 首帧（convertToRgba 中 has_frame_==false 分支）以 target 建立
    // 递推起点（BACKWARD 关键帧 ≤ target，误差 ≤ 一个 GOP），后续帧
    // 顺延。若不记录而强设 0，播放中段 seek 后递推从 0 追 target——
    // 解码量失控且内容错帧。
    seek_base_us_  = target_us;
    seek_pending_  = true;
    if (!decodeNext()) {    // 解不出关键帧（EOF/解码失败）
        has_frame_ = had_frame; // fail-safe：恢复旧帧停留（不黑屏）
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// decodeNext —— 解码下一帧并转 RGBA；false = 流结束或解码失败
// ---------------------------------------------------------------------------
bool FfmpegMediaSource::decodeNext() {
    while (true) {
        // ① 先尝试取帧（多数调用直接命中：解码器已有产出）
        int ret = avcodec_receive_frame(codec_, frame_);
        if (ret == 0) {
            if (!convertToRgba()) return false; // 像素转换失败：按解码失败处理
            ++decoded_count_;
            return true;
        }
        if (ret == AVERROR_EOF) {           // 已 drain 完毕：流尾
            at_eof_ = true;                 // produce 据此扩展缓存命中区间
            return false;
        }
        if (ret != AVERROR(EAGAIN)) {       // 真实解码错误
            char err[128];
            av_strerror(ret, err, sizeof(err));
            std::fprintf(stderr, "[media] 解码错误(%d %s) fmt=%d %dx%d\n",
                         ret, err, frame_->format, width_, height_);
            return false;
        }

        // ② EAGAIN：解码器需要更多输入
        if (av_read_frame(fmt_, pkt_) < 0) {
            // 输入读完：发送空 packet 进入 drain 模式，收尾解码器内部余帧
            if (!flushed_) {
                flushed_ = true;
                avcodec_send_packet(codec_, nullptr);
                continue;
            }
            at_eof_ = true; // 已 flush 且无余帧：确认 EOF（流尾停留）
            return false;
        }
        // 非视频流的包（音频/字幕等）直接跳过
        if (pkt_->stream_index != stream_index_) {
            av_packet_unref(pkt_);
            continue;
        }
        ret = avcodec_send_packet(codec_, pkt_);
        av_packet_unref(pkt_); // 引用计数归零（packet 数据归 FFmpeg 内部所有）
        if (ret < 0 && ret != AVERROR(EAGAIN)) return false;
        // EAGAIN 属正常背压：回到 ① 先收帧再喂数
    }
}

// ---------------------------------------------------------------------------
// convertToRgba —— 解码原始帧 → RGBA 缓冲（sws_scale 按需重建转换器）
// \return false = 转换器创建/像素转换失败（当前帧不可用，调用方按失败处理）
// ---------------------------------------------------------------------------
bool FfmpegMediaSource::convertToRgba() {
    // ---- 【发布抑制分支（2026-11-05 暂停"快放"修复）】-------------------
    // produce 补齐循环期间（suppress_publish_ 置位）：只推进时间轴
    // （PTS 递推/has_frame_，与下方常规路径同式），跳过 GPU clone、
    // 硬解下载、sws_scale 与发布槽 store——渲染端停在旧帧，不会看到
    // 逐帧"快进"的中间帧；循环结束后调用方对目标帧再次调用本函数
    // （标志已复位）完成转换+发布。frame_ 的解码属性在所有路径下均
    // 有效（是解码器原始输出帧），属性读取无需像素转换参与。
    if (suppress_publish_) {
        std::int64_t ts = frame_->best_effort_timestamp;
        if (ts == AV_NOPTS_VALUE) ts = frame_->pts;
        const AVRational tb{tb_num_, tb_den_};
        if (ts != AV_NOPTS_VALUE && tb.den > 0) {
            frame_pts_us_ =
                av_rescale_q(ts, tb, AVRational{1, AV_TIME_BASE}) -
                av_rescale_q(start_pts_, tb, AVRational{1, AV_TIME_BASE});
            seek_pending_ = false;
        } else {
            frame_pts_us_ = has_frame_ ? frame_pts_us_ + frame_span_us_
                                       : (seek_pending_ ? seek_base_us_ : 0);
            seek_pending_ = false;
        }
        has_frame_ = true;
        return true;
    }

    // ---- 【③阶段 GPU 帧交付】协商开启 + CUDA 硬解生效 → 原生 NV12 发布 --
    // 条件逐项（全部满足才跳过下载，任一不满足走既有下载路径）：
    //   * hw_active_ && CUDA 后端 && 帧确为硬件帧容器；
    //   * gpu_delivery 请求开启 && 协商旗标存在且为 true（GL 后端 interop
    //     探测失败时会把它翻成 false → 本分支自动旁路，无人工干预回退）。
    // 发布方式：av_frame_clone（硬件帧引用 +1，像素零拷贝）进 shared_ptr
    // 快照——渲染端 draw 期间引用计数 ≥ 1，解码器不会复用该表面；
    // 快照释构时 av_frame_free 归还引用。此路径下 sws/rgba_ 全部旁路
    // （零 CPU 像素工作），NV12→RGB 转换在 GL 着色器完成。
    if (hw_active_ && hw_type_ == HwAccelType::Cuda &&
        frame_->format == hw_pix_fmt_ && hw_cfg_.gpu_delivery &&
        hw_cfg_.gpu_delivery_ok &&
        hw_cfg_.gpu_delivery_ok->load(std::memory_order_relaxed)) {
        const int w = frame_->width;
        const int h = frame_->height;
        if (w > 0 && h > 0) {
            // 帧时间戳（与下方 CPU 路径同一套语义：best_effort 优先）
            std::int64_t ts = frame_->best_effort_timestamp;
            if (ts == AV_NOPTS_VALUE) ts = frame_->pts;
            const AVRational tb{tb_num_, tb_den_};
            if (ts != AV_NOPTS_VALUE && tb.den > 0) {
                frame_pts_us_ =
                    av_rescale_q(ts, tb, AVRational{1, AV_TIME_BASE}) -
                    av_rescale_q(start_pts_, tb, AVRational{1, AV_TIME_BASE});
                seek_pending_ = false;
            } else {
                frame_pts_us_ = has_frame_ ? frame_pts_us_ + frame_span_us_
                                           : (seek_pending_ ? seek_base_us_ : 0);
                seek_pending_ = false;
            }

            auto pf = std::make_shared<PublishedFrame>();
            pf->domain  = FrameDomain::GpuCudaNv12;
            pf->width   = w;
            pf->height  = h;
            pf->stride  = static_cast<std::uint32_t>(frame_->linesize[0]);
            pf->pts_us  = frame_pts_us_;
            // AVFrame 引用持有者：clone（引用 +1）+ 自定义释放器
            pf->native = std::shared_ptr<const void>(
                av_frame_clone(frame_),
                [](const void* p) {
                    // const_cast 安全：释构只做引用释放，无内容写入
                    AVFrame* f = const_cast<AVFrame*>(
                        static_cast<const AVFrame*>(p));
                    if (f != nullptr) av_frame_free(&f);
                });
            if (!pf->native) {
                std::fprintf(stderr, "[media] GPU 帧引用克隆失败（本帧丢弃）\n");
                return false;
            }
            has_frame_ = true;
            // decoded_count_ 由 decodeNext 统一累加（本函数不重复计数）
            published_.store(std::move(pf), std::memory_order_release);
            return true;
        }
        // w/h 非法（异常流）：落到下方常规路径按失败处理
    }

    // ---- 硬解帧下载（第①阶段/③回退档）：GPU 帧容器 → 系统内存帧 ----
    // 硬解生效时 frame_->format == hw_pix_fmt_（D3D11/CUDA/... 帧容器，
    // data 指向显存），sws 无法直接消费——经 av_hwframe_transfer_data
    // 下载到 sw_frame_（NV12 等常规格式）。拷贝代价：走显卡 DMA 引擎
    // （1080p NV12 ≈ 3.1MB/帧，30fps 下 PCIe 占用 <1%，不占计算单元）；
    // 软解帧（普通内存格式）跳过本步，行为与既有实现逐位一致。
    AVFrame* src = frame_;
    if (hw_active_ && frame_->format == hw_pix_fmt_) {
        if (sw_frame_ == nullptr) sw_frame_ = av_frame_alloc(); // 复用缓冲
        av_frame_unref(sw_frame_); // 释放上一帧残留引用
        if (av_hwframe_transfer_data(sw_frame_, frame_, 0) < 0) {
            std::fprintf(stderr, "[media] 硬解帧下载失败（本帧丢弃）\n");
            return false; // 单帧下载失败按解码失败处理（下一帧重试）
        }
        // 【关键坑】transfer 只搬像素不搬属性——PTS/best_effort_timestamp
        // 在 sw_frame_ 上为 0，必须手动补齐，否则缓存命中区间/seek 递推
        // /发布时间戳全部失准（画面时间轴错乱）。
        sw_frame_->pts                  = frame_->pts;
        sw_frame_->best_effort_timestamp = frame_->best_effort_timestamp;
        src = sw_frame_;
    }

    const int w = src->width;
    const int h = src->height;
    if (w <= 0 || h <= 0) return false;

    // ---- 转换上下文：仅源格式/尺寸变化时重建（sws_getContext 较贵）----
    // 【发布槽 move 语义】像素缓冲随发布移交（rgba_ 每帧变空），故缓冲
    // 重建与转换器重建解耦：转换器只在真尺寸变化时重建，缓冲每帧新分配
    // （assign；发布时 move 走，旧缓冲由渲染线程的 shared_ptr 保活）。
    // 源格式动态探测：软解典型 yuv420p；硬解下载后典型 nv12——sws 对
    // 两者均有原生支持，转换逻辑无需分支。
    if (!sws_ || static_cast<std::size_t>(w) * 4 != stride_ ||
        width_ != w || height_ != h ||
        sws_src_fmt_ != static_cast<int>(src->format)) {
        sws_freeContext(sws_);
        sws_ = sws_getContext(w, h, static_cast<AVPixelFormat>(src->format),
                              w, h, AV_PIX_FMT_RGBA,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_) {
            std::fprintf(stderr, "[media] 像素转换器创建失败(fmt=%d %dx%d)\n",
                         src->format, w, h);
            return false; // 旧帧保留（has_frame_ 不动），本帧不可用
        }
        width_      = w;
        height_     = h;
        stride_     = static_cast<std::size_t>(w) * 4;
        sws_src_fmt_ = static_cast<int>(src->format); // 记录：格式变化触发重建
    }
    if (rgba_.size() != stride_ * static_cast<std::size_t>(h)) {
        rgba_.assign(stride_ * static_cast<std::size_t>(h), 0);
    }

    // 逐帧行拷贝转换（yuv420p/nv12/… → RGBA）
    std::uint8_t* dst[4]  = { rgba_.data(), nullptr, nullptr, nullptr };
    int           dls[4]  = { static_cast<int>(stride_), 0, 0, 0 };
    if (sws_scale(sws_, src->data, src->linesize, 0, h, dst, dls) < 0) {
        std::fprintf(stderr, "[media] 像素转换失败(fmt=%d %dx%d)\n",
                     src->format, w, h);
        return false;
    }

    // 帧时间戳：best_effort_timestamp（容器信息推得的可靠 PTS）优先
    std::int64_t ts = src->best_effort_timestamp;
    if (ts == AV_NOPTS_VALUE) ts = src->pts;
    const AVRational tb{tb_num_, tb_den_};
    if (ts != AV_NOPTS_VALUE && tb.den > 0) {
        // 流时间戳 → 素材内部微秒（扣除媒体起点偏移）
        frame_pts_us_ = av_rescale_q(ts, tb, AVRational{1, AV_TIME_BASE}) -
                        av_rescale_q(start_pts_, tb, AVRational{1, AV_TIME_BASE});
        seek_pending_ = false; // 有可靠 PTS：seek 基准派不上用场
    } else {
        // 退化流无 PTS：按帧时长线性递推（保持时间轴单调）。
        // 首帧（has_frame_ 尚为 false）的递推基准：
        //   * seek 后首帧 → seek_base_us_（seekTo 记录的目标时刻）——
        //     若强设 0，播放中段 seek 后前向递推从 0 追目标，解码量
        //     失控且画面错帧（修复点）；
        //   * prepare 后首帧 → 0（缓存命中区间为 [0, span)）。
        frame_pts_us_ = has_frame_ ? frame_pts_us_ + frame_span_us_
                                   : (seek_pending_ ? seek_base_us_ : 0);
        seek_pending_ = false; // 基准已消费
    }
    has_frame_ = true;

    // ---- 发布帧快照（解码→渲染跨线程交付；见 PublishedFrame 注释）----
    // 像素所有权 move 进新快照（零拷贝发布）；rgba_ 变空，下一帧
    // convertToRgba 按需重新分配（等效每帧一次大块分配，成本可接受）。
    // 渲染线程正在绘制的旧发布帧由 shared_ptr 引用计数保活，不悬垂。
    auto pf = std::make_shared<PublishedFrame>();
    pf->rgba   = std::move(rgba_);
    pf->width  = width_;
    pf->height = height_;
    pf->stride = static_cast<std::uint32_t>(stride_);
    pf->pts_us = frame_pts_us_;
    published_.store(std::move(pf), std::memory_order_release);
    return true;
}

// ---------------------------------------------------------------------------
// frameView —— 最近发布帧的通用视图（渲染线程任意时刻可调；与 produce 并发安全）
//
// 读原子发布槽（acquire 配对 produce 端 release store）：返回的 FrameView
// 借用快照像素，keep 持有快照引用——绘制期间解码线程发布新帧/源释放均
// 不悬垂。无发布帧（未解码出首帧/已 release）返回 false。
// ---------------------------------------------------------------------------
bool FfmpegMediaSource::frameView(FrameView& out) const {
    const std::shared_ptr<const PublishedFrame> pf =
        published_.load(std::memory_order_acquire);
    if (!pf) return false;
    out.width  = pf->width;
    out.height = pf->height;
    out.stride = pf->stride;
    out.domain = pf->domain;
    out.keep   = pf; // 保活：draw 期间快照引用计数 ≥ 1
    if (pf->domain == FrameDomain::CpuRgba) {
        if (pf->rgba.empty()) return false;
        out.rgba   = pf->rgba.data();
        out.native = nullptr;
    } else {
        // GPU 原生帧域：rgba 为空，native 持有 AVFrame 引用（GL interop 消费）
        out.rgba   = nullptr;
        out.native = pf->native;
    }
    return true;
}

// ---------------------------------------------------------------------------
// release —— 释放全部底层资源（可随后重新 prepare 复用实例）
// ---------------------------------------------------------------------------
void FfmpegMediaSource::release() {
    // av_*_free 家族安全置空入参指针；close_input 同时关文件句柄
    if (pkt_)   av_packet_free(&pkt_);
    if (frame_) av_frame_free(&frame_);
    if (sw_frame_) av_frame_free(&sw_frame_); // 硬解帧下载缓冲（复用块）
    if (codec_) avcodec_free_context(&codec_); // hw_device_ctx 引用随之 unref
    if (hw_device_) av_buffer_unref(&hw_device_); // 设备引用兜底释放
    hw_pix_fmt_ = 0;
    hw_active_  = false;
    hw_type_    = HwAccelType::Soft;
    if (fmt_)   avformat_close_input(&fmt_);
    if (sws_)   { sws_freeContext(sws_); sws_ = nullptr; }
    sws_src_fmt_ = -1;

    rgba_.clear();
    rgba_.shrink_to_fit();
    stride_      = 0;
    stream_index_ = -1;
    width_ = height_ = 0;
    tb_num_ = tb_den_ = 0;
    start_pts_ = 0;
    duration_us_    = 0;
    frame_span_us_  = 33'333;
    frame_pts_us_   = 0;
    has_frame_ = false;
    flushed_   = false;
    at_eof_    = false;     // EOF 状态随上下文一起作废
    seek_base_us_  = 0;
    seek_pending_  = false;
    // 发布槽一并清空：渲染线程随后的 frameView 返回 false（无帧可画）。
    // 正持旧快照的渲染线程不受影响（shared_ptr 保活到绘制结束）。
    published_.store(nullptr, std::memory_order_release);
    // decoded_count_ 不清零：跨 prepare 累计，供诊断统计
}

// ---------------------------------------------------------------------------
// probeDurationUs —— 容器级时长探测（静态，无解码上下文）
// 纯音频文件无视频流，prepare 的 av_find_best_stream(VIDEO) 必失败；
// 此处只 avformat_open_input + find_stream_info 读容器头（不选流、不建
// 解码器），时长取容器级 fmt->duration（AV_TIME_BASE 微秒，直通 us 域）。
// 任何失败静默返回 0（调用方以"探测失败不阻断导入"策略兜底）。
// ---------------------------------------------------------------------------
std::int64_t FfmpegMediaSource::probeDurationUs(const std::string& utf8_path) {
    AVFormatContext* fmt = nullptr;
    // 路径为 UTF-8 字节流（与 prepare 同约定）；ffmpeg 内部按 UTF-8 处理
    int ret = avformat_open_input(&fmt, utf8_path.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char err[128];
        av_strerror(ret, err, sizeof(err));
        std::fprintf(stderr, "[media] 时长探测打开容器失败(%s): %s\n",
                     err, utf8_path.c_str());
        return 0;
    }
    std::int64_t dur = 0;
    ret = avformat_find_stream_info(fmt, nullptr);
    if (ret >= 0 && fmt->duration != AV_NOPTS_VALUE && fmt->duration > 0)
        dur = fmt->duration;
    else
        std::fprintf(stderr,
                     "[media] 时长探测失败(ret=%d duration=%lld): %s\n",
                     ret, (long long)fmt->duration, utf8_path.c_str());
    avformat_close_input(&fmt);
    return dur;
}

} // namespace ms
