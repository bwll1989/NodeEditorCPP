// ============================================================================
//  SpoutIngest 实现 —— 纯 DX11 接收（零 GL 依赖；技术路线见头注释）
//
//  收帧循环（与 NdiIngest::recvLoop 同构）：
//      发现 sender（spoutSenderNames，共享内存读，即时）
//        → 打开共享纹理（OpenSharedResource，老式 MISC_SHARED 句柄）
//        → 轮询读回（~60fps 节流；DxTextureReadback 统一读回路径）
//        → sink 上抛
//  断线自愈：每轮 CheckSender 校验共享句柄/尺寸——sender 重建（改尺寸/
//  重启）自动重连；sender 消失回发现循环（限频日志）。
// ============================================================================

#include "live/SpoutIngest.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <set>

#include "live/DxReadback.hpp" // 纹理→CPU 读回（与 WgcIngest 共享）

#include "SpoutCommon.h"        // SpoutMaxSenderNameLen
#include "SpoutDirectX.h"       // spoutDirectX（DX11 设备/共享纹理打开）
#include "SpoutFrameCount.h"    // spoutFrameCount（named/keyed mutex 访问）
#include "SpoutSenderNames.h"   // spoutSenderNames（枚举/信息/校验）

namespace ms {

namespace {

/// 轮询节流目标帧间隔（60fps；最新帧语义，重复帧由快照覆盖自然消化）
constexpr DWORD kPollIntervalMs = 16;

} // namespace

// ---------------------------------------------------------------------------
// 生命周期（线程/诊断/兜底壳继承 IngestBase；构造只落配置）
// ---------------------------------------------------------------------------
SpoutIngest::SpoutIngest(std::string sender_name, int latency_mode, FrameSink sink)
    : IngestBase(std::move(sender_name), latency_mode, std::move(sink)) {}

SpoutIngest::~SpoutIngest() = default;

// ---------------------------------------------------------------------------
// 接收线程主体（IngestBase 兜底壳内执行）
// ---------------------------------------------------------------------------
void SpoutIngest::recvLoop() {
    // —— DX11 设备（线程私有；默认 adapter——与 Spout 官方接收器同行为）——
    spoutDirectX dx;
    if (!dx.OpenDirectX11()) {
        std::fprintf(stderr, "[spout] DX11 设备创建失败（Ingest 退出）\n");
        std::fflush(stderr);
        return;
    }
    ID3D11DeviceContext* context = dx.GetDX11Context();

    spoutSenderNames names;
    spoutFrameCount  fcount; // 仅用 named/keyed mutex 访问防御（不接帧计数
                             // semaphore——其依赖注册表开关，避免写用户注册表）
    DxTextureReadback readback; // staging 生命周期（尺寸变化自动重建）

    // —— 连接态（重连时清理重建）——
    ID3D11Texture2D* shared_tex = nullptr; ///< sender 共享纹理
    HANDLE  share_handle = nullptr;
    bool    have_sender = false;
    std::int64_t last_find_log = -1;
    std::chrono::steady_clock::time_point last_read;

    // sender 名工作缓冲（Spout API 为 char* 形态）
    char name[SpoutMaxSenderNameLen]{};
    strncpy_s(name, sizeof(name), source_ref_.c_str(), _TRUNCATE);

    auto disconnect = [&]() {
        if (shared_tex != nullptr) {
            dx.ReleaseDX11Texture(dx.GetDX11Device(), shared_tex);
            shared_tex = nullptr;
        }
        share_handle = nullptr;
        have_sender = false;
        readback.release();
        fcount.CloseAccessMutex();
        connected_.store(false, std::memory_order_release);
    };

    while (running_.load(std::memory_order_acquire)) {
        // ---- 发现/校验 sender（未连接 = 发现循环；已连接 = 每轮轻量校验，
        //      sender 重建/退出立即感知）----
        unsigned sw = 0, sh = 0;
        DWORD   fmt = 0;
        HANDLE  sh_handle = nullptr;
        if (!names.CheckSender(name, sw, sh, sh_handle, fmt)) {
            if (have_sender) {
                logThrottled("sender 共享信息失效，重连", last_find_log, 5000);
                disconnect();
            } else {
                logThrottled("等待 Spout sender 上线", last_find_log, 5000);
            }
            Sleep(100);
            continue;
        }

        // ---- 共享句柄变化 → 重开（sender 改分辨率 = 新纹理新句柄）----
        if (!have_sender || sh_handle != share_handle) {
            disconnect();
            if (!dx.OpenDX11shareHandle(dx.GetDX11Device(), &shared_tex,
                                        sh_handle)) {
                logThrottled("共享纹理打开失败（跨 GPU？）重试",
                             last_find_log, 5000);
                Sleep(200);
                continue;
            }
            share_handle = sh_handle;
            fcount.CreateAccessMutex(name); // named mutex（老式 sender 的
                                            // 纹理访问保护；keyed 纹理走
                                            // CheckTextureAccess 的另一分支）
            have_sender = true;
            connected_.store(true, std::memory_order_release);
            D3D11_TEXTURE2D_DESC td{};
            shared_tex->GetDesc(&td);
            std::fprintf(stderr, "[spout] 已连接 '%s' (%ux%u 格式 %d)\n",
                         name, td.Width, td.Height, static_cast<int>(td.Format));
            std::fflush(stderr);
        }

        // ---- 轮询节流 ----
        const auto now = std::chrono::steady_clock::now();
        const auto since = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_read).count();
        if (since < kPollIntervalMs) {
            Sleep(static_cast<DWORD>(kPollIntervalMs - since));
            continue;
        }
        last_read = now;

        // ---- 读回：mutex 防御 → CopyResource+Map（共享工具）→ sink ----
        const bool locked = fcount.CheckTextureAccess(shared_tex);
        LiveFramePtr frame = readback.read(context, shared_tex);
        if (locked) fcount.AllowTextureAccess(shared_tex);
        if (frame == nullptr) continue;
        emitFrame(std::move(frame));
    }

    disconnect(); // 线程收尾（纹理/mutex 清理）
}

// ---------------------------------------------------------------------------
// 注册表包装：枚举 + 工厂
// ---------------------------------------------------------------------------
std::vector<IngestSource> listSpoutSources(std::uint32_t wait_ms) {
    (void)wait_ms; // Spout 枚举即时返回（注册表共享内存直读）
    std::vector<IngestSource> out;
    spoutSenderNames names;
    std::set<std::string> senders;
    if (!names.GetSenderNames(&senders)) return out;
    for (const auto& n : senders) // set 有序——UI 列表稳定
        out.push_back({n, n});    // label=ref=sender 名
    return out;
}

std::unique_ptr<ILiveIngest> makeSpoutIngest(std::string sender_name,
                                             int        latency_mode,
                                             ILiveIngest::FrameSink sink) {
    return std::make_unique<SpoutIngest>(std::move(sender_name), latency_mode,
                                         std::move(sink));
}

} // namespace ms
