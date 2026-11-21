// ============================================================================
//  MpegTsSender 实现 —— Feed 轮询 → NVENC/x264 → MPEG-TS → UDP 组播
// ============================================================================

#include "live/MpegTsSender.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

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
    std::int64_t               pts     = 0;       ///< 递增 PTS（编码时基）
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
    av_ = a;
    av_ok_ = false; // 致命失败路径 teardown 时据此区分释放

    // ---- 码率：1080p60 ≈ 16Mbps 基准，按像素量/帧率缩放，clamp 4~40M ----
    const std::int64_t bitrate =
        std::clamp<std::int64_t>(
            static_cast<std::int64_t>(ew) * eh * 60 / (1920 * 1080) * 16'000'000 / 60,
            4'000'000, 40'000'000);

    // ---- 编码器探测：nvenc 打开成功才用；否则回退 libx264 ----
    const char* candidates[] = {"h264_nvenc", "libx264"};
    for (const char* name : candidates) {
        a->codec = avcodec_find_encoder_by_name(name);
        if (a->codec == nullptr) continue;
        a->enc = avcodec_alloc_context3(a->codec);
        if (a->enc == nullptr) continue;
        a->enc->width     = ew;
        a->enc->height    = eh;
        a->enc->pix_fmt   = AV_PIX_FMT_YUV420P;
        a->enc->time_base = AVRational{1, 90000}; // TS 标准时基
        a->enc->framerate = AVRational{30, 1};    // 元数据（实际按 Feed 帧驱动）
        a->enc->bit_rate  = bitrate;
        a->enc->gop_size  = 60;                   // 1s@60p / 2s@30p：新加入
                                                  // display 秒级出画
        a->enc->max_b_frames = 0;                 // 低延迟：解码序=显示序
        if (std::strcmp(name, "libx264") == 0) {
            av_opt_set(a->enc->priv_data, "preset", "ultrafast", 0);
            av_opt_set(a->enc->priv_data, "tune", "zerolatency", 0);
        } else {
            // NVENC 低延迟 CBR：p1 最快 preset；无 B 帧已由 max_b_frames 保证
            av_opt_set(a->enc->priv_data, "preset", "p1", 0);
            av_opt_set(a->enc->priv_data, "rc", "cbr", 0);
            av_opt_set_int(a->enc->priv_data, "zerolatency", 1, 0);
        }
        if (avcodec_open2(a->enc, a->codec, nullptr) >= 0) {
            encoder_name_ = name;
            break;
        }
        avcodec_free_context(&a->enc); // 探测失败（无 N 卡等）→ 下一候选
        a->enc  = nullptr;
        a->codec = nullptr;
    }
    if (a->enc == nullptr) {
        std::fprintf(stderr, "mcast: 无可用 H.264 编码器（nvenc/x264 均失败）\n");
        std::fflush(stderr);
        return false;
    }

    // ---- 编码输入帧（YUV420P）----
    a->yuv = av_frame_alloc();
    a->yuv->format = AV_PIX_FMT_YUV420P;
    a->yuv->width  = ew;
    a->yuv->height = eh;
    if (av_frame_get_buffer(a->yuv, 0) < 0) return false;

    // ---- RGBA → YUV420P 转换器（BT.709 色彩域——视频标准）----
    a->sws = sws_getContext(w, h, AV_PIX_FMT_RGBA,
                            ew, eh, AV_PIX_FMT_YUV420P,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (a->sws == nullptr) return false;

    // ---- MPEG-TS muxer over UDP 组播 ----
    if (avformat_alloc_output_context2(&a->mux, nullptr, "mpegts", url_.c_str()) < 0)
        return false;
    a->vs = avformat_new_stream(a->mux, nullptr);
    if (a->vs == nullptr) return false;
    avcodec_parameters_from_context(a->vs->codecpar, a->enc);
    a->vs->time_base = a->enc->time_base;
    // TS 周期重传 SDT/PAT/PMT + 关键帧前头数据（新加入 display 快速出画）
    av_opt_set_int(a->mux->priv_data, "sdt_period", 0.5 * AV_TIME_BASE, 0);
    av_opt_set_int(a->mux->priv_data, "pat_period",  0.5 * AV_TIME_BASE, 0);
    if (!(a->mux->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&a->mux->pb, url_.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::fprintf(stderr, "mcast: 组播地址打开失败: %s\n", url_.c_str());
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
    std::fprintf(stderr, "mcast: 组播启动 %s（%s %dx%d %.1fMbps）\n",
                 url_.c_str(), encoder_name_.c_str(), ew, eh,
                 static_cast<double>(bitrate) / 1e6);
    std::fflush(stderr);
    return true;
}

// ---------------------------------------------------------------------------
// encodeAndSend：RGBA → YUV420P → 编码 → muxer
// ---------------------------------------------------------------------------
bool MpegTsSender::encodeAndSend(const LiveFrame& frame) {
    auto* a = static_cast<Av*>(av_);

    // PTS = 帧接收戳换算到编码时基（首帧锚定——同戳同延迟多端一致）
    if (first_pts_us_ == 0) first_pts_us_ = frame.pts_us != 0 ? frame.pts_us : av_gettime();
    const std::int64_t rel_us =
        (frame.pts_us != 0 ? frame.pts_us : av_gettime()) - first_pts_us_;
    a->pts = av_rescale_q(rel_us, AVRational{1, 1000000}, a->enc->time_base);

    // RGBA → YUV420P（源 stride = w*4）
    const uint8_t* src_slice[1] = {frame.rgba.data()};
    const int      src_stride[1] = {frame.w * 4};
    sws_scale(a->sws, src_slice, src_stride, 0, frame.h,
              a->yuv->data, a->yuv->linesize);
    a->yuv->pts = a->pts;

    if (avcodec_send_frame(a->enc, a->yuv) < 0) return false;
    AVPacket* pkt = av_packet_alloc();
    bool ok = true;
    while (avcodec_receive_packet(a->enc, pkt) == 0) {
        av_packet_rescale_ts(pkt, a->enc->time_base, a->vs->time_base);
        pkt->stream_index = a->vs->index;
        if (av_interleaved_write_frame(a->mux, pkt) < 0) ok = false;
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
    return ok;
}

// ---------------------------------------------------------------------------
// run：Feed version 轮询 → 编码发送（与 LiveHub NDI senderLoop 同款节拍）
// ---------------------------------------------------------------------------
void MpegTsSender::run() {
    std::shared_ptr<LiveHub::Feed> feed;
    last_version_ = 0;
    bool setup_tried = false;

    while (running_.load(std::memory_order_acquire)) {
        feed = feed_.lock();
        if (!feed) break; // Hub 条目已亡：线程自退

        const std::uint64_t v = feed->version();
        if (v == last_version_) { // 无新帧：20ms 一拍
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }
        const LiveFramePtr frame = feed->latest();
        last_version_ = v;
        if (!frame || frame->w <= 0 || frame->h <= 0) continue;

        // 惰性装配（首帧按实际分辨率；失败后按 2s 限频重试——设备可能
        // 被其他进程占用后释放）
        if (av_ == nullptr || !av_ok_) {
            const std::int64_t now = av_gettime();
            static thread_local std::int64_t last_try = 0;
            if (setup_tried && now - last_try < 2'000'000) continue;
            last_try = now;
            setup_tried = true;
            teardown();
            if (!setupEncoder(frame->w, frame->h)) continue;
        }
        if (!encodeAndSend(*frame)) {
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
    auto* a = static_cast<Av*>(av_);
    if (a->mux != nullptr) {
        if (av_ok_ && (a->mux->oformat->flags & AVFMT_NOFILE) == 0)
            av_write_trailer(a->mux); // TS 尾包（尽力——组播无连接语义）
        if (a->mux->pb != nullptr) avio_closep(&a->mux->pb);
        avformat_free_context(a->mux);
        a->mux = nullptr;
    }
    if (a->sws != nullptr) { sws_freeContext(a->sws); a->sws = nullptr; }
    if (a->yuv != nullptr) av_frame_free(&a->yuv);
    if (a->enc != nullptr) avcodec_free_context(&a->enc);
    delete a;
    av_ = nullptr;
    av_ok_ = false;
    first_pts_us_ = 0;
}

} // namespace ms
