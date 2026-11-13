#pragma once

#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace media_api {

inline std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

inline std::string ok_json(const std::string& data_json) {
    return std::string("{\"ok\":true,\"data\":") + data_json + ",\"error\":null}";
}

inline std::string err_json(const std::string& code, const std::string& message) {
    return std::string("{\"ok\":false,\"data\":null,\"error\":{\"code\":\"") + json_escape(code) +
           "\",\"message\":\"" + json_escape(message) + "\"}}";
}

inline long long now_unix() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

inline std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

inline bool ends_with_ci(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    return to_lower(s.substr(s.size() - suffix.size())) == to_lower(suffix);
}

inline std::string basename_of(const std::string& path) {
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

inline std::string join_path(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (a.back() == '/' || a.back() == '\\') return a + b;
    return a + "/" + b;
}

inline bool extract_string_field(const std::string& body, const std::string& key, std::string& out) {
    const std::string pat = "\"" + key + "\"";
    auto pos = body.find(pat);
    if (pos == std::string::npos) return false;
    pos = body.find(':', pos + pat.size());
    if (pos == std::string::npos) return false;
    pos = body.find('\"', pos + 1);
    if (pos == std::string::npos) return false;
    ++pos;
    std::string value;
    while (pos < body.size()) {
        char c = body[pos++];
        if (c == '\\' && pos < body.size()) {
            char n = body[pos++];
            switch (n) {
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'u':
                    if (pos + 4 <= body.size()) {
                        auto hex = body.substr(pos, 4);
                        char* end = nullptr;
                        long v = std::strtol(hex.c_str(), &end, 16);
                        if (end && *end == '\0') value.push_back(static_cast<char>(v & 0xff));
                        pos += 4;
                    }
                    break;
                default: value.push_back(n); break;
            }
            continue;
        }
        if (c == '\"') break;
        value.push_back(c);
    }
    out = value;
    return true;
}

inline bool extract_bool_field(const std::string& body, const std::string& key, bool& out) {
    const std::string pat = "\"" + key + "\"";
    auto pos = body.find(pat);
    if (pos == std::string::npos) return false;
    pos = body.find(':', pos + pat.size());
    if (pos == std::string::npos) return false;
    ++pos;  // skip ':'
    while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos]))) ++pos;
    if (body.compare(pos, 4, "true") == 0) {
        out = true;
        return true;
    }
    if (body.compare(pos, 5, "false") == 0) {
        out = false;
        return true;
    }
    return false;
}

inline bool extract_number_field(const std::string& body, const std::string& key, double& out) {
    const std::string pat = "\"" + key + "\"";
    auto pos = body.find(pat);
    if (pos == std::string::npos) return false;
    pos = body.find(':', pos + pat.size());
    if (pos == std::string::npos) return false;
    ++pos;
    while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos]))) ++pos;
    try {
        size_t idx = 0;
        out = std::stod(body.substr(pos), &idx);
        return idx > 0;
    } catch (...) {
        return false;
    }
}

/**
 * 从 JSON 文本按 key 提取"平衡"值段（M4 新增，供嵌套对象/数组透传）：
 *   对象/数组 → 花括号/方括号配对整段（跳过字符串字面量内的括号）；
 *   字符串    → 配对引号内的内容（转义原样保留）；
 *   数字/bool → 逗号/右括号前的裸值。
 * 用于 MPCDI enable 请求体的 layout/options 与 session.json 的同名字段——
 * 本工程刻意不引完整 JSON 解析器，平衡提取对自产 JSON 足够可靠。
 */
inline bool extract_json_value(const std::string& text, const std::string& key, std::string& out) {
    const std::string pat = "\"" + key + "\"";
    auto pos = text.find(pat);
    if (pos == std::string::npos) return false;
    auto colon = text.find(':', pos + pat.size());
    if (colon == std::string::npos) return false;
    size_t i = colon + 1;
    while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
    if (i >= text.size()) return false;
    const char open = text[i];
    if (open == '{' || open == '[') {
        const char close = (open == '{') ? '}' : ']';
        int depth = 0;
        bool in_str = false, esc = false;
        size_t j = i;
        for (; j < text.size(); ++j) {
            const char c = text[j];
            if (in_str) {
                if (esc) esc = false;
                else if (c == '\\') esc = true;
                else if (c == '"') in_str = false;
                continue;
            }
            if (c == '"') {
                in_str = true;
                continue;
            }
            if (c == open) ++depth;
            else if (c == close) {
                --depth;
                if (depth == 0) {
                    ++j;
                    break;
                }
            }
        }
        out = text.substr(i, j - i);
        return true;
    }
    if (open == '"') {
        size_t j = i + 1;
        while (j < text.size()) {
            char c = text[j];
            if (c == '\\' && j + 1 < text.size()) {
                j += 2;
                continue;
            }
            if (c == '"') break;
            ++j;
        }
        if (j >= text.size()) return false;
        out = text.substr(i + 1, j - i - 1);
        return true;
    }
    size_t j = i;
    while (j < text.size() && text[j] != ',' && text[j] != '}' && text[j] != ']') ++j;
    std::string t = text.substr(i, j - i);
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.back()))) t.pop_back();
    if (t.empty()) return false;
    out = t;
    return true;
}

inline std::string url_decode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            auto hex = in.substr(i + 1, 2);
            char* end = nullptr;
            long v = std::strtol(hex.c_str(), &end, 16);
            if (end && *end == '\0') {
                out.push_back(static_cast<char>(v));
                i += 2;
                continue;
            }
        } else if (in[i] == '+') {
            out.push_back(' ');
            continue;
        }
        out.push_back(in[i]);
    }
    return out;
}

}  // namespace media_api
