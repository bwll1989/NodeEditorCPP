// ============================================================================
//  MpegTsSender 实现 —— Feed 轮询 → NVENC/x264 → MPEG-TS → UDP 组播
// ============================================================================

#include "live/MpegTsSender.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

#include "live/LanProbe.hpp" // 出口接口探测（localaddr 钉定）

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

namespace ms {

// ---------------------------------------------------------------------------
// FFmpeg 句柄集合（转发线程独占——本 struct 生命周期 = 编码会话）
// ---------------------------------------------------------------------------
struct MpegTsSender::Av {
    const AVCodec*             codec   = nullptr;
    AVCodecContext*            enc     = nullptr;
    AVStream*                  vs      = nullptr;
    AVFormatContext*           mux     = nullptr;
    SwsContext*                sws     = nullptr;
    AVFrame*                   yuv     = nullptr; ///< 编码输入帧（YUV420P）
    AVFrame*                   in      = nullptr; ///< RGBA 直喂包装帧（nvenc 专用）
    AVPacket*                  pkt     = nullptr; ///< 编码输出包（会话内复用）
    std::int64_t               pts     = 0;       ///< 递增 PTS（编码时基）
    bool                       nvenc_rgba = false; ///< true = RGBA 直喂（跳过 sws）
};

// ---------------------------------------------------------------------------
// 组播 URL 确定性分配
// ---------------------------------------------------------------------------
std::string multicastUrlFor(const std::string& asset_id) {
    // FNV-1a 32（与 MediaScheduler::Fnv1a64 同族——此处 32 位足够选址）
    std::uint32_t h = 2166136261u;
    for (unsigned char c : asset_id) {
        h ^= c;
        h *= 16777619u;
    }
    const unsigned o3 = 1 + ((h >> 8) & 0xFE);  // 1..255 奇数（合法主机位）
    const unsigned o4 = 1 + ((h >> 16) & 0xFE);
    const unsigned port = 50000 + (h % 10000);  // 50000..59999
    char buf[64];
    std::snprintf(buf, sizeof(buf),
                  "udp://239.100.%u.%u:%u?ttl=8&pkt_size=1316",
                  o3, o4, port);
    return buf;
}

// ---------------------------------------------------------------------------
// 网络层初始化（进程内一次；FFmpeg 8 需要显式调用）
// ---------------------------------------------------------------------------
namespace {
bool ffmpegNetInit() {
    static const bool ok = [] {
        avformat_network_init();
        return true;
    }();
    return ok;
}
} // namespace

MpegTsSender::MpegTsSender(std::string url, std::weak_ptr<LiveHub::Feed> feed)
    : url_(std::move(url)), feed_(std::move(feed)) {}

MpegTsSender::~MpegTsSender() { stop(); }

bool MpegTsSender::start() {
    if (running_.load(std::memory_order_acquire)) return true; // 幂等
    if (!ffmpegNetInit()) return false;
    running_.store(true, std::memory_order_release);
    try {
        worker_ = std::thread([this] { run(); });
    } catch (...) {
        running_.store(false, std::memory_order_release);
        return false;
    }
    return true;
}

void MpegTsSender::stop() {
    if (!running_.load(std::memory_order_acquire)) return;
    running_.store(false, std::memory_order_release);
    if (worker_.joinable()) worker_.join();
    teardown();
}

// ---------------------------------------------------------------------------
// setupEncoder：编码器探测（NVENC → libx264）+ TS muxer 装配
// ---------------------------------------------------------------------------
bool MpegTsSender::setupEncoder(int w, int h) {
    // 尺寸对齐（H.264 编码器要求偶数；YUV420P 需 2 对齐）
    const int ew = (w / 2) * 2;
    const int eh = (h / 2) * 2;

    auto* a = new Av();
    av_.reset(a);
    av_ok_ = false; // 致命失败路径 teardown 时据此区分释放

    // ---- 码率：1080p60 ≈ 16Mbps 基准，按像素量/帧率缩放，clamp 4~40M ----
    const std::int64_t bitrate =
        std::clamp<std::int64_t>(
            static_cast<std::int64_t>(ew) * eh * 60 / (1920 * 1080) * 16'000'000 / 60,
            4'000'000, 40'000'000);

    // ---- 编码器探测：nvenc 打开成功才用；否则回退 libx264 ----
    // 【2026-11-23 性能修复】nvenc 成功路径直接喂 RGBA（enc->pix_fmt=
    // AV_PIX_FMT_RGBA，内部 GPU 转换）——跳过 CPU sws RGBA→YUV420P
    //（1080p60 曾恒占约半核）。约束：源尺寸须无裁剪（奇数宽高被偶数
    // 对齐裁边时回退 yuv 路径走 sws）。色彩域显式 BT.601：与 display
    // 收端 sws 默认（SWS_CS_DEFAULT=601）闭环一致，颜色不漂移。
    const char* candidates[] = {"h264_nvenc", "libx264"};
    for (const char* name : candidates) {
        a->codec = avcodec_find_encoder_by_name(name);
        if (a->codec == nullptr) continue;
        a->enc = avcodec_alloc_context3(a->codec);
        if (a->enc == nullptr) continue;
        a->enc->width     = ew;
        a->enc->height    = eh;
        a->enc->time_base = AVRational{1, 90000}; // TS 标准时基
        a->enc->framerate = AVRational{30, 1};    // 元数据（实际按 Feed 帧驱动）
        a->enc->bit_rate  = bitrate;
        a->enc->gop_size  = 60;                   // 1s@60p / 2s@30p：新加入
                                                  // display 秒级出画
        a->enc->max_b_frames = 0;                 // 低延迟：解码序=显示序
        if (std::strcmp(name, "h264_nvenc") == 0) {
            // NVENC 低延迟 CBR：p1 最快 preset；无 B 帧已由 max_b_frames 保证
            av_opt_set(a->enc->priv_data, "preset", "p1", 0);
            av_opt_set(a->enc->priv_data, "rc", "cbr", 0);
            av_opt_set_int(a->enc->priv_data, "zerolatency", 1, 0);
            if (w == ew && h == eh) { // 无裁剪才可直喂（奇数宽高走 sws 裁剪）
                a->enc->pix_fmt     = AV_PIX_FMT_RGBA;
                a->enc->colorspace  = AVCOL_SPC_SMPTE170M; // BT.601（枚举名
                // SMPTE170M——与收端 sws 默认 SWS_CS_DEFAULT=601 闭环一致）
                a->enc->color_range = AVCOL_RANGE_MPEG; // limited（sws 同默认）
                a->nvenc_rgba       = true;
            } else {
                a->enc->pix_fmt = AV_PIX_FMT_YUV420P;
            }
        } else {
            a->enc->pix_fmt = AV_PIX_FMT_YUV420P;
            av_opt_set(a->enc->priv_data, "preset", "ultrafast", 0);
            av_opt_set(a->enc->priv_data, "tune", "zerolatency", 0);
            // 限 2 线程（sliced——zerolatency 禁 frame threading 无帧延迟）：
            // 默认线程数=核数会把 editor UI/渲染的 CPU 吃穿（用户报"很卡"
            // ——编码霸核拖累全进程）。2 线程 1080p ultrafast 编码能力
            // ≈50fps，不足帧率时 Feed 取 latest 跳帧，画面仍连续。
            av_opt_set(a->enc->priv_data, "threads", "2", 0);
        }
        const int open_ret = avcodec_open2(a->enc, a->codec, nullptr);
        if (open_ret >= 0) {
            encoder_name_ = name;
            break;
        }
        // 探测失败（无 N 卡/驱动会话超限等）：带原因日志——用户报"CPU 高"
        // 时可一眼确认是否静默降级到了 x264 软编（霸核大头）
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(open_ret, errbuf, sizeof(errbuf));
        std::fprintf(stderr, "mcast: %s 打开失败（%s）→ 尝试下一候选\n",
                     name, errbuf);
        std::fflush(stderr);
        avcodec_free_context(&a->enc); // 探测失败（无 N 卡等）→ 下一候选
        a->enc  = nullptr;
        a->codec = nullptr;
    }
    if (a->enc == nullptr) {
        std::fprintf(stderr, "mcast: 无可用 H.264 编码器（nvenc/x264 均失败）\n");
        std::fflush(stderr);
        return false;
    }

    // ---- 编码输入帧 ----
    // RGBA 直喂（nvenc）：仅包装帧——data 指向 LiveFrame 像素（不持有），
    // send_frame 内部同步拷入 GPU pinned 缓冲后即可释放引用；LiveFrame
    // 帧内存恒不可变快照（新帧新对象永不复用），无悬垂窗口。
    // YUV 路径（x264 / 奇数裁剪回退）：自持 YUV420P 帧 + sws 转换。
    if (a->nvenc_rgba) {
        a->in = av_frame_alloc();
        if (a->in == nullptr) return false;
    } else {
        a->yuv = av_frame_alloc();
        a->yuv->format = AV_PIX_FMT_YUV420P;
        a->yuv->width  = ew;
        a->yuv->height = eh;
        if (av_frame_get_buffer(a->yuv, 0) < 0) return false;
        // ---- RGBA → YUV420P 转换器（sws 默认 BT.601——与收端闭环一致）----
        a->sws = sws_getContext(w, h, AV_PIX_FMT_RGBA,
                                ew, eh, AV_PIX_FMT_YUV420P,
                                SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (a->sws == nullptr) return false;
    }
    a->pkt = av_packet_alloc(); // 编码输出包（会话内复用——unref 复位语义）
    if (a->pkt == nullptr) return false;

    // ---- MPEG-TS muxer over UDP 组播 ----
    // 【2026-11-23 双网卡免疫】组播出口接口钉定：URL 保持场景存储的基础
    // 形式，发送前 append 本机 LAN IPv4（FFmpeg localaddr =
    // IP_MULTICAST_IF）。Windows 组播 best route 可能命中 127.0.0.1
    // （系统 224/4→loopback 默认路由 metric 最低）或 WSL 虚拟网卡 → 本机
    // 回环自收正常、局域网零包（探测规则见 LanProbe.hpp）。
    // 【2026-11-26 手动指定】URL 显式 localaddr > 进程显式配置
    // （setExplicitLanIPv4，display/编辑器启动时注入）> 自动探测——
    // 裁决与日志统一在 appendLanLocaladdr。
    const std::string open_url =
        appendLanLocaladdr(url_, "mcast:sender");
    if (avformat_alloc_output_context2(&a->mux, nullptr, "mpegts",
                                       open_url.c_str()) < 0)
        return false;
    a->vs = avformat_new_stream(a->mux, nullptr);
    if (a->vs == nullptr) return false;
    avcodec_parameters_from_context(a->vs->codecpar, a->enc);
    a->vs->time_base = a->enc->time_base;
    // TS 周期重传 SDT/PAT/PMT + 关键帧前头数据（新加入 display 快速出画）
    av_opt_set_int(a->mux->priv_data, "sdt_period",
                   static_cast<std::int64_t>(0.5 * AV_TIME_BASE), 0);
    av_opt_set_int(a->mux->priv_data, "pat_period",
                   static_cast<std::int64_t>(0.5 * AV_TIME_BASE), 0);
    if (!(a->mux->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&a->mux->pb, open_url.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::fprintf(stderr, "mcast: 组播地址打开失败: %s\n",
                         open_url.c_str());
            std::fflush(stderr);
            return false;
        }
    }
    AVDictionary* opt = nullptr;
    av_dict_set(&opt, "muxrate", "0", 0); // VBR TS（按需打包——CBR mux 会
                                          // 填充空包翻倍带宽）
    av_dict_set(&opt, "flush_packets", "1", 0); // 每包即发（低延迟）
    const int ret = avformat_write_header(a->mux, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        std::fprintf(stderr, "mcast: TS 头写入失败: %s\n", url_.c_str());
        std::fflush(stderr);
        return false;
    }
    av_ok_ = true;
    std::fprintf(stderr, "mcast: 组播启动 %s（%s %dx%d %.1fMbps%s）\n",
                 open_url.c_str(), encoder_name_.c_str(), ew, eh,
                 static_cast<double>(bitrate) / 1e6,
                 a->nvenc_rgba ? "，RGBA 直喂" : "");
    std::fflush(stderr);
    return true;
}

// ---------------------------------------------------------------------------
// encodeAndSend：RGBA 直喂（nvenc）/ RGBA→YUV420P→编码（x264），muxer 写出
// ---------------------------------------------------------------------------
bool MpegTsSender::encodeAndSend(const LiveFrame& frame) {
    Av* a = av_.get();

    // PTS = 应显时刻戳换算到编码时基（首帧锚定——同戳同延迟多端一致）。
    // pts_us==0（异常帧）回退本地时钟作戳——与排程端语义一致。
    if (first_pts_us_ == 0)
        first_pts_us_ = frame.pts_us != 0 ? frame.pts_us : av_gettime();
    const std::int64_t stamp =
        frame.pts_us != 0 ? frame.pts_us : av_gettime();
    a->pts = av_rescale_q(stamp - first_pts_us_,
                          AVRational{1, 1000000}, a->enc->time_base);

    if (a->nvenc_rgba) {
        // RGBA 直喂：包装帧指向源像素（send_frame 同步拷入 GPU——
        // LiveFrame 快照不可变，消费窗口内数据恒有效）
        a->in->format      = AV_PIX_FMT_RGBA;
        a->in->width       = frame.w;
        a->in->height      = frame.h;
        a->in->data[0]     = const_cast<std::uint8_t*>(frame.rgba.data());
        a->in->linesize[0] = frame.w * 4;
        a->in->pts         = a->pts;
        if (avcodec_send_frame(a->enc, a->in) < 0) return false;
    } else {
        // RGBA → YUV420P（源 stride = w*4）
        const uint8_t* src_slice[1] = {frame.rgba.data()};
        const int      src_stride[1] = {frame.w * 4};
        sws_scale(a->sws, src_slice, src_stride, 0, frame.h,
                  a->yuv->data, a->yuv->linesize);
        a->yuv->pts = a->pts;
        if (avcodec_send_frame(a->enc, a->yuv) < 0) return false;
    }

    bool ok = true;
    while (avcodec_receive_packet(a->enc, a->pkt) == 0) {
        av_packet_rescale_ts(a->pkt, a->enc->time_base, a->vs->time_base);
        a->pkt->stream_index = a->vs->index;
        if (av_interleaved_write_frame(a->mux, a->pkt) < 0) ok = false;
        av_packet_unref(a->pkt);
    }
    av_packet_unref(a->pkt); // 收尾复位（下次 receive 前无脏状态）
    return ok;
}

// ---------------------------------------------------------------------------
// run：Feed version 轮询 → 编码发送（与原 LiveHub NDI 转发线程同款节拍）
// ---------------------------------------------------------------------------
void MpegTsSender::run() {
    std::shared_ptr<LiveHub::Feed> feed;
    bool setup_tried = false;
    std::int64_t last_try = 0; // 装配失败限频（线程栈变量——本线程独占）

    // 转发速率取证（5s 限频，对齐收端 live-ingest 日志模式——editor 端
    // 发送速率 vs display 端收帧速率对比，定位带宽瓶颈在哪段）；跳 =
    // Feed 已产但只发最新被跳过的帧数（非零 = 编码/网络跟不上采集节奏）
    std::int64_t  stat_ms    = 0;
    std::uint64_t total_sent = 0, win_sent = 0;
    std::uint64_t total_skip = 0, win_skip = 0;
    std::int64_t  idle_ms    = 0; // 空闲心跳限频（0 = 发帧中）

    while (running_.load(std::memory_order_acquire)) {
        feed = feed_.lock();
        if (!feed) break; // Hub 条目已亡：线程自退

        const std::uint64_t v = feed->version();
        if (v == last_version_) { // 无新帧：5ms 一拍（2026-11-23 自 20ms
                                  // 收紧——帧节拍量化抖动降到 1 帧@200fps
                                  // 轮询上限；空转原子读代价可忽略）
            // 【2026-11-26 空闲心跳】收端 "I/O error" 实锤 = 等不到首包
            // （FFmpeg rw_timeout 耗尽，socket/join 均成功）——发送端在
            // 编辑器关闭/源停帧时全部静默，必须可见：5s 一条。
            const std::int64_t now = av_gettime();
            if (now - idle_ms >= 5'000'000) {
                idle_ms = now;
                std::fprintf(stderr, "mcast: %s %s\n", url_.c_str(),
                             (av_ != nullptr && av_ok_)
                                 ? "帧流中断（源停帧，组播停发）"
                                 : "等待首帧（源未出帧，组播未开始发送）");
                std::fflush(stderr);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        idle_ms = 0; // 恢复发帧：心跳复位
        const std::uint64_t new_frames = v - last_version_; // ≥1
        const LiveFramePtr frame = feed->latest();
        last_version_ = v;
        if (!frame || frame->w <= 0 || frame->h <= 0) continue;

        // 惰性装配（首帧按实际分辨率；失败后按 2s 限频重试——设备可能
        // 被其他进程占用后释放）
        if (av_ == nullptr || !av_ok_) {
            const std::int64_t now = av_gettime();
            if (setup_tried && now - last_try < 2'000'000) continue;
            last_try = now;
            setup_tried = true;
            teardown();
            if (!setupEncoder(frame->w, frame->h)) continue;
        }
        if (encodeAndSend(*frame)) {
            ++total_sent;
            ++win_sent;
            total_skip += new_frames - 1;
            win_skip   += new_frames - 1;
            // 5s 限频转发取证（跨机排障：与 display 端收帧速率对读）
            const std::int64_t now_ms = std::chrono::duration_cast<
                std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            if (stat_ms == 0) {
                stat_ms = now_ms;
            } else if (now_ms - stat_ms >= 5000) {
                const double secs = double(now_ms - stat_ms) / 1000.0;
                std::fprintf(stderr,
                             "mcast: 转发速率 %.1f fps（累计发 %llu 跳 %llu）"
                             "%dx%d\n",
                             double(win_sent) / secs,
                             static_cast<unsigned long long>(total_sent),
                             static_cast<unsigned long long>(total_skip),
                             frame->w, frame->h);
                std::fflush(stderr);
                stat_ms  = now_ms;
                win_sent = 0;
                win_skip = 0;
            }
        } else {
            // 写包失败（网络异常）：拆链重装配（下次帧到达重试）
            std::fprintf(stderr, "mcast: 发送失败，重装配 %s\n", url_.c_str());
            std::fflush(stderr);
            teardown();
        }
    }
    teardown();
}

// ---------------------------------------------------------------------------
// teardown：释放全部 FFmpeg 句柄（幂等）
// ---------------------------------------------------------------------------
void MpegTsSender::teardown() {
    if (av_ == nullptr) return;
    Av* a = av_.get();
    if (a->mux != nullptr) {
        if (av_ok_ && (a->mux->oformat->flags & AVFMT_NOFILE) == 0)
            av_write_trailer(a->mux); // TS 尾包（尽力——组播无连接语义）
        if (a->mux->pb != nullptr) avio_closep(&a->mux->pb);
        avformat_free_context(a->mux);
        a->mux = nullptr;
    }
    if (a->sws != nullptr) { sws_freeContext(a->sws); a->sws = nullptr; }
    if (a->in  != nullptr) av_frame_free(&a->in);
    if (a->yuv != nullptr) av_frame_free(&a->yuv);
    if (a->pkt != nullptr) av_packet_free(&a->pkt);
    if (a->enc != nullptr) avcodec_free_context(&a->enc);
    av_.reset(); // delete Av（unique_ptr 管理）
    av_ok_ = false;
    first_pts_us_ = 0;
}

} // namespace ms
