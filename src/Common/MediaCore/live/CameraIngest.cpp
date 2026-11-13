// ============================================================================
//  CameraIngest 实现 —— MF Source Reader 同步拉帧（技术路线见头注释；
//  线程生命周期/诊断/兜底壳继承 IngestBase）
// ============================================================================

#include "live/CameraIngest.hpp"

#include <atomic>
#include <cstdio>
#include <cstring>

#include <windows.h>

#include <mfapi.h>        // MFStartup/MFEnumDeviceSources/MFCreateMediaType
#include <mfidl.h>        // MF_DEVSOURCE_* 属性键 + IMFMediaSource
#include <mfreadwrite.h>  // IMFSourceReader（Source Reader API）

#include <wrl/client.h>

namespace ms {

namespace {

using Microsoft::WRL::ComPtr;

// ---------------------------------------------------------------------------
// 线程级 COM/MF 初始化守卫（RAII 逆序清理：MFShutdown → CoUninitialize）
// ---------------------------------------------------------------------------
struct MfThreadGuard {
    bool co_init = false; ///< COM 由本守卫初始化（负责配对 Uninit）
    bool mf_init = false; ///< MF 由本守卫初始化（负责配对 Shutdown）

    bool init() {
        // MTA 请求失败（RPC_E_CHANGED_MODE=线程已 STA）不致命——MF 不
        // 强制 apartment 模式，继续用；仅本守卫不再负责 Uninit
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        co_init = SUCCEEDED(hr);
        if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET))) {
            if (co_init) { CoUninitialize(); co_init = false; }
            return false;
        }
        mf_init = true;
        return true;
    }

    ~MfThreadGuard() {
        if (mf_init) MFShutdown();
        if (co_init) CoUninitialize();
    }
};

