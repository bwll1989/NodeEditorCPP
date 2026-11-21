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
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

#include "clock/ClockProvider.hpp"

namespace ms {

bool NetworkIngest::onStart() {
    avformat_network_init(); // 进程内幂等（与 MpegTsSender 共享一次）
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

    auto closeAll = [&]() {
        if (dec != nullptr) avcodec_free_context(&dec);
        if (fmt != nullptr) avformat_close_input(&fmt);
        if (sws != nullptr) { sws_freeContext(sws); sws = nullptr; }
        dec_desc = nullptr;
        vs_index = -1;
        connected_.store(false, std::memory_order_release);
    };

    std::int64_t last_log_ms = 0;
    int sws_w = 0, sws_h = 0; // sws 转换器当前目标尺寸（变化重建判据）
    while (running_.load(std::memory_order_acquire)) {
        // ---- 打开（低延迟选项组）----
        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "fflags", "nobuffer", 0);
        av_dict_set(&opts, "probesize", "32768", 0);
        av_dict_set(&opts, "analyzeduration", "0", 0);
        av_dict_set(&opts, "buffer_size", "4194304", 0);   // 4M UDP 收缓冲
                                                           // （组播突发削峰）
        av_dict_set(&opts, "timeout", "3000000", 0);        // 3s 断流检测
        av_dict_set(&opts, "reuse", "1", 0);                // 组播端口复用
        const int ret = avformat_open_input(&fmt, source_ref_.c_str(),
                                            nullptr, &opts);
        av_dict_free(&opts);
        if (ret < 0) {
            logThrottled("打开失败（2s 重试）: " + source_ref_,
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
        dec->thread_count = 1;                 // 软解低延迟（并行线程引入
                                               // 帧级流水延迟）
        if (avcodec_open2(dec, dec_desc, nullptr) < 0) {
            logThrottled("解码器打开失败（重开）", last_log_ms, 2000);
            closeAll();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        std::fprintf(stderr, "net: 已连接 %s（%s %dx%d）\n",
                     source_ref_.c_str(), dec_desc->name,
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

                // 帧尺寸/格式变化 → sws 重建（组播源切分辨率自愈）
                if (sws == nullptr || fr->width != sws_w ||
                    fr->height != sws_h) {
                    if (sws != nullptr) sws_freeContext(sws);
                    sws = sws_getContext(fr->width, fr->height,
                                         static_cast<AVPixelFormat>(fr->format),
                                         fr->width, fr->height,
                                         AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);
                    sws_w = fr->width;
                    sws_h = fr->height;
                    if (sws == nullptr) { stream_ok = false; break; }
                }

                auto out = std::make_shared<LiveFrame>();
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

                // 帧同步戳：解码出画时刻（进程对齐时钟域——display 端
                // 含 SNTP offset；多 display 同戳同延迟 → 跨机同步）
                out->pts_us = processAlignedNowUs();
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
