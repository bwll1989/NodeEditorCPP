#include "path_utf8.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace media_api {

std::filesystem::path utf8_to_path(const std::string& utf8) {
#ifdef _WIN32
    if (utf8.empty()) return {};
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return std::filesystem::path(utf8);
    std::wstring w(static_cast<size_t>(wlen - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, w.data(), wlen);
    return std::filesystem::path(w);
#else
    return std::filesystem::u8path(utf8);
#endif
}

std::string path_to_utf8(const std::filesystem::path& path) {
#ifdef _WIN32
    const std::wstring w = path.wstring();
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), len, nullptr, nullptr);
    return out;
#else
    auto u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
#endif
}

std::string acp_to_utf8(const std::string& acp) {
#ifdef _WIN32
    if (acp.empty()) return {};
    // ACP → UTF-16
    int wlen = MultiByteToWideChar(CP_ACP, 0, acp.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return acp;  // 转码失败时原样返回（兜底不丢消息）
    std::wstring w(static_cast<size_t>(wlen - 1), L'\0');
    MultiByteToWideChar(CP_ACP, 0, acp.c_str(), -1, w.data(), wlen);
    // UTF-16 → UTF-8
    int ulen = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (ulen <= 0) return acp;
    std::string out(static_cast<size_t>(ulen - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), ulen, nullptr, nullptr);
    return out;
#else
    return acp;
#endif
}

}  // namespace media_api
