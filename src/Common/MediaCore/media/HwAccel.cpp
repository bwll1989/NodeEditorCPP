// ============================================================================
//  HwAccel.cpp —— 硬解后端配置解析实现
//
//  刻意保持最小：只做字符串 ↔ 枚举的规范映射（两端同源 display/editor
//  的 config.ini 与 --hwaccel= 共用）。FFmpeg 类型常量的映射在
//  FfmpegMediaSource.cpp 内完成（那里才 include FFmpeg 头）。
// ============================================================================

#include "media/HwAccel.hpp"

#include <cctype>

namespace ms {

namespace {

/// ASCII 大小写折叠（仅比较用；本地化无关——配置键恒为 ASCII）
std::string lowerAscii(const std::string& s) {
    std::string out = s;
    for (char& c : out)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

} // namespace

HwAccelType HwAccelConfig::parseType(const std::string& name) {
    const std::string k = lowerAscii(name);
    if (k == "soft" || k == "off" || k == "none") return HwAccelType::Soft;
    if (k == "auto")                              return HwAccelType::Auto;
    if (k == "d3d11va" || k == "d3d11")           return HwAccelType::D3D11VA;
    if (k == "cuda" || k == "nvdec" || k == "cuvid")
        return HwAccelType::Cuda; // nvdec 为 cuda 后端的产品别名
    if (k == "vaapi")                             return HwAccelType::Vaapi;
    if (k == "mediacodec")                        return HwAccelType::MediaCodec;
    return HwAccelType::Soft; // 未知值：安全回落软解（调用方负责提示）
}

const char* HwAccelConfig::typeName(HwAccelType t) {
    switch (t) {
        case HwAccelType::Soft:       return "soft";
        case HwAccelType::Auto:       return "auto";
        case HwAccelType::D3D11VA:    return "d3d11va";
        case HwAccelType::Cuda:       return "cuda";
        case HwAccelType::Vaapi:      return "vaapi";
        case HwAccelType::MediaCodec: return "mediacodec";
    }
    return "soft";
}

} // namespace ms