// ---------------------------------------------------------------------------
// 设备打开：SymbolicLink → IMFMediaSource → IMFSourceReader
// ---------------------------------------------------------------------------
ComPtr<IMFSourceReader> openReader(const std::string& symbolic_link_u8) {
    ComPtr<IMFAttributes> attrs;
    if (FAILED(MFCreateAttributes(attrs.GetAddressOf(), 2))) return nullptr;
    if (FAILED(attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                              MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
        return nullptr;
    const std::wstring link = wideOf(symbolic_link_u8);
    if (link.empty()) return nullptr;
    if (FAILED(attrs->SetString(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
            link.c_str())))
        return nullptr;

    ComPtr<IMFMediaSource> source;
    if (FAILED(MFCreateDeviceSource(attrs.Get(), source.GetAddressOf())))
        return nullptr; // 设备不存在/被占用——调用方 500ms 重试

    ComPtr<IMFAttributes> rattrs;
    MFCreateAttributes(rattrs.GetAddressOf(), 2);
    // CPU 侧读回：关硬件加速变换（不必要，也避免驱动差异）
    rattrs->SetUINT32(MF_SOURCE_READER_DISABLE_DXVA, TRUE);
    // 【2026-11-10 修复"RGB32 不被接受"】允许 Source Reader 插入视频处理
    // MFT 做任意原生格式→目标格式转换——大量 UVC 摄像头原生只出 YUY2/
    // MJPG/NV12（无原生 RGB32 档），未开此 flag 时 SetCurrentMediaType
    // (RGB32) 依赖内置 Color Converter 独力完成（MJPG 还需先解码），驱动
    // 不给原生 RGB32 时必失败（用户实测"输出格式配置失败，重试"死循环，
    // live track camera 不可用）。开启后系统插完整视频处理器（含 MJPEG
    // 解码 + YUV→RGB 转换），RGB32 目标几乎恒可满足。
    rattrs->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING,
                      TRUE);

    ComPtr<IMFSourceReader> reader;
    if (FAILED(MFCreateSourceReaderFromMediaSource(
            source.Get(), rattrs.Get(), reader.GetAddressOf())))
        return nullptr;
    return reader;
}

// ---------------------------------------------------------------------------
// 输出配置：强制 RGB32（MF 自动插颜色转换器）；读回实际帧尺寸与行距
// （MF 转换器输出可能 bottom-up——stride 为负，拷贝时按行翻转）
// ---------------------------------------------------------------------------
bool configureOutput(IMFSourceReader* reader, int& w, int& h, int& stride) {
    ComPtr<IMFMediaType> type;
    if (FAILED(MFCreateMediaType(type.GetAddressOf()))) return false;
    if (FAILED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)))
        return false;
    if (FAILED(type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32)))
        return false;
    if (FAILED(type->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE)))
        return false;
    if (FAILED(type->SetUINT32(MF_MT_INTERLACE_MODE,
                               MFVideoInterlace_Progressive)))
        return false;
    // 帧 size 不指定——取设备原生分辨率（采集卡/摄像头各自的默认档）
    const DWORD stream = MF_SOURCE_READER_FIRST_VIDEO_STREAM;
    // 尝试 1：partial type（仅指定 RGB32 目标）直配——Source Reader 自动
    // 插视频处理器完成原生格式转换（advanced processing 已开）
    bool ok = SUCCEEDED(reader->SetCurrentMediaType(stream, nullptr,
                                                    type.Get()));
    if (!ok) {
        // 尝试 2【两步法回退】：先钉住设备原生档（设备自己的格式必收），
        // 再要求转换到 RGB32——部分驱动对"凭空 partial type"协商失败，
        // 但先落原生档后可转换（Source Reader 转换链以原生档为基准）。
        // 全部原生档逐个试（取第一个能转 RGB32 的档位）。
        for (DWORD i = 0; !ok; ++i) {
            ComPtr<IMFMediaType> nat;
            if (FAILED(reader->GetNativeMediaType(stream, i,
                                                  nat.GetAddressOf())))
                break; // 原生档枚举到头
            if (FAILED(reader->SetCurrentMediaType(stream, nullptr,
                                                   nat.Get())))
                continue; // 该档被拒（罕见）——下一档
            ok = SUCCEEDED(reader->SetCurrentMediaType(stream, nullptr,
                                                       type.Get()));
        }
    }
    if (!ok) return false;

    ComPtr<IMFMediaType> actual;
    if (FAILED(reader->GetCurrentMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM, actual.GetAddressOf())))
        return false;
    // 防呆：协商结果必须是 RGB32——frameFromSample 按 BGRA swizzle 解包，
    // Source Reader 静默回退到 YUV 直通时会静默错色（宁可失败重试走
    // 诊断 dump，不让绿紫色画面流进合成）
    GUID sub = {};
    if (FAILED(actual->GetGUID(MF_MT_SUBTYPE, &sub)) ||
        sub != MFVideoFormat_RGB32)
        return false;
    UINT32 w32 = 0, h32 = 0;
    if (FAILED(MFGetAttributeSize(actual.Get(), MF_MT_FRAME_SIZE, &w32, &h32)))
        return false;
    w = static_cast<int>(w32);
    h = static_cast<int>(h32);
    if (w <= 0 || h <= 0) return false;
    // 默认行距：缺席时按 top-down 紧排（w*4）；实际值可能为负（bottom-up）
    stride = static_cast<int>(MFGetAttributeUINT32(
        actual.Get(), MF_MT_DEFAULT_STRIDE, 0));
    if (stride == 0) stride = w * 4;
    return true;
}

/// 诊断（2026-11-10）：RGB32 配置失败时 dump 设备原生输出档（前 8 个：
/// subtype FOURCC + 分辨率）——定位"设备到底给什么"（YUY2/MJPG/NV12…），
/// 为后续格式回退链提供事实依据。
void dumpNativeFormats(IMFSourceReader* reader) {
    for (DWORD i = 0;; ++i) {
        ComPtr<IMFMediaType> t;
        if (FAILED(reader->GetNativeMediaType(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, t.GetAddressOf()))) {
            if (i == 0)
                std::fprintf(stderr, "[camera] 原生档枚举失败\n");
            break;
        }
        GUID sub = {};
        char  cc[5] = {0, 0, 0, 0, 0};
        if (SUCCEEDED(t->GetGUID(MF_MT_SUBTYPE, &sub))) {
            const DWORD d = sub.Data1; // FOURCC 型 subtype：Data1=四字符
            cc[0] = char(d & 0xFF);
            cc[1] = char((d >> 8) & 0xFF);
            cc[2] = char((d >> 16) & 0xFF);
            cc[3] = char((d >> 24) & 0xFF);
        }
        UINT32 w32 = 0, h32 = 0;
        MFGetAttributeSize(t.Get(), MF_MT_FRAME_SIZE, &w32, &h32);
        std::fprintf(stderr, "[camera-diag] 原生档#%u: %.4s %ux%u\n", i, cc,
                     w32, h32);
        if (i >= 7) { // 防刷屏：只列前 8 档
            std::fprintf(stderr, "[camera-diag] …（其余省略）\n");
            break;
        }
    }
    std::fflush(stderr);
}

