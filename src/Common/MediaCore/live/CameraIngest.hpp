// ============================================================================
//  CameraIngest —— 本机摄像头/采集卡设备采集（Media Foundation Source Reader；
//  注册表 Camera 条目实现）
//
//  技术路线（2026-11-09 定案）：
//    * MFEnumDeviceSources(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)
//      枚举本机视频捕获设备——label=FriendlyName（UI 显示）、
//      ref=SymbolicLink（连接串，收端按它重建设备源）；
//    * IMFSourceReader 同步 ReadSample 轮询（阻塞至下一帧，自带设备帧率
//      节奏 ~30fps，无需额外 sleep）→ 强制 RGB32 输出（MF 自动插颜色
//      转换器）→ BGRA swizzle RGBA → LiveFrame（行主序 stride=w*4）；
//    * 设备丢失/拔出（ReadSample 失败或流结束）→ 关闭 reader → 500ms
//      节流重试重新打开（同 WgcIngest 自愈语义）；热切换分辨率
//      （MEDIATYPECHANGED 事件）→ 同样重开重配；
//    * RGB32 的 alpha 字节未定义（多数设备为 0）——统一置 0xFF，保证与
//      NDI/WGC 帧同契约（不透明）。
//
//  线程纪律：继承 IngestBase 兜底壳（漏接异常不得击穿 std::thread）；
//  CoInitializeEx(MTA) + MFStartup 在接收线程内完成，退出时 RAII 逆序清理。
//
//  依赖：mfplat（MF 运行时）/ mf（设备枚举）/ mfuuid（媒体子类型 GUID）+
//  ole32（COM 初始化）——纯系统 API，无第三方库。
// ============================================================================

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "live/IngestBase.hpp"
namespace ms {

class CameraIngest : public IngestBase {
public:
    /// source_ref = 设备 SymbolicLink（listCameraSources 生成，原样存
    /// asset.live_source_name；收端 UTF-8 → wide 后重建设备源）
    CameraIngest(std::string source_ref, int latency_mode, FrameSink sink);
    ~CameraIngest() override;

    CameraIngest(const CameraIngest&)            = delete;
    CameraIngest& operator=(const CameraIngest&) = delete;

private:
    void        recvLoop() override;
    const char* logTag() const noexcept override { return "camera"; }
};

/// 摄像头/采集卡设备枚举（注册表 list 包装；即时返回，wait_ms 兼容签名；
/// 无设备返回空表）
std::vector<IngestSource> listCameraSources(std::uint32_t wait_ms);

/// 工厂（注册表 create 包装）
std::unique_ptr<ILiveIngest> makeCamera(std::string source_name,
                                        int        latency_mode,
                                        ILiveIngest::FrameSink sink);

} // namespace ms
