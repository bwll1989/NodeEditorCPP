// ============================================================================
//  AssetPath.hpp —— 素材本地路径工具（归集语义；2026-11-13 素材归集改造）
//
//  背景：editor 收编后 Asset.path 以绝对路径直通（GUI 素材寻址约定），
//  display 端此前"绝对路径直读/下载保存回原绝对路径"——display 的
//  assets_root 对绝对路径素材完全无效（用户配置 assets_root 期望素材
//  归集到 display 目录，实测 assets/ 永远为空）。
//
//  归集语义（2026-11-13 用户拍板）：
//    * 下载保存（assetStorePath）：绝对路径素材落 assets_root/文件名
//      （不再写回原绝对路径）；相对路径素材落 assets_root/相对路径
//      （保留子目录结构，.show/standalone 语义不变）。
//    * 读取寻址（assetResolvePath）：归集副本存在 → 用副本；否则旧语义
//      （绝对直通/相对拼根）。同机原文件在时零下载零复制（读原路径），
//      文件缺失才拉取归集——display 目录自此可自含分发。
//
//  调用点（四处寻址同语义收敛，原各处手写盘符判定全部退役）：
//    * PlayerRuntime 拉取保存 / 就绪 stat
//    * FfmpegMediaSource::prepare（视频/图片渲染装配）
//    * AudioMediaSource::prepare（音频装配）
// ============================================================================

#pragma once

#include <filesystem>
#include <string>

namespace ms {

/// 绝对路径判定（Windows 盘符 "X:..." / POSIX 根 "/..." / UNC "\\..."）
inline bool isAbsoluteAssetPath(const std::string& path) {
    if (path.size() >= 2 && path[1] == ':') return true; // Windows 盘符
    return !path.empty() && (path[0] == '/' || path[0] == '\\');
}

/// UTF-8 窄串 → filesystem::path（C++20 char8_t 语义构造，按 UTF-8 解码）。
/// 【为什么必须存在】protobuf/JSON 协议里的路径恒为 UTF-8 字节；MSVC
/// std::filesystem 窄字符构造按 ACP（中文系统 GBK）隐式解码——中文文件名
/// 轻则逐字节误解，重则整个转换**抛 std::system_error**（非法 GBK 序列，
/// 如 UTF-8 字节流中 "起" 的尾字节 B7 后跟句点 0x2E）。异常从
/// error_code 重载也拦不住（path 构造发生在调用点，先于 error_code 生效）；
/// 拉取 worker 线程内逃逸即 std::terminate → 全进程静默死亡（2026-11-13
/// display「同步媒体」崩溃实证，探针 Temp/flow_osc_test/fs_path_probe.cpp）。
/// 全链窄串路径进 filesystem API / ifstream / ofstream 前必须经此转换
///（AssetHttpServer.cpp 的 u8path 同源先例，伺服端 2026-11-12 已修）。
inline std::filesystem::path u8Path(const std::string& path) {
    return std::filesystem::path(
        reinterpret_cast<const char8_t*>(path.c_str()));
}

/// 归集落点（下载保存路径）：绝对路径 → assets_root/文件名（拍平目录——
/// 跨机原路径无意义，按文件名归集）；相对路径 → assets_root/path（原样
/// 保留子目录）。assets_root 为空 → 原路径原样（未启用归集，测试场景）。
inline std::string assetStorePath(const std::string& assets_root,
                                  const std::string& path) {
    if (assets_root.empty()) return path;
    std::string root = assets_root;
    if (root.back() != '/' && root.back() != '\\') root += '/';
    if (isAbsoluteAssetPath(path)) {
        const auto pos = path.find_last_of("/\\");
        return root + (pos == std::string::npos
                           ? path
                           : path.substr(pos + 1));
    }
    return root + path;
}

/// 读取寻址（归集副本优先）：绝对路径素材先探归集副本（assets_root/文件名），
/// 命中即用副本（display 目录自含）；未命中回退原绝对路径（同机常态——
/// 原文件在 → 零下载直读）。相对路径/未启用归集 = 旧语义原样。
/// 【编码】protobuf/JSON 路径恒为 UTF-8：以 char8_t 语义构造 path
///（绕开 MSVC 窄字符默认 ACP 解码坑，FfmpegMediaSource.cpp:291 先例）。
inline std::string assetResolvePath(const std::string& assets_root,
                                    const std::string& path) {
    if (isAbsoluteAssetPath(path)) {
        if (assets_root.empty()) return path;
        const std::string store = assetStorePath(assets_root, path);
        std::error_code ec;
        const bool have = std::filesystem::exists(
            std::filesystem::path(
                reinterpret_cast<const char8_t*>(store.c_str())),
            ec);
        return (have && !ec) ? store : path;
    }
    if (assets_root.empty()) return path;
    std::string root = assets_root;
    if (root.back() != '/' && root.back() != '\\') root += '/';
    return root + path;
}

} // namespace ms