// ---------------------------------------------------------------------------
// 样本 → LiveFrame：BGRA swizzle RGBA + 行序归一（bottom-up 翻转）+
// alpha 置 0xFF（RGB32 alpha 未定义，与 NDI/WGC 不透明帧对齐契约）
// ---------------------------------------------------------------------------
LiveFramePtr frameFromSample(IMFSample* sample, int w, int h, int stride) {
    ComPtr<IMFMediaBuffer> buf;
    if (FAILED(sample->ConvertToContiguousBuffer(buf.GetAddressOf())))
        return nullptr;
    BYTE* data = nullptr;
    DWORD max_len = 0, cur_len = 0;
    if (FAILED(buf->Lock(&data, &max_len, &cur_len))) return nullptr;

    const int    abs_stride = stride < 0 ? -stride : stride; // 已保证非 0
    const bool   bottom_up  = stride < 0;
    const size_t need       = static_cast<std::size_t>(abs_stride) * h;
    if (cur_len < need) { // buffer 小于预期（格式失配）——丢帧防越界读
        buf->Unlock();
        return nullptr;
    }

    auto f  = std::make_shared<LiveFrame>();
    f->w    = w;
    f->h    = h;
    f->rgba.resize(static_cast<std::size_t>(w) * h * 4);

    const auto base = reinterpret_cast<std::uintptr_t>(data);
    if ((base & 3) == 0) {
        // ---- 快路径：4 字节对齐——uint32 位运算重组（每像素一次读写；
        //      内存 [B,G,R,A]=0xAARRGGBB → [R,G,B,A]=0xAABBGGRR，alpha
        //      直接置满；/O2 下可自动向量化）----
        for (int y = 0; y < h; ++y) {
            const BYTE* row = bottom_up
                ? data + static_cast<std::size_t>(h - 1 - y) * abs_stride
                : data + static_cast<std::size_t>(y) * abs_stride;
            const auto* src = reinterpret_cast<const std::uint32_t*>(row);
            auto* dst = reinterpret_cast<std::uint32_t*>(
                f->rgba.data() + static_cast<std::size_t>(y) * w * 4);
            for (int x = 0; x < w; ++x) {
                const std::uint32_t px = src[x];
                dst[x] = ((px & 0x00FF0000u) >> 16)  // R ← 第 3 字节
                       |  (px & 0x0000FF00u)         // G 原位
                       | ((px & 0x000000FFu) << 16)  // B ← 第 1 字节
                       |  0xFF000000u;               // alpha 置满
            }
        }
    } else {
        // ---- 兜底路径：未对齐 buffer——逐字节（正确性优先，罕见）----
        for (int y = 0; y < h; ++y) {
            const BYTE* row = bottom_up
                ? data + static_cast<std::size_t>(h - 1 - y) * abs_stride
                : data + static_cast<std::size_t>(y) * abs_stride;
            std::uint8_t* dst =
                f->rgba.data() + static_cast<std::size_t>(y) * w * 4;
            for (int x = 0; x < w; ++x) {
                dst[0] = row[x * 4 + 2]; // R
                dst[1] = row[x * 4 + 1]; // G
                dst[2] = row[x * 4 + 0]; // B
                dst[3] = 0xFF;           // alpha 强制不透明
            }
        }
    }
    buf->Unlock();
    return f;
}

} // namespace

// ---------------------------------------------------------------------------
// 生命周期（配置透传；线程/诊断/兜底全在 IngestBase）
// ---------------------------------------------------------------------------
CameraIngest::CameraIngest(std::string source_ref, int latency_mode,
                           FrameSink sink)
    : IngestBase(std::move(source_ref), latency_mode, std::move(sink)) {}

CameraIngest::~CameraIngest() = default; // 基类 RAII 兜底停线程

