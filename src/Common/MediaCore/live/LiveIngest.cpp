// ============================================================================
//  LiveIngest 实现 —— 注册表定义（Ingest 层开闭原则的"唯一改动点"）
//
//  加新协议三步（2026-11-08 定案）：
//    1) 新增 XxxIngest.hpp/.cpp（实现 ILiveIngest；第三方头零泄漏）；
//    2) 在 kRegistry 填一行条目（显示名/list/create）；
//    3) SceneModel::LiveIngest 枚举加镜像值（proto int32 直通）。
//  LiveHub / LiveMediaSource / LiveClipSection 三接入点零改动。
//
//  平台条件编译（2026-11-21 Android 真库接入）：NDI/Network 全平台可用；
//  窗口/显示器捕获（WGC）、Spout 纹理、摄像头（Media Foundation）为
//  Win32 专属——Android 端 FlowDisplayer 编本 TU 时按 _WIN32 排除。
//
//  自环过滤（NDI 列表）：源名含 ms-live 标记不作为采集源（NDI 分发退役
//  前由 Hub 再发布产生；退役后保留为历史场景防御——LiveClipSection 的
//  同名过滤为第二道防线）。
// ============================================================================

#include "live/LiveIngest.hpp"

#include "live/NdiIngest.hpp"
#ifdef _WIN32
#include "live/SpoutIngest.hpp"
#include "live/WgcIngest.hpp"
#include "live/CameraIngest.hpp"
#endif
#include "live/NetworkIngest.hpp"

namespace ms {

namespace {

/// 再发布流标记（历史：与 NDI 分发退役前的 Hub 发布名前缀
/// "ms-live/<project>/<asset>" 同源；保留作自环防御）
constexpr const char* kRepublishMark = "ms-live";

/// NDI 列表包装：源名 → {label=ref=源名}，滤再发布流（防自环）
std::vector<IngestSource> listNdi(std::uint32_t wait_ms) {
    std::vector<IngestSource> out;
    for (auto& name : NdiIngest::listSources(wait_ms)) {
        // 归一化包含匹配（NDI 把源名中 '/' 等替换为空格——见
        // LiveClipSection::isRepublishName 同款防御）
        std::string norm;
        norm.reserve(name.size());
        for (char c : name) {
            if (std::isalnum(static_cast<unsigned char>(c)))
                norm.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            else if (!norm.empty() && norm.back() != ' ')
                norm.push_back(' ');
        }
        if (norm.find(kRepublishMark) != std::string::npos) continue;
        out.push_back({name, std::move(name)});
    }
    return out;
}

/// NDI 工厂包装（unique_ptr 装箱进 ILiveIngest 接口）
std::unique_ptr<ILiveIngest> makeNdi(std::string source_name, int latency_mode,
                                     ILiveIngest::FrameSink sink) {
    return std::make_unique<NdiIngest>(std::move(source_name), latency_mode,
                                       std::move(sink));
}

#ifdef _WIN32
/// Spout 工厂包装（makeSpoutIngest 签名适配）
std::unique_ptr<ILiveIngest> makeSpout(std::string source_name, int latency_mode,
                                       ILiveIngest::FrameSink sink) {
    return makeSpoutIngest(std::move(source_name), latency_mode,
                           std::move(sink));
}
#endif

/// Network 工厂包装（组播/rtsp/srt URL 直开——2026-11-21 组播转发转正）
std::unique_ptr<ILiveIngest> makeNetwork(std::string source_name, int latency_mode,
                                         ILiveIngest::FrameSink sink) {
    return std::make_unique<NetworkIngest>(std::move(source_name), latency_mode,
                                           std::move(sink));
}

/// 注册表（顺序 = UI 下拉顺序；未实现条目保留占位——UI 置灰可见）。
/// Win32 专属条目（WGC/Spout/MF 摄像头）按 _WIN32 条件编译。
const std::vector<IngestDesc> kRegistry = {
    {LiveIngest::Ndi,     "NDI 源",     true, listNdi,              makeNdi},
#ifdef _WIN32
    {LiveIngest::Window,  "窗口捕获",   true, listWgcWindowSources, makeWgcWindow},
    {LiveIngest::Spout,   "Spout 纹理", true, listSpoutSources,     makeSpout},
    {LiveIngest::Monitor, "显示器捕获", true, listWgcMonitorSources, makeWgcMonitor},
    {LiveIngest::Camera,  "摄像头",    true, listCameraSources,     makeCamera},
#endif
    // 网络流（udp 组播/rtsp/srt）：不进 UI 下拉源选择（组播转发由
    // LiveClipSection 勾选自动生成 URL，display 端按 live_url 分派）——
    // list_sources 恒空（无"发现"语义）
    {LiveIngest::Network, "网络流",     true, nullptr,              makeNetwork},
};

} // namespace

// ---------------------------------------------------------------------------
// 注册表与查找
// ---------------------------------------------------------------------------
const std::vector<IngestDesc>& ingestRegistry() { return kRegistry; }

const IngestDesc* findIngest(std::int32_t proto_value) {
    for (const auto& d : kRegistry)
        if (d.kind == static_cast<LiveIngest>(proto_value)) return &d;
    return nullptr; // 未知 proto 值（旧工程/越界）——调用方按未实现处理
}

} // namespace ms
