// ============================================================================
//  MpegTsSender —— LiveHub 组播发送器（Feed → H.264 → MPEG-TS → UDP 组播）
//
//  解决的问题（2026-11-21 用户定案）：一个 editor + N 台 display 的部署下，
//  NDI 单播（multi-TCP）按接收端数放大带宽（N 台 = N 份码流），组播把
//  editor 出口带宽收敛为恒 1 份（交换机侧 IGMP 复制）。
//
//  技术选型：
//    * 容器 MPEG-TS over UDP 组播——TS 自描述（PAT/PMT/SPS/PPS 周期重传），
//      免 SDP 协商分发，display 端 avformat 用 URL 直接打开（IPTV 同款）；
//    * 编码 h264_nvenc（探测：encoder 存在且打开成功才用；无 N 卡回退
//      libx264 ultrafast + zerolatency）；B 帧 0 → 解码序 = 显示序，端到端
//      编码延迟 <20ms，与 display 帧同步排程（pts + target_latency）兼容；
//    * 码率：按分辨率/帧率缩放（1080p60 ≈ 16Mbps 基准，clamp 4~40M）。
//
//  线程契约：单转发线程（构造后 start 启动；持 weak_ptr<Feed> 轮询
//  version——与原 LiveHub NDI 转发线程同款模式）；stop join 后线程绝迹。
//  FFmpeg 网络层需进程内 avformat_network_init 一次（本类惰性调用）。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "live/LiveFrame.hpp"
#include "live/LiveHub.hpp" // Feed（轮询源）

namespace ms {

class MpegTsSender {
public:
    /// \param url  组播输出 URL（udp://239.x.x.x:port?ttl=8&pkt_size=1316）
    /// \param feed 轮询帧源（weak 持有——Hub 条目先亡则线程自退）
    /// \param bind_ip 发送网卡 IPv4（2026-11-26：live 属性下拉指定——
    ///                多真实网卡机器自动探测可能选错；空 = 自动探测）
    MpegTsSender(std::string url, std::weak_ptr<LiveHub::Feed> feed,
                 std::string bind_ip = {});
    ~MpegTsSender(); // stop 兜底（join 后成员析构）

    MpegTsSender(const MpegTsSender&)            = delete;
    MpegTsSender& operator=(const MpegTsSender&) = delete;

    /// 启动转发线程（惰性初始化编码器/muxer 在线程首帧——设备探测失败
    /// 降级 libx264 也在线程内进行）。false = 线程创建失败。
    bool start();
    /// 停线程 + 释放编码器/muxer（幂等；join 后回调绝迹）
    void stop();

    /// 实际使用的编码器名（诊断："h264_nvenc"/"libx264"；启动前为空）
    const std::string& encoderName() const noexcept { return encoder_name_; }

private:
    /// 转发线程主体（Feed version 轮询 → 编码 → TS 打包发送）
    void run();
    /// 编码器 + TS muxer 装配（首帧到达时按实际分辨率惰性建；false = 致命）
    bool setupEncoder(int w, int h);
    /// 单帧编码 + 交织写入 muxer（RGBA 帧入口）
    bool encodeAndSend(const LiveFrame& frame);
    /// 释放编码器/转换器/muxer（幂等）
    void teardown();

    std::string                    url_;
    std::string                    bind_ip_; ///< 发送网卡（空=自动探测）
    std::weak_ptr<LiveHub::Feed>   feed_;
    std::thread                    worker_;
    std::atomic<bool>              running_{false};
    std::string                    encoder_name_; ///< 实际编码器（诊断）

    // ---- FFmpeg 装配（转发线程独占，无锁）----
    struct Av;                     ///< FFmpeg 句柄集合（cpp 内定义，头零泄漏）
    std::unique_ptr<Av> av_;       ///< 空或装配中（setupEncoder 建 / teardown 清）
    bool               av_ok_  = false; ///< setupEncoder 成功旗标
    std::uint64_t      last_version_ = 0;
    std::int64_t       first_pts_us_ = 0; ///< 首帧接收戳（PTS 锚）
};

/// 组播 URL 确定性分配（editor/display 同算无需存储）：FNV-1a(asset_id)
/// → 239.100.x.x : 50000+N。同 id 恒同址；选址空间 ≈163k 组合
///（奇数主机位 127×127 × 端口 10000），素材数 <100 时碰撞概率 ~3%，
/// 碰撞时用户改 URL 即可（display 端照收）。
std::string multicastUrlFor(const std::string& asset_id);

} // namespace ms
