// ============================================================================
//  SpoutIngest —— 同机 Spout sender 接收（Ingest 注册表 Spout 条目实现）
//
//  技术路线（2026-11-08 定案）：【纯 DX11 接收，零 GL 依赖】
//    * Spout 协议 = sender 共享纹理（老式 MISC_SHARED，uint32 共享句柄）
//      + 名字共享内存（SpoutSenderNames）+ 帧计数 semaphore（2.006+，
//      spoutFrameCount）——三者都有 C++ 直读接口，无需 GL 上下文；
//    * 官方 SpoutGL sender 创建共享纹理用 (keyed=false, NThandle=false)
//      ⇒ 接收端 OpenSharedResource 直开（SpoutGL.cpp:1298 实锤）；
//    * 帧检测：新式 sender 走 semaphore（WaitNewFrame 低开销阻塞）；
//      老 sender 无 semaphore → 退化为 ~60fps 轮询读回；
//    * 读回：CopyResource 共享纹理 → staging → Map → BGRA/R8G8B8A8 分派
//      换序拷贝 → LiveFrame（RGBA）→ sink。
//
//  与 NdiIngest 同构的生命周期契约：start() 启线程 / stop() 停线程
//  （RAII 兜底）；sink 在接收线程执行（订阅方自行保证发布安全）；
//  sender 未上线 = 离线占位 + 发现循环重试（限频日志）。
//
//  限制（第一版已知边界）：
//    * 多 GPU：共享纹理要求收发同 adapter——用系统默认 adapter（与
//      Spout 官方接收器同行为）；跨 GPU sender 打开失败 → 持续重试；
//    * latency_mode 语义保留参数位（Spout 为"最新帧"模型，天然 drain）。
// ============================================================================

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "live/IngestBase.hpp"

namespace ms {

class SpoutIngest : public IngestBase {
public:
    SpoutIngest(std::string sender_name, int latency_mode, FrameSink sink);
    ~SpoutIngest() override;

    SpoutIngest(const SpoutIngest&)            = delete;
    SpoutIngest& operator=(const SpoutIngest&) = delete;

private:
    void        recvLoop() override;
    const char* logTag() const noexcept override { return "spout"; }
};

/// Spout sender 枚举（注册表 list 包装；即时返回，wait_ms 兼容签名）
std::vector<IngestSource> listSpoutSources(std::uint32_t wait_ms);

/// Spout 工厂（注册表 create 包装）
std::unique_ptr<ILiveIngest> makeSpoutIngest(std::string sender_name,
                                             int        latency_mode,
                                             ILiveIngest::FrameSink sink);

} // namespace ms
