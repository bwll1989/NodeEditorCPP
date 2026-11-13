#include "http_util.hpp"

#include "json_util.hpp"
#include "path_utf8.hpp"

#include "Poco/StreamCopier.h"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace media_api {

void send_json(Poco::Net::HTTPServerResponse& resp, int status, const std::string& body) {
    resp.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(status));
    resp.setContentType("application/json; charset=utf-8");
    resp.set("Access-Control-Allow-Origin", "*");
    resp.set("Cache-Control", "no-store");
    std::ostream& os = resp.send();
    os << body;
}

void send_text(Poco::Net::HTTPServerResponse& resp, int status, const std::string& mime,
               const std::string& body) {
    resp.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(status));
    resp.setContentType(mime);
    resp.set("Cache-Control", "no-store");
    std::ostream& os = resp.send();
    os << body;
}

bool read_file(const std::string& path, std::string& out) {
    std::ifstream ifs(utf8_to_path(path), std::ios::binary);
    if (!ifs) return false;
    std::ostringstream oss;
    oss << ifs.rdbuf();
    out = oss.str();
    return true;
}

std::string mime_for(const std::string& path) {
    auto lower = to_lower(path);
    if (ends_with_ci(lower, ".html")) return "text/html; charset=utf-8";
    if (ends_with_ci(lower, ".js")) return "application/javascript; charset=utf-8";
    if (ends_with_ci(lower, ".css")) return "text/css; charset=utf-8";
    if (ends_with_ci(lower, ".svg")) return "image/svg+xml";
    if (ends_with_ci(lower, ".png")) return "image/png";
    if (ends_with_ci(lower, ".jpg") || ends_with_ci(lower, ".jpeg")) return "image/jpeg";
    if (ends_with_ci(lower, ".ico")) return "image/x-icon";
    return "application/octet-stream";
}

std::string format_size(long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 3) {
        v /= 1024.0;
        ++u;
    }
    char buf[64];
    if (u == 0) {
        std::snprintf(buf, sizeof(buf), "%lld %s", static_cast<long long>(bytes), units[u]);
    } else {
        std::snprintf(buf, sizeof(buf), "%.1f %s", v, units[u]);
    }
    return buf;
}

std::string read_body(Poco::Net::HTTPServerRequest& req) {
    std::stringstream ss;
    Poco::StreamCopier::copyStream(req.stream(), ss);
    return ss.str();
}

}  // namespace media_api
