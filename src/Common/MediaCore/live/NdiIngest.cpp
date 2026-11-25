// ============================================================================
//  NdiIngest 实现 —— NDI 收端核心（发现→连接→拉帧→回调上抛）
// ============================================================================

#include "live/NdiIngest.hpp"

#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstring>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_X86)
#include <tmmintrin.h> // SSSE3 pshufb（BGRX→RGBA 批量换序；x86/x64 专属）
#define MS_NDI_X86_SIMD 1
#else
// ARM（Android NDK arm64 等）：SSE 头不存在，SIMD 段编译剔除——
// deinterleaveToRgba 恒走下方标量回退（NEON vqtbl1q 同款换序待接入）。
#define MS_NDI_X86_SIMD 0
#endif

#include "Processing.NDI.Lib.h"
#include "Processing.NDI.Find.h"
#include "Processing.NDI.Recv.h"

#include "clock/ClockProvider.hpp"

namespace ms {

namespace {

// ---------------------------------------------------------------------------
// NDI 运行时引用计数（与 LiveHub.cpp 同语义：多实例共存只初始化一次；
// 收端逻辑唯一实现在本 TU，LiveMediaSource 已无 NDI 运行时调用）
// ---------------------------------------------------------------------------
std::atomic<int>& ndiRefs() {
    static std::atomic<int> refs{0};
    return refs;
}

bool ndiAcquire() {
    if (ndiRefs().fetch_add(1, std::memory_order_acq_rel) == 0) {
        if (!NDIlib_initialize()) {
            ndiRefs().fetch_sub(1, std::memory_order_acq_rel);
            return false;
        }
    }
    return true;
}

void ndiRelease() {
    // 【2026-11-10 切换崩溃修复】只减计数、不再调 NDIlib_destroy()——
    // destroy 会停掉 NDI 全局后台线程（发现服务/收发队列），同进程内任何
    // 其他 receiver/sender/finder 瞬间失效。切换实况源时旧收端析构（渲染
    // 线程——shared_ptr 最后持有者）与新收端装配（驱动线程）并发：旧
    // release 的 destroy 与新实例的 NDI API 调用竞态 = display 崩溃根因。
    // 计数保留仅防"重复 initialize"（NDIlib_initialize 自带重入计数，多
    // 次调用安全）；进程生命周期内运行时常驻，退出由 OS 统一回收。
    // LiveHub.cpp 同名计数同语义同步修改（两 TU 各自独立计数、TU 内配对）。
    ndiRefs().fetch_sub(1, std::memory_order_acq_rel);
}

/// 源名匹配：全名精确 或 "MACHINE (源名)" 后缀。比较前先做归一化
/// （小写 + 连续非字母数字折叠为单个空格）——NDI 发现列表会把源名中
/// 的 '/' 等非法字符替换为空格（实测 2026-11-08 质量探针：Hub 再发布
/// "ms-live/qa/live-qa" 在列表显示 "ms-live qa live-qa"，不归一则匹配
/// 恒失败 = 二代链路不通根因）。两侧同规则折叠，结果天然一致。
std::string normalizeName(const char* s) {
    std::string out;
    bool last_sep = true; // 抑制前导与连续分隔符
    for (const char* p = s ? s : ""; *p; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if (std::isalnum(c)) {
            out.push_back(static_cast<char>(std::tolower(c)));
            last_sep = false;
        } else if (!last_sep) {
            out.push_back(' ');
            last_sep = true;
        }
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

bool sourceNameMatches(const char* ndi_name, const std::string& wanted) {
    if (ndi_name == nullptr || wanted.empty()) return false;
    const std::string full = normalizeName(ndi_name);
    const std::string want = normalizeName(wanted.c_str());
    if (want.empty()) return false;
    if (full == want) return true;
    // "MACHINE (源名)" 形态：括号归一化后已折叠为空格 → 按 " want" 尾匹配
    const std::string tail = " " + want;
    return full.size() > tail.size() &&
           full.compare(full.size() - tail.size(), tail.size(), tail) == 0;
}

/// "host:port" 直连串判定（冒号后纯数字）。命中 → 免发现直连（NDI 6
/// NDIlib_source_t：p_ndi_name 置空 + p_url_address=host:port，SDK 跳过
/// mDNS 按 URL 直接建收器）。动机：mDNS 组播在受限网络不可达——安卓
/// 模拟器 NAT、组播被过滤的 WiFi、跨网段部署；NDI 源名（"MACHINE (名)"
/// / "ms-live/..."）含 '/' 或机器名，与 "数字端口" 尾天然不冲突。
/// host 允许 IPv4/主机名；IPv6 字面量含多冒号 → rfind(':') 取最后一段，
/// 同样要求纯数字尾（含端口的 IPv6 字面量应写成 [::1]:5961，rfind 落在
/// 端口冒号上，判定仍成立）。
bool isHostPort(const std::string& s) {
    const auto pos = s.rfind(':');
    if (pos == std::string::npos || pos + 1 >= s.size()) return false;
    for (std::size_t i = pos + 1; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}

/// 限频日志（5s 一条）——共享实现见 IngestBase::logThrottled

// ---------------------------------------------------------------------------
// deinterleaveToRgba —— BGRX/BGRA/RGBX → RGBA 装配（2026-11-23 性能修复）
//
// 【热点】原逐像素标量循环（d[0]=s[2]…四路散读散写）1080p60 实测独占
// 约一整核——RGBX_RGBA 请求下源无 alpha 恒交付 BGRX（见
// publishNdiFrame 布局分派注释），swap_rb 慢路径是常态而非异常。
//
// 优化：SSSE3 pshufb 一次重排 16 字节（4 像素），换序吞吐 ≈ 标量 8~10
// 倍；RGBX（仅补 alpha）同样吃 SIMD or 收益。依赖 SSSE3（2006 年后
// 全部 x64 CPU 标配；MSVC x64 intrinsic 直接编码 pshufb，无需 arch 开关）。
// 【架构适配 2026-11-23】SSE intrinsic 仅 x86/x64——ARM 编译（Android
// NDK arm64）以 MS_NDI_X86_SIMD=0 剔除 SIMD 段，恒走标量回退；NEON
// vqtbl1q_u8 同款换序（NEON 恒有，无需运行时探测）留待实测后接入。
//
// 行内按 16 字节块处理（w%4==0 时整块无尾段——视频分辨率几乎恒满足）；
// 不满足回退标量行。alpha 缺失域（RGBX/BGRX）or 0xFF000000 补满。
// ---------------------------------------------------------------------------
void deinterleaveToRgba(const std::uint8_t* src, int src_stride,
                        std::uint8_t* dst, int dst_stride,
                        int w, int h, bool swap_rb, bool has_alpha) {
    const int row_bytes = w * 4;
    // 无换序且行内无填充：整帧 memcpy 快路径（RGBA 源）
    if (!swap_rb && has_alpha && src_stride == dst_stride) {
        std::memcpy(dst, src, static_cast<std::size_t>(dst_stride) * h);
        return;
    }
#if MS_NDI_X86_SIMD
    const bool simd = (w % 4) == 0;
    // pshufb 掩码：每像素字节序 [B G R X] → [R G B A]（低字节在前）
    const __m128i shuf = _mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7,
                                       10, 9, 8, 11, 14, 13, 12, 15);
    const __m128i amask = _mm_set1_epi32(static_cast<int>(0xFF000000u));
#endif
    for (int y = 0; y < h; ++y) {
        const std::uint8_t* s = src + static_cast<std::ptrdiff_t>(y) * src_stride;
        std::uint8_t* d = dst + static_cast<std::ptrdiff_t>(y) * dst_stride;
#if MS_NDI_X86_SIMD
        if (simd) {
            auto*       sv = reinterpret_cast<const __m128i*>(s);
            __m128i*    dv = reinterpret_cast<__m128i*>(d);
            for (int i = 0; i < w; i += 4, ++sv, ++dv) {
                __m128i v = _mm_loadu_si128(sv);
                if (swap_rb)   v = _mm_shuffle_epi8(v, shuf);
                if (!has_alpha) v = _mm_or_si128(v, amask);
                _mm_storeu_si128(dv, v);
            }
        } else
#endif
        {
            for (int x = 0; x < w; ++x) {
                d[x * 4 + 0] = swap_rb ? s[x * 4 + 2] : s[x * 4 + 0]; // R
                d[x * 4 + 1] = s[x * 4 + 1];                          // G
                d[x * 4 + 2] = swap_rb ? s[x * 4 + 0] : s[x * 4 + 2]; // B
                d[x * 4 + 3] = has_alpha ? s[x * 4 + 3] : 0xFF;       // A
            }
        }
    }
}

} // namespace

// ---------------------------------------------------------------------------
// listSources：一次性源枚举（临时发现器；UI「刷新」按钮）
// ---------------------------------------------------------------------------
std::vector<std::string> NdiIngest::listSources(std::uint32_t wait_ms) {
    std::vector<std::string> names;
    if (!ndiAcquire()) return names;
    NDIlib_find_create_t desc{};
    desc.show_local_sources = true;
    auto* finder = NDIlib_find_create_v2(&desc);
    if (finder != nullptr) {
        NDIlib_find_wait_for_sources(finder, wait_ms);
        std::uint32_t n = 0;
        const NDIlib_source_t* sources =
            NDIlib_find_get_current_sources(finder, &n);
        names.reserve(n);
        for (std::uint32_t i = 0; i < n; ++i)
            if (sources[i].p_ndi_name != nullptr)
                names.emplace_back(sources[i].p_ndi_name);
        NDIlib_find_destroy(finder);
    }
    ndiRelease();
    return names;
}

// ---------------------------------------------------------------------------
// 构造 / 析构（线程/诊断继承 IngestBase）
// ---------------------------------------------------------------------------
NdiIngest::NdiIngest(std::string source_name, int latency_mode,
                     std::int64_t schedule_delay_us, FrameSink sink)
    : IngestBase(std::move(source_name), latency_mode, schedule_delay_us,
                 std::move(sink)) {
    // Hub 再发布源标记缓存（构造期判定一次，免去逐帧字符串前缀比较）：
    // 命中时帧同步排程锚走 NDI timecode（发送端写入的对齐时钟戳，已含
    // 应显时刻偏移——发送端捕获时已加延迟预算，2026-11-25 定案），
    // 见 publishNdiFrame。
    hub_source_ = source_ref_.rfind("ms-live/", 0) == 0;
}

NdiIngest::~NdiIngest() {
    stop();
    if (ndi_held_) { // 运行时引用配对释放（2026-11-09 计数修正——旧版
                     // stop 无条件 release 且析构兜底再调 = 计数双减）
        ndiRelease();
        ndi_held_ = false;
    }
}

bool NdiIngest::onStart() {
    if (!ndi_held_) { // 实例级持有一份（重复 start 不重复 acquire）
        if (!ndiAcquire()) return false;
        ndi_held_ = true;
    }
    return true;
}

void NdiIngest::onCleanup() {
    // worker 已停（join 后）：无并发——仅销毁 NDI 实例，不动运行时计数
    if (recv_ != nullptr) {
        NDIlib_recv_destroy(static_cast<NDIlib_recv_instance_t>(recv_));
        recv_ = nullptr;
    }
    if (finder_ != nullptr) {
        NDIlib_find_destroy(static_cast<NDIlib_find_instance_t>(finder_));
        finder_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// 接收线程主体：发现 → 连接 → 拉帧循环（IngestBase::recvLoop 纯虚实现；
// 帧经 sink_ 上抛给 LiveHub 分发，不落本地快照缓冲）
// ---------------------------------------------------------------------------
void NdiIngest::recvLoop() {
    std::int64_t last_find_log = -1;
    const bool   drain         = latency_mode_ != 1; // 0=最低（宁丢勿延迟）

    while (running_.load(std::memory_order_acquire)) {
        // ---- 无接收器：发现源 → 创建并连接 ----
        if (recv_ == nullptr) {
            NDIlib_source_t      direct_src{};
            const NDIlib_source_t* hit = nullptr;
            if (isHostPort(source_ref_)) {
                // ---- 直连模式：免发现（见 isHostPort 注释）----
                direct_src.p_ndi_name    = ""; // 置空 → SDK 直用 url_address
                direct_src.p_url_address = source_ref_.c_str();
                hit = &direct_src;
            } else {
                // ---- 发现模式：mDNS 按名匹配 ----
                if (finder_ == nullptr) {
                    NDIlib_find_create_t find_desc{};
                    find_desc.show_local_sources = true; // 本机源（本机其他应用的 NDI 输出）
                    finder_ = NDIlib_find_create_v2(&find_desc);
                    if (finder_ == nullptr) {
                        logThrottled("NDI 发现器创建失败", last_find_log, 5000);
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        continue;
                    }
                }
                NDIlib_find_wait_for_sources(
                    static_cast<NDIlib_find_instance_t>(finder_), 500);
                std::uint32_t n = 0;
                const NDIlib_source_t* sources = NDIlib_find_get_current_sources(
                    static_cast<NDIlib_find_instance_t>(finder_), &n);
                for (std::uint32_t i = 0; i < n; ++i) {
                    if (sourceNameMatches(sources[i].p_ndi_name, source_ref_)) {
                        hit = &sources[i];
                        break;
                    }
                }
                if (hit == nullptr) {
                    logThrottled("等待 NDI 源 '" + source_ref_ + "' 上线…",
                                 last_find_log, 5000);
                    continue;
                }
            }
            // RGBX_RGBA 直出（零颜色转换）+ 最高带宽（不降分辨率）+ 渐进化
            NDIlib_recv_create_v3_t desc{};
            desc.source_to_connect_to = *hit;
            desc.color_format         = NDIlib_recv_color_format_RGBX_RGBA;
            desc.bandwidth            = NDIlib_recv_bandwidth_highest;
            desc.allow_video_fields   = false;
            desc.p_ndi_recv_name      = "Flow-Hub";
            recv_ = NDIlib_recv_create_v3(&desc);
            if (recv_ == nullptr) {
                logThrottled("NDI 接收器创建失败", last_find_log, 5000);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
            NDIlib_recv_connect(static_cast<NDIlib_recv_instance_t>(recv_), hit);
            connected_.store(true, std::memory_order_release);
            std::fprintf(stderr,
                         "live-ingest: %p 已连接 NDI 源 '%s'（latency=%d）\n",
                         static_cast<void*>(this),
                         hit->p_ndi_name ? hit->p_ndi_name
                                         : source_ref_.c_str(),
                         latency_mode_);
            std::fflush(stderr);
        }

        // ---- 拉帧（双模式：drain=宁丢勿延迟 / 直通=逐帧）----
        // 【2026-11-10 断线重连修复】两分支统一处理 error（连接丢失）：
        // 旧版 drain 分支 error 直接 break 且不销毁 receiver——Hub 再发布
        // 流短暂中断（切换源/重建 sender）后 receiver 绑定的旧 url 永久
        // 失效，源再上线也收不到帧 = display "经常性没显示"根因（默认
        // latency=0 恒走 drain 分支）。error → 销毁 receiver → 走上方
        // 发现→匹配→连接流程重建（源名匹配与新源 url 无关，自动跟随）。
        if (drain) {
            int drained = 0;
            NDIlib_frame_type_e last = NDIlib_frame_type_none;
            for (;;) {
                NDIlib_video_frame_v2_t v{};
                const NDIlib_frame_type_e t = NDIlib_recv_capture_v2(
                    static_cast<NDIlib_recv_instance_t>(recv_), &v, nullptr,
                    nullptr, 0);
                if (t == NDIlib_frame_type_video) {
                    publishNdiFrame(&v);
                    NDIlib_recv_free_video_v2(
                        static_cast<NDIlib_recv_instance_t>(recv_), &v);
                    ++drained;
                    continue;
                }
                last = t; // none（队列空）/ error（断线）
                break;
            }
            if (drained > 1)
                frames_dropped_.fetch_add(drained - 1,
                                          std::memory_order_relaxed);
            if (drained == 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(6));
            if (last == NDIlib_frame_type_error) {
                std::fprintf(stderr, "live-ingest: 连接断开，重连 '%s'\n",
                             source_ref_.c_str());
                std::fflush(stderr);
                NDIlib_recv_destroy(static_cast<NDIlib_recv_instance_t>(recv_));
                recv_ = nullptr;
                connected_.store(false, std::memory_order_release);
            }
        } else {
            NDIlib_video_frame_v2_t v{};
            const NDIlib_frame_type_e t = NDIlib_recv_capture_v2(
                static_cast<NDIlib_recv_instance_t>(recv_), &v, nullptr,
                nullptr, 16);
            if (t == NDIlib_frame_type_video) {
                publishNdiFrame(&v);
                NDIlib_recv_free_video_v2(
                    static_cast<NDIlib_recv_instance_t>(recv_), &v);
            } else if (t == NDIlib_frame_type_error) {
                std::fprintf(stderr, "live-ingest: 连接断开，重连 '%s'\n",
                             source_ref_.c_str());
                std::fflush(stderr);
                NDIlib_recv_destroy(static_cast<NDIlib_recv_instance_t>(recv_));
                recv_ = nullptr;
                connected_.store(false, std::memory_order_release);
            }
        }
    }
    connected_.store(false, std::memory_order_release);
}

// ---------------------------------------------------------------------------
// 发布一帧：NDI 帧缓冲 → LiveFrame 快照 → sink 上抛
//
// 字节序按实际 FourCC 分派（实测 2026-11-08 质量探针教训）：RGBX_RGBA
// 颜色请求下 SDK 按源 alpha 有无交付不同布局（SDK 头注释 "no alpha →
// BGRX / with alpha → RGBA"）——Full NDI 压缩不保 alpha，RGBA 源通常
// 以 BGRX 到达；若一律按 R,G,B,X 解析则 R/B 互换（display 端红蓝颠倒、
// 探针 PSNR 13.4dB 假象）。
// ---------------------------------------------------------------------------
void NdiIngest::publishNdiFrame(const void* ndi_video_frame) {
    const auto* v = static_cast<const NDIlib_video_frame_v2_t*>(ndi_video_frame);
    if (v == nullptr || v->p_data == nullptr || v->xres <= 0 || v->yres <= 0)
        return;

    // 【2026-11-25 帧率取证】收端实际交付帧率（5s 一条）——用户实测
    // "NDI 幻灯片"而网络占用仅 5.9Mbps（带宽无关），链路四段（采集→
    // 再发布→网络→收端）各打速率定位瓶颈段：本行 = 收端交付速率。
    {
        using namespace std::chrono;
        const std::int64_t now_ms =
            duration_cast<milliseconds>(steady_clock::now().time_since_epoch())
                .count();
        static thread_local std::int64_t   s_last_ms = 0;
        static thread_local std::uint64_t  s_last_n  = 0;
        if (s_last_ms == 0) {
            s_last_ms = now_ms;
            s_last_n  = frames_received_.load(std::memory_order_relaxed);
        } else if (now_ms - s_last_ms >= 5000) {
            const std::uint64_t n =
                frames_received_.load(std::memory_order_relaxed);
            std::fprintf(stderr,
                         "live-ingest: 收帧速率 %.1f fps（累计收 %llu 丢 %llu）"
                         " %dx%d\n",
                         double(n - s_last_n) / double(now_ms - s_last_ms) *
                             1000.0,
                         static_cast<unsigned long long>(n),
                         static_cast<unsigned long long>(
                             frames_dropped_.load(std::memory_order_relaxed)),
                         v->xres, v->yres);
            std::fflush(stderr);
            s_last_ms = now_ms;
            s_last_n  = n;
        }
    }

    // ---- 布局分派：has_alpha（第 4 字节语义）+ swap_rb（源是否 B 在前）----
    bool has_alpha = false, swap_rb = false;
    switch (v->FourCC) {
        case NDIlib_FourCC_video_type_RGBA: has_alpha = true;                break;
        case NDIlib_FourCC_video_type_BGRA: has_alpha = true; swap_rb = true; break;
        case NDIlib_FourCC_video_type_BGRX:                   swap_rb = true; break;
        case NDIlib_FourCC_video_type_RGBX:                                   break;
        default: // RGBX_RGBA 请求下不应出现（UYVY 等仅限对应颜色请求）
            logThrottled("不支持的 NDI 帧格式 FourCC=" +
                             std::to_string(static_cast<int>(v->FourCC)) +
                             "（帧丢弃）",
                         unsupported_log_ms_, 5000);
            return;
    }

    // 池化取帧（rgba 缓冲复用——同分辨率 resize 恒 no-op，消每帧
    // 8MB 堆分配 + first-touch 缺页；LiveFramePool.hpp）
    auto frame = newFrame();
    frame->w   = v->xres;
    frame->h   = v->yres;
    frame->rgba.resize(static_cast<std::size_t>(v->xres) * v->yres * 4);
    // 帧同步戳锚点（2026-11-24 重构）：Hub 再发布源（"ms-live/" 前缀）
    // 优先用发送端写入的 NDI timecode（编辑端对齐时钟捕获戳，单位
    // 100ns）——NDI 单播各收端连接交付相位不同（TCP 步调/解压耗时/连接
    // 时刻），按本地到达时刻排程会把相位差固化成端间画面恒定偏移（实测
    // 2026-11-24）。timecode 与预览排程同戳同域（SNTP 对齐），收端直接
    // 换算。旧版发送端（timecode=0）或窗口外异常值退回本地接收戳（原
    // 语义，外部 NDI 源直收恒走此路径）。
    const std::int64_t now_us = processAlignedNowUs();
    bool anchored = false;
    if (hub_source_ && v->timecode != 0 &&
        v->timecode != NDIlib_recv_timestamp_undefined) {
        const std::int64_t tc_us = v->timecode / 10;
        // 同钟域 sanity：SNTP 正常对齐残差亚毫秒级；网络延迟正常 <1s。
        // 窗从 ±10s 收紧到 ±2s（2026-11-25）：两机时钟漂移几百 ms~数秒
        // （SNTP 失效）时 ±10s 窗放行 → 排程恒错位（帧到达即过期/恒未
        // 到期）= 跨机"幻灯片"根因之一；±2s 外退回本地接收戳（LiveMedia
        // Source 另有 5s 滑窗自愈覆盖病态中间态）。
        if (tc_us > now_us - 2'000'000 && tc_us < now_us + 2'000'000) {
            frame->pts_us = tc_us; // 应显时刻（发送端已加延迟预算），透传
            anchored      = true;
        } else {
            logThrottled("NDI timecode 超出对齐时钟窗口（" +
                             std::to_string(tc_us) + " vs " +
                             std::to_string(now_us) + "），退回接收戳",
                         tc_anchor_log_ms_, 5000);
        }
    }
    if (!anchored) frame->pts_us = now_us + schedule_delay_us_;

    const int          w      = v->xres;
    const int          h      = v->yres;
    const int          stride = v->line_stride_in_bytes > 0
                                    ? v->line_stride_in_bytes
                                    : w * 4;
    const std::uint8_t* src   = v->p_data;
    std::uint8_t*       dst   = frame->rgba.data();

    // 像素装配（SIMD 主路径 + 标量回退 + memcpy 快路径；见函数注释）：
    // BGRX/BGRA 换序、RGBX 补 alpha——三者曾是逐像素标量热点
    deinterleaveToRgba(src, stride, dst, w * 4, w, h, swap_rb, has_alpha);

    emitFrame(std::move(frame));
}

} // namespace ms
