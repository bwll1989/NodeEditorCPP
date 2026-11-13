// ============================================================================
//  LiveIngest —— 实时源采集层统一接口 + 注册表（Ingest 层开闭原则落点）
//
//  架构定稿（2026-11-08 用户定案"注册表化重构"）：
//    * Live 是一种独立片段类型（AssetKind::Live）；NDI/Spout/窗口捕获/
//      显示器捕获/网络流只是【接入通道】（LiveIngest 枚举 + 本注册表）；
//    * 加新协议 = 新增一对实现文件（XxxIngest.hpp/.cpp，纯 core 无 UI/
//      无第三方头泄漏）+ 注册表填一行——LiveHub / LiveMediaSource /
//      LiveClipSection 三个接入点全部查表，零分支改动。
//
//  三接入点（全部只依赖本头，不依赖任何具体实现头）：
//    * LiveHub::ensureAsset        —— editor 端收源 → Feed → NDI 再发布；
//    * LiveMediaSource::prepare    —— display/预览端直收（Hub 未命中时）；
//    * LiveClipSection             —— UI 接入方式下拉 + 源列表刷新。
//
//  source_name 语义按接入通道解释（注册表契约，UI 直接把 ref 存入
//  asset.live_source_name）：
//    * Ndi      —— NDI 源名（全名 "MACHINE (name)" 或纯源名，收端自适配）；
//    * Spout    —— Spout sender 名（如 "SpoutCam"）；
//    * Window   —— "标题 [hwnd:十六进制]"（列表项生成；收端解析后缀定位
//                  窗口，窗口已关 = 离线占位 + 发现循环重试）；
//    * Monitor  —— "显示器 N [hmon:十六进制]"（HMONITOR 句柄同理）；
//    * Camera   —— 设备 SymbolicLink（MFEnumDeviceSources 生成，收端按它
//                  重建设备源；label=FriendlyName 仅显示用）；
//    * Network  —— 流 URL（rtsp:// srt:// udp://…，待实现）。
// ============================================================================

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "live/LiveFrame.hpp"
#include "model/SceneModel.hpp" // LiveIngest 枚举（proto LivePayload.ingest 镜像）

namespace ms {

// ---------------------------------------------------------------------------
// ILiveIngest —— 采集器统一接口（NdiIngest/SpoutIngest/WgcIngest 共同遵守）
// ---------------------------------------------------------------------------
class ILiveIngest {
public:
    /// 帧回调（接收线程执行）：新帧上抛；发布方应立即转存 shared_ptr
    using FrameSink = std::function<void(LiveFramePtr)>;

    virtual ~ILiveIngest() = default;

    /// 启动接收线程（发现/连接异步进行；运行时初始化失败返回 false）
    virtual bool start() = 0;
    /// 停线程 + 释放资源（幂等；join 后回调不再触发）
    virtual void stop()  = 0;

    // ---- 诊断 ----
    virtual bool          connected()      const noexcept = 0;
    virtual std::uint64_t framesReceived() const noexcept = 0;
    virtual std::uint64_t framesDropped()  const noexcept = 0;
};

// ---------------------------------------------------------------------------
// 列表项：label = UI 显示名；ref = asset.live_source_name 存储串
// （两者分离——窗口/显示器列表项带定位后缀，显示时不必露出）
// ---------------------------------------------------------------------------
struct IngestSource {
    std::string label; ///< UI 显示（如 "显示器 1"、"记事本"）
    std::string ref;   ///< 连接串（如 "记事本 [hwnd:0001ABCD]"）
};

// ---------------------------------------------------------------------------
// IngestDesc —— 注册表条目（纯数据 + 两个函数指针，无虚函数无状态）
// ---------------------------------------------------------------------------
struct IngestDesc {
    LiveIngest   kind;        ///< proto int32 ingest 的枚举镜像
    const char*  name;        ///< UI 显示名（"NDI 源"/"Spout 纹理"/…）
    bool         implemented; ///< false = UI 置灰 + prepare 拒绝（待实现）
    /// 源枚举（UI「刷新」按钮；同步阻塞 ≤ wait_ms；未实现/无环境返回空表）
    std::vector<IngestSource> (*list_sources)(std::uint32_t wait_ms);
    /// 工厂（未实现条目为 nullptr；运行时初始化失败返回空指针）
    std::unique_ptr<ILiveIngest> (*create)(std::string source_name,
                                           int        latency_mode,
                                           ILiveIngest::FrameSink sink);
};

/// 注册表（有序数组——UI 下拉顺序即此顺序；新增协议在此加一行）
const std::vector<IngestDesc>& ingestRegistry();

/// proto 值查条目（未知值返回 nullptr——防御旧工程文件/越界值）
const IngestDesc* findIngest(std::int32_t proto_value);

} // namespace ms
