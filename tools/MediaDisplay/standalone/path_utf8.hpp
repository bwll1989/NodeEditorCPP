#pragma once

#include <filesystem>
#include <string>

namespace media_api {

/** UTF-8 路径字符串 → filesystem::path（Windows 走 UTF-16）。 */
std::filesystem::path utf8_to_path(const std::string& utf8);

/** filesystem::path → UTF-8（供 JSON / libmpv 使用）。 */
std::string path_to_utf8(const std::filesystem::path& path);

/** 本地 ANSI 代码页（ACP）窄字节 → UTF-8。用于将 std::filesystem /
 *  标准库异常消息（本地化 ACP 字节）转码后经 JSON 直出网页，避免
 *  HTTP 500 错误消息乱码（round3 S1 顺带修复）。非 Windows 恒等返回。 */
std::string acp_to_utf8(const std::string& acp);

}  // namespace media_api