// ---------------------------------------------------------------------------
// 接收主体（IngestBase 兜底壳内执行）：开设备→配输出→拉帧循环；
// 设备丢失/格式变化自愈重开
// ---------------------------------------------------------------------------
void CameraIngest::recvLoop() {
    // ---- 线程级 COM + MF 初始化（守卫析构逆序清理；失败即退出）----
    MfThreadGuard guard;
    if (!guard.init()) {
        std::fprintf(stderr, "[camera] MF 初始化失败（Ingest 退出）\n");
        std::fflush(stderr);
        return;
    }

    std::int64_t last_find_log = -1;

    while (running_.load(std::memory_order_acquire)) {
        // ---- 打开设备（不存在/被占用 → 节流日志 + 500ms 重试）----
        ComPtr<IMFSourceReader> reader = openReader(source_ref_);
        if (reader == nullptr) {
            logThrottled("设备打开失败（不存在或被占用），重试",
                         last_find_log, 5000);
            Sleep(500);
            continue;
        }

        // ---- 输出配置（RGB32；读回尺寸与行距）----
        int w = 0, h = 0, stride = 0;
        if (!configureOutput(reader.Get(), w, h, stride)) {
            logThrottled("输出格式配置失败（RGB32 不被接受），重试",
                         last_find_log, 5000);
            dumpNativeFormats(reader.Get()); // 诊断：设备原生档位留痕
            reader.Reset();
            Sleep(500);
            continue;
        }

        connected_.store(true, std::memory_order_release);
        std::fprintf(stderr, "[camera] 开始采集 %s (%dx%d, stride=%d)\n",
                     source_ref_.c_str(), w, h, stride);
        std::fflush(stderr);

        // ---- 拉帧循环（同步 ReadSample 阻塞至下一帧——自带设备帧率
        //      节奏；失败/流结束/格式变化 = 重开循环自愈）----
        while (running_.load(std::memory_order_acquire)) {
            DWORD flags = 0;
            ComPtr<IMFSample> sample;
            const HRESULT hr = reader->ReadSample(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM, /*flags=*/0, nullptr,
                &flags, nullptr, sample.GetAddressOf());
            if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
                break; // 设备拔出/流终止——重开（外层重新枚举+建立）
            if ((flags & (MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED |
                          MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)) != 0)
                break; // 热切换分辨率/格式——重开重配（w/h 缓存已失效）
            if (sample == nullptr) { // 格式切换等过渡事件——防忙转
                Sleep(5);
                continue;
            }
            emitFrame(frameFromSample(sample.Get(), w, h, stride));
        }

        // ---- 本轮清理（设备拔出/停止——重走外层打开循环）----
        reader.Reset();
        connected_.store(false, std::memory_order_release);
        if (running_.load(std::memory_order_acquire))
            logThrottled("设备断开，重连", last_find_log, 5000);
    }
}

// ---------------------------------------------------------------------------
// 注册表包装：枚举 + 工厂
// ---------------------------------------------------------------------------
std::vector<IngestSource> listCameraSources(std::uint32_t wait_ms) {
    (void)wait_ms; // 同步枚举即时返回
    std::vector<IngestSource> out;

    MfThreadGuard guard;
    if (!guard.init()) return out;

    IMFActivate** activates = nullptr;
    UINT32 count = 0;
    IMFAttributes* enum_attrs = nullptr;
    // 枚举约束：只列视频捕获设备（摄像头/采集卡）——首参是属性集非 GUID
    if (FAILED(MFCreateAttributes(&enum_attrs, 1)) ||
        FAILED(enum_attrs->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)) ||
        FAILED(MFEnumDeviceSources(enum_attrs, &activates, &count)))
        return out;
    if (enum_attrs != nullptr) enum_attrs->Release();

    for (UINT32 i = 0; i < count; ++i) {
        LPWSTR name = nullptr, link = nullptr;
        UINT32 name_len = 0, link_len = 0;
        const bool have_name = SUCCEEDED(activates[i]->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &name_len));
        const bool have_link = SUCCEEDED(activates[i]->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
            &link, &link_len));
        if (have_name && have_link) {
            const std::string label = utf8Of(name);
            const std::string ref   = utf8Of(link);
            if (!label.empty() && !ref.empty())
                out.push_back({label, ref}); // ref=SymbolicLink（收端原样重建）
        }
        CoTaskMemFree(name);
        CoTaskMemFree(link);
        activates[i]->Release();
    }
    CoTaskMemFree(activates);
    return out;
}

std::unique_ptr<ILiveIngest> makeCamera(std::string source_name,
                                        int        latency_mode,
                                        ILiveIngest::FrameSink sink) {
    return std::make_unique<CameraIngest>(std::move(source_name),
                                          latency_mode, std::move(sink));
}

} // namespace ms
