#pragma once

// ============================================================================
// http_util —— HTTP 层共用小工具（响应发送 / 文件读取 / MIME / 请求体）。
//
// 职责单一：被 http_server.cpp（骨架/静态文件）与 http_routes_*.cpp
// （四组业务路由）共同包含；只做纯工具，不含任何路由逻辑。
// ============================================================================

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"

#include <string>

namespace media_api {

/** 发送 JSON 响应（统一 CORS / no-store 头）。 */
void send_json(Poco::Net::HTTPServerResponse& resp, int status, const std::string& body);

/** 发送文本响应（静态文件用，指定 MIME）。 */
void send_text(Poco::Net::HTTPServerResponse& resp, int status, const std::string& mime,
               const std::string& body);

/** 二进制读整个文件（UTF-8 路径）。 */
bool read_file(const std::string& path, std::string& out);

/** 按扩展名猜 MIME（web 静态资源白名单，兜底 octet-stream）。 */
std::string mime_for(const std::string& path);

/** 字节数人性化（"12.3 MB"，库列表展示用）。 */
std::string format_size(long long bytes);

/** 读完整个请求体（Content-Length 小请求；大文件上传走流式，不经此）。 */
std::string read_body(Poco::Net::HTTPServerRequest& req);

}  // namespace media_api
