// ============================================================================
//  ControlClient.cpp —— TCP 控制通道·客户端实现
// ============================================================================

#include "net/ControlClient.hpp"

#include <cstdio>

#include <Poco/Exception.h>
#include <Poco/Net/SocketAddress.h>

#include "net/FrameCodec.hpp"

namespace ms {

bool ControlClient::connect(const std::string& host, std::uint16_t port) {
    // 重连前【无条件】清理旧连接：对端断开时 recv 线程自行退出并置
    // running_=false，但线程对象尚未 join——若仅按 running_ 判断会漏掉
    // join，随后对 joinable 线程移动赋值直接 std::terminate（P0 实证）。
    // stop() 幂等：未连接时 socket close 与 join 均安全、无回调副作用。
    stop();

    try {
        // connect 带超时握手（编辑器在本机/局域网，握手毫秒级）
        socket_.connect(Poco::Net::SocketAddress(host, port), Poco::Timespan(3'000'000));
        // 【关键】带超时的 connect 在 POCO 内部临时切换非阻塞模式，部分
        // 版本/路径不会恢复——非阻塞 socket 的 sendBytes 会部分写入且不报
        // 错，直接破坏 [长度前缀+Envelope] 帧完整性。显式恢复阻塞模式。
        socket_.setBlocking(true);
        socket_.setNoDelay(true); // 控制信令低延迟
        // 【发送超时 2026-11-04】编辑触发的场景下发在 Qt GUI 主线程同步
        // 广播（publishSceneAuto 防抖超时/onPublishScene 按钮均在主线程调
        // broadcast）——阻塞 socket 无发送超时时，对端"半死"（TCP 仍连
        // 但对端停止 recv：显示端卡死/挂起/网线半断）会让 sendBytes 在
        // 两端缓冲区填满后【无限期阻塞】→ GUI 冻结。setSendTimeout 把
        // 无限阻塞变为限时失败：超时抛 Poco::TimeoutException → send()
        // 既有 catch(Poco::Exception&) 捕获返回 false → 广播继续下一台、
        // 断连由 recv 线程回调/watchdog 既有链路接管。500ms 对局域网
        // 控制信令足够宽裕（正常路径 send 进内核缓冲即返回，微秒级）。
        socket_.setSendTimeout(Poco::Timespan(500 * 1000));
    } catch (const Poco::Exception& e) {
        // 【排障走 stderr】connect 失败原因必须留痕（联测出现过"看门狗
        // 疑似从未发起连接"的悬案——失败原因/次数是定位关键）
        std::fprintf(stderr, "[ctrl-client] connect %s:%u 失败: %s\n",
                     host.c_str(), (unsigned)port, e.displayText().c_str());
        // 连接失败可能留下半初始化 socket：显式关闭复位，供看门狗下轮
        // 重试在干净对象上进行（Poco connect 失败不保证 socket 可复用）
        try { socket_.close(); } catch (...) {}
        return false; // 连不上（Editor 未启动/地址错）由上层决定重试策略
    }

    running_.store(true);
    recv_buf_.clear();
    recv_thread_ = std::thread(&ControlClient::recv_loop, this);
    if (on_connected_) on_connected_();
    return true;
}

void ControlClient::stop() {
    // 【注意】不能因 running_ 已为 false 而提前返回：
    // 对端断开时 recv 线程自行退出并置 running_=false，但线程对象尚未
    // join——此时必须照常 join，否则析构 joinable 线程 → std::terminate。
    running_.store(false);
    {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        try { socket_.shutdownSend(); } catch (...) {}
        try { socket_.close();        } catch (...) {}
    }
    if (recv_thread_.joinable()) recv_thread_.join();
}

bool ControlClient::send(const ms_proto::Envelope& env) {
    const std::string frame = FrameCodec::encode(env);
    std::lock_guard<std::mutex> lock(socket_mutex_);
    if (!running_.load()) return false;
    bool ok = false;
    try {
        // 全帧写入校验：阻塞 socket 下 POCO sendBytes 必然发满；若出现
        // 部分写入说明 socket 处于非阻塞模式——帧已损坏，必须报失败
        const int sent = socket_.sendBytes(frame.data(), static_cast<int>(frame.size()));
        ok = (sent == static_cast<int>(frame.size()));
    } catch (const Poco::Exception&) {
        ok = false;
    }
    if (!ok) {
        // 【掉线隔离 2026-11-04】发送失败（异常 / 500ms 发送超时 / 部分写）
        // = 连接已不可用：立即关闭 socket，让 recv 线程的 poll/receive 立即
        // 报错退出 → running_ 置 false + 断连回调（既有链路）→ is_connected
        // 清零 → 后续 broadcast 快照过滤此槽位。若不关：半死连接（对端挂死/
        // 断电，TCP 未感知）会让【每一帧】都拖满 500ms 超时才失败——发送
        // 线程串行，广播队列里排在该台之后的显示端全部被逐帧拖延（台间
        // 起停时差 ≤500ms）。关闭后隔离语义变为"只拖一次"；重连由
        // DisplayConnector 看门狗既有退避机制接管（与正常掉线同路）。
        // 锁内直接关：本线程已持 socket_mutex_ 无需重入；recv 线程的 poll
        // 不持锁，与 close 竞态时 poll 抛异常 → 既有 catch 退出——断连
        // 语义不变（见 recv_loop 注释，与 stop() 同构）。
        try { socket_.shutdownSend(); } catch (...) {}
        try { socket_.close(); } catch (...) {}
    }
    return ok;
}

void ControlClient::recv_loop() {
    for (;;) {
        if (!running_.load()) break;

        // 200ms 轮询：无数据则回头检查 running_（保证 stop 及时退出）
        // 【不持锁 poll（命令延迟 17.7s 饿死修复，联测 2026-11-03 实证）】
        // 此前 poll 持 socket_mutex_ 且一持就是 200ms（轮询超时时长）——
        // 锁间隙仅微秒级，send 线程（发送命令/场景）几乎永远抢不到锁：
        // 联测实测"播放"重锚从入队到真正写出延迟 17.7s（display 退出、
        // recv 线程结束才解卡），显示端全程收不到命令——这正是用户报告
        // 的"播放/停止/暂停/改场景不实时同步"的根因。
        // poll 只读 fd：与 sendBytes（写方向）全双工并发安全；与 stop()
        // 的 close 竞态时 poll 抛异常 → catch 退出（断连语义不变）。
        bool readable = false;
        try {
            readable = socket_.poll(Poco::Timespan(200 * 1000),
                                    Poco::Net::Socket::SELECT_READ);
        } catch (const Poco::Exception& e) {
            // 【排障】poll 异常退出路径（display 断连诊断关键线索）
            std::fprintf(stderr, "[ctrl-client] poll 异常: %s\n",
                         e.displayText().c_str());
            break;
        }
        if (!readable) continue;

        char buf[64 * 1024];
        int n = 0;
        {
            std::lock_guard<std::mutex> lock(socket_mutex_);
            if (!running_.load()) break; // 锁内复查：poll 期间可能被 stop
            try {
                n = socket_.receiveBytes(buf, sizeof(buf));
            } catch (const Poco::Exception& e) {
                // 【排障】接收异常退出路径
                std::fprintf(stderr, "[ctrl-client] receive 异常: %s\n",
                             e.displayText().c_str());
                break;
            }
        }
        if (n <= 0) {
            // 【排障】对端关闭（0=优雅关闭；<0=异常）
            std::fprintf(stderr, "[ctrl-client] 对端关闭 (n=%d)\n", n);
            break;
        }

        recv_buf_.append(buf, static_cast<std::size_t>(n));

        ms_proto::Envelope env;
        for (;;) {
            const int r = FrameCodec::tryExtract(recv_buf_, env);
            if (r == 0) break;  // 半包
            if (r < 0) {        // 协议错误：断开（Editor 行为异常，不应容忍）
                // 【排障】协议错误退出路径（帧长度非法/魔数不符）
                std::fprintf(stderr, "[ctrl-client] 协议错误 (r=%d，缓冲 %zu 字节)\n",
                             r, recv_buf_.size());
                std::lock_guard<std::mutex> lock(socket_mutex_);
                try { socket_.close(); } catch (...) {}
                break;
            }
            if (on_envelope_) on_envelope_(env);
        }
    }

    const bool was_running = running_.exchange(false);
    if (was_running && on_disconnected_) on_disconnected_();
}

} // namespace ms
