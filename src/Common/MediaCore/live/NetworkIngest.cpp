// ============================================================================
//  NetworkIngest 实现 —— avformat 收流 → 解码 → RGBA → 打戳上抛
// ============================================================================

#include "live/NetworkIngest.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

#include "clock/ClockProvider.hpp"

#include "live/LanProbe.hpp" // join 接口探测（localaddr 钉定，与发送端同构）

namespace ms {

bool NetworkIngest::onStart() {
    avformat_network_init(); // 进程内幂等（FFmpeg 网络层全局一次）
    // 【2026-11-26 取证放行】udp 组播 open 失败（如 EIO）只有错误码无
    // syscall 明细，无法分诊——FFmpeg 内部 getaddrinfo/setsockopt/join
    // 的 av_log(ERROR) 是唯一真相源（默认 ERROR 级已可见，DEBUG 级连
    // socket 细节一起放行）。进程级一次设置，幂等。
    av_log_set_level(AV_LOG_DEBUG);
    return true;
}

// ---------------------------------------------------------------------------
// recvLoop：打开 → 读包解码 → RGBA 打戳上抛；异常/断流 → 重开（自愈）
// ---------------------------------------------------------------------------
void NetworkIngest::recvLoop() {
    AVFormatContext* fmt = nullptr;
    const AVCodec*   dec_desc = nullptr;
    AVCodecContext*  dec = nullptr;
    SwsContext*      sws = nullptr;
    int              vs_index = -1;
    std::int64_t     last_log_ms = 0;
    int sws_w = 0, sws_h = 0;          // sws 转换器当前目标尺寸（变化重建判据）
    enum AVPixelFormat sws_fmt = AV_PIX_FMT_NONE; // 源像素格式（同上）

    auto closeAll = [&]() {
        if (dec != nullptr) avcodec_free_context(&dec);
        if (fmt != nullptr) avformat_close_input(&fmt);
        if (sws != nullptr) { sws_freeContext(sws); sws = nullptr; }
        dec_desc = nullptr;
        vs_index = -1;
        sws_w = 0; sws_h = 0; sws_fmt = AV_PIX_FMT_NONE;
        connected_.store(false, std::memory_order_release);
    };

    while (running_.load(std::memory_order_acquire)) {
        // ---- 打开（低延迟选项组）----
        // 【2026-11-23 双网卡免疫】join 接口钉定：多网卡机器 FFmpeg 默认
        // join 可能落在 loopback/虚拟网卡 = 永远收不到物理网组播——与发
        // 送端同构，动态 append 本机 LAN IPv4（FFmpeg localaddr =
        // IP_ADD_MEMBERSHIP 接口；探测规则见 LanProbe.hpp；空 = 不追加）。
        std::string open_url = source_ref_;
        {
            const std::string lan = probeLanIPv4();
            if (!lan.empty())
                open_url += (open_url.find('?') == std::string::npos
                                 ? "?localaddr="
                                 : "&localaddr=") +
                            lan;
        }
        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "fflags", "nobuffer", 0);
        av_dict_set(&opts, "probesize", "32768", 0);
        av_dict_set(&opts, "analyzeduration", "0", 0);
        av_dict_set(&opts, "buffer_size", "4194304", 0);   // 4M UDP 收缓冲
                                                           // （组播突发削峰）
        av_dict_set(&opts, "timeout", "3000000", 0);        // 3s 断流检测
        av_dict_set(&opts, "reuse", "1", 0);                // 组播端口复用
        // 【2026-11-26 分诊隔离】Live 组播源恒 MPEG-TS（MpegTsSender 出
        // 流）——显式指定格式跳过探测层：EIO 若出在探测层（读首包）会转
        // 移到 find_stream_info 的"流信息探测失败"分支，与 socket 层失败
        // （getaddrinfo/bind/join）彻底区分 + 省首包等待。
        const AVInputFormat* ifmt = av_find_input_format("mpegts");
        const int ret = avformat_open_input(&fmt, open_url.c_str(),
                                            ifmt, &opts);
        av_dict_free(&opts);
        if (ret < 0) {
            // 【2026-11-26 错误码取证】"打开失败"笼统日志无法分诊——udp
            // 组播 open 内含 bind/join + 格式探测读首包：ETIMEDOUT=探测
            // 收不到包（组播被滤/发送端未推流/AP 不转发组播）；
            // EADDRNOTAVAIL=localaddr join 失败（IP 不在本机接口）。
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            // 分诊提示（2026-11-26 实锤：udp 读持续 EAGAIN 超过 rw_timeout
            // 时 FFmpeg retry_transfer_wrapper 返回 AVERROR(EIO)——"I/O
            // error" 与 ETIMEDOUT 同义 = 收不到包，socket/join 均成功（设
            // 备日志零 FFmpeg 错误行可证）。发送端三静默态：编辑器未运行/
            // 源无帧未发包/组播被滤）
            const char* hint = "";
            if (ret == AVERROR(ETIMEDOUT) || ret == AVERROR(EIO))
                hint = "（收不到首包：编辑器未运行/源未出帧未发包/组播被滤/AP 不转发）";
            else if (ret == AVERROR(EADDRNOTAVAIL))
                hint = "（localaddr 非本机接口，join 失败）";
            logThrottled("打开失败（2s 重试）[" + std::string(errbuf) +
                             "]" + hint + " " + open_url,
                         last_log_ms, 2000);
            closeAll();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        // ---- 选流 + 建解码器 ----
        if (avformat_find_stream_info(fmt, nullptr) < 0) {
            logThrottled("流信息探测失败（重开）", last_log_ms, 2000);
            closeAll();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        vs_index = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1,
                                       &dec_desc, 0);
        if (vs_index < 0 || dec_desc == nullptr) {
            logThrottled("无视频流（重开）", last_log_ms, 2000);
            closeAll();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        dec = avcodec_alloc_context3(dec_desc);
        if (dec == nullptr || avcodec_parameters_to_context(
                                  dec, fmt->streams[vs_index]->codecpar) < 0) {
            closeAll();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        dec->flags |= AV_CODEC_FLAG_LOW_DELAY; // B帧0 流：低延迟直出
        // 【2026-11-23 2 线程】原单线程（1.5 核@1080p60）是"零帧级延迟"
        // 刻意决策——但 display 端帧同步排程 target_latency 默认 120ms
        // （≥7 帧），frame threading 引入的 1 帧流水缓冲（thread_count-1）
        // 完全被排程窗吸收。2 线程软解省约半核（1080p60 单线程 h264 是
        // display 端 CPU 大头），延迟语义不变。
        dec->thread_count = 2;
        if (avcodec_open2(dec, dec_desc, nullptr) < 0) {
            logThrottled("解码器打开失败（重开）", last_log_ms, 2000);
            closeAll();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        std::fprintf(stderr, "net: 已连接 %s（%s %dx%d）\n",
                     open_url.c_str(), dec_desc->name,
                     dec->width, dec->height);
        std::fflush(stderr);

        // ---- 读包解码循环 ----
        AVPacket* pkt = av_packet_alloc();
        AVFrame*  fr = av_frame_alloc();
        bool stream_ok = true;
        while (stream_ok && running_.load(std::memory_order_acquire)) {
            const int r = av_read_frame(fmt, pkt);
            if (r < 0) { // 断流/超时 → 重开（自愈）
                logThrottled("流中断（重开）", last_log_ms, 2000);
                break;
            }
            if (pkt->stream_index != vs_index) { av_packet_unref(pkt); continue; }
            if (avcodec_send_packet(dec, pkt) < 0) { av_packet_unref(pkt); continue; }
            av_packet_unref(pkt);

            while (true) {
                const int rr = avcodec_receive_frame(dec, fr);
                if (rr == AVERROR(EAGAIN) || rr == AVERROR_EOF) break;
                if (rr < 0) { stream_ok = false; break; } // 解码异常 → 重开

                // 帧尺寸/格式变化 → sws 重建（组播源切分辨率/格式自愈）
                if (sws == nullptr || fr->width != sws_w ||
                    fr->height != sws_h ||
                    fr->format != sws_fmt) {
                    if (sws != nullptr) sws_freeContext(sws);
                    sws = sws_getContext(fr->width, fr->height,
                                         static_cast<AVPixelFormat>(fr->format),
                                         fr->width, fr->height,
                                         AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);
                    sws_w = fr->width;
                    sws_h = fr->height;
                    sws_fmt = static_cast<AVPixelFormat>(fr->format);
                    if (sws == nullptr) { stream_ok = false; break; }
                }

                auto out = newFrame(); // 池化（rgba 缓冲复用，消逐帧 8MB 分配）
                out->w = fr->width;
                out->h = fr->height;
                out->rgba.resize(static_cast<std::size_t>(fr->width) *
                                 fr->height * 4);
                const uint8_t* src[4] = {fr->data[0], fr->data[1],
                                         fr->data[2], fr->data[3]};
                const int stride[4] = {fr->linesize[0], fr->linesize[1],
                                       fr->linesize[2], fr->linesize[3]};
                uint8_t* dst[1] = {out->rgba.data()};
                const int dst_stride[1] = {fr->width * 4};
                sws_scale(sws, src, stride, 0, fr->height, dst,
                          const_cast<int*>(dst_stride));

                // 应显时刻 = 解码出画时刻 + 延迟预算（组播链路 display 端
                // 打戳落点：组播同流同达 → 多 display 解码时刻差亚毫秒，
                // "解码+预算"即跨机同步戳——pts 语义 = 应显时刻 2026-11-25；
                // 进程对齐时钟域，display 端含 SNTP offset）
                out->pts_us = processAlignedNowUs() + schedule_delay_us_;

                // 【2026-11-25 帧率取证】收端实际解码交付帧率（5s 一条，
                // 与 NdiIngest 同款）——组播链路瓶颈段定位：editor 端采集/
                // 组播正常而本行低 = 网络段（带宽/组播 join）瓶颈。
                {
                    using namespace std::chrono;
                    const std::int64_t now_ms = duration_cast<milliseconds>(
                        steady_clock::now().time_since_epoch()).count();
                    static thread_local std::int64_t  s_last_ms = 0;
                    static thread_local std::uint64_t s_last_n  = 0;
                    if (s_last_ms == 0) {
                        s_last_ms = now_ms;
                        s_last_n =
                            frames_received_.load(std::memory_order_relaxed);
                    } else if (now_ms - s_last_ms >= 5000) {
                        const std::uint64_t n =
                            frames_received_.load(std::memory_order_relaxed);
                        std::fprintf(stderr,
                                     "live-ingest: 收帧速率 %.1f fps"
                                     "（累计收 %llu 丢 %llu） %dx%d\n",
                                     double(n - s_last_n) /
                                         double(now_ms - s_last_ms) * 1000.0,
                                     static_cast<unsigned long long>(n),
                                     static_cast<unsigned long long>(
                                         frames_dropped_.load(
                                             std::memory_order_relaxed)),
                                     out->w, out->h);
                        std::fflush(stderr);
                        s_last_ms = now_ms;
                        s_last_n  = n;
                    }
                }
                connected_.store(true, std::memory_order_release);
                emitFrame(std::move(out));
            }
        }
        av_packet_free(&pkt);
        av_frame_free(&fr);
        closeAll();
        if (running_.load(std::memory_order_acquire))
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

} // namespace ms
