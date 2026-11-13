// ============================================================================
//  IniConfig.cpp —— 轻量 INI 配置读取器实现（Editor / Display 两端同源共用）
//
//  解析规则详见 IniConfig.hpp 头注释（节/键值/注释/BOM/trim）。
//  刻意不做的：转义、引号、行内分号续行、include——配置是运维手写的，
//  规则越少越可预期；需要更复杂表达时再扩展。
// ============================================================================

#include "config/IniConfig.hpp"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
// windows.h 的三件套保护与 platform/ConsoleUtf8.hpp 同款
// （LEAN 排 winsock/dlgs 老宏、NOMINMAX 排 min/max）——本 cpp 属于
// engine_core，保护必须独立自洽（防被其他 TU 先行污染）
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h> // readlink（Linux exe 路径）
#endif

namespace ms::config {

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e &&
           std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b &&
           std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

IniConfig IniConfig::load(const std::string& path) {
    IniConfig cfg;

    std::ifstream in(path, std::ios::binary);
    if (!in) return IniConfig{}; // 文件不存在 → 空配置（正常路径）

    std::string current; // 当前节名（节外键归 ""，本加载器不暴露——
                         // 两端约定所有键都写在节内）
    std::string line;
    while (std::getline(in, line)) {
        // 去 BOM（仅首行一次）：UTF-8 BOM = EF BB BF
        static const std::string bom = "\xEF\xBB\xBF";
        if (line.rfind(bom, 0) == 0) line.erase(0, bom.size());
        // 去 \r（Windows CRLF 文本在 binary 模式下残留）
        if (!line.empty() && line.back() == '\r') line.pop_back();

        const std::string trimmed = trim(line);
        if (trimmed.empty()) continue;
        if (trimmed[0] == ';' || trimmed[0] == '#') continue; // 整行注释

        if (trimmed.front() == '[') {
            const auto close = trimmed.find(']');
            if (close != std::string::npos)
                current = trim(trimmed.substr(1, close - 1));
            continue; // 无闭合 ']' 的坏行：忽略（容错优先）
        }

        const auto eq = trimmed.find('=');
        if (eq == std::string::npos) continue; // 无 '=' 的行：忽略
        std::string key = trim(trimmed.substr(0, eq));
        std::string val = trim(trimmed.substr(eq + 1));
        // 行内注释：' ;' / ' #'（分号/井号前有空白才截断——保护含
        // '#' 的路径值；行首注释已在上一步处理）
        for (std::size_t i = 0; i < val.size(); ++i) {
            if ((val[i] == ';' || val[i] == '#') && i > 0 &&
                std::isspace(static_cast<unsigned char>(val[i - 1]))) {
                val = trim(val.substr(0, i));
                break;
            }
        }
        if (key.empty()) continue;
        cfg.sections_[current][key] = val; // static 函数：经实例访问成员
    }
    return cfg;
}

std::string IniConfig::getStr(const std::string& section,
                              const std::string& key,
                              const std::string& fallback) const {
    const auto sec = sections_.find(section);
    if (sec == sections_.end()) return fallback;
    const auto kv = sec->second.find(key);
    if (kv == sec->second.end()) return fallback;
    if (kv->second.empty()) return fallback; // "key =" 留空 = 未设置（语义
                                             // 统一：回落默认，不返回空串）
    return kv->second;
}

long IniConfig::getLong(const std::string& section, const std::string& key,
                        long fallback) const {
    const std::string v = getStr(section, key);
    if (v.empty()) return fallback;
    char* end = nullptr;
    errno = 0;
    const long parsed = std::strtol(v.c_str(), &end, 10);
    if (end == v.c_str()) return fallback; // 非数字开头（解析零位）
    if (errno == ERANGE) return fallback;  // 溢出（返回极值）不可信
    while (end < v.c_str() + v.size() &&
           std::isspace(static_cast<unsigned char>(*end)))
        ++end; // 容许尾随空白（值已 trim，防御性）
    if (end != v.c_str() + v.size()) return fallback; // 尾部垃圾（"17000x"）
    return parsed;
}

std::int64_t IniConfig::getI64(const std::string& section,
                               const std::string& key,
                               std::int64_t fallback) const {
    const std::string v = getStr(section, key);
    if (v.empty()) return fallback;
    char* end = nullptr;
    errno = 0;
    const std::int64_t parsed = std::strtoll(v.c_str(), &end, 10);
    if (end == v.c_str()) return fallback; // 非数字开头（解析零位）
    if (errno == ERANGE) return fallback;  // 溢出（返回极值）不可信
    while (end < v.c_str() + v.size() &&
           std::isspace(static_cast<unsigned char>(*end)))
        ++end; // 容许尾随空白（值已 trim，防御性）
    if (end != v.c_str() + v.size()) return fallback; // 尾部垃圾
    return parsed;
}

bool IniConfig::getBool(const std::string& section, const std::string& key,
                        bool fallback) const {
    std::string v = getStr(section, key);
    if (v.empty()) return fallback;
    // 全体转小写（含数字：'0'/'1' 是合法布尔字面量，不能用 isalpha 过滤）
    for (char& c : v) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (v == "1" || v == "true" || v == "yes" || v == "on") return true;
    if (v == "0" || v == "false" || v == "no" || v == "off") return false;
    return fallback;
}

std::string findConfigFile(const std::vector<std::string>& candidates) {
    for (const std::string& p : candidates) {
        if (p.empty()) continue;
        std::ifstream in(p, std::ios::binary);
        if (in) return p; // 第一个可读的候选
    }
    return {};
}

bool saveKeyToFile(const std::string& path, const std::string& section,
                   const std::string& key, const std::string& value) {
    // ---- 读全文件（不存在 = 空 → 下方按"新建仅含目标节"处理）--------------
    std::string content;
    {
        std::ifstream in(path, std::ios::binary);
        if (in) {
            std::ostringstream ss;
            ss << in.rdbuf();
            content = ss.str();
        }
    }
    // 行尾风格跟随原文件（CRLF 优先判定）；新建文件统一 LF
    const std::string eol =
        content.find("\r\n") != std::string::npos ? "\r\n" : "\n";

    // 逐行拆分（剥离行尾 \r 存纯内容，重组时按 eol 统一还原）
    std::vector<std::string> lines;
    {
        std::size_t start = 0;
        while (start < content.size()) {
            const auto nl = content.find('\n', start);
            std::string l = (nl == std::string::npos)
                                ? content.substr(start)
                                : content.substr(start, nl - start);
            if (!l.empty() && l.back() == '\r') l.pop_back();
            lines.push_back(std::move(l));
            if (nl == std::string::npos) break;
            start = nl + 1;
        }
    }

    // ---- Pass 1：定位节头行 / 键值行 / 节内插入点 ---------------------------
    // （对齐 load 语义：重复键行取最后者生效——替换也取节内最后一处）
    long sec_head = -1, key_line = -1, last_kv = -1;
    std::string cur_sec;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string t = trim(lines[i]);
        if (!t.empty() && t.front() == '[') {
            const auto close = t.find(']');
            cur_sec = (close == std::string::npos)
                          ? std::string{}
                          : trim(t.substr(1, close - 1));
            if (cur_sec == section && sec_head < 0) sec_head = static_cast<long>(i);
            continue;
        }
        if (cur_sec != section || t.empty() ||
            t[0] == ';' || t[0] == '#')
            continue;                      // 节外/空行/注释行
        const auto eq = t.find('=');
        if (eq == std::string::npos) continue;
        if (trim(t.substr(0, eq)) == key)
            key_line = static_cast<long>(i); // 继续扫——最后者为准
        else if (key_line < 0)
            last_kv = static_cast<long>(i);  // 键之前的节内键值行（插入点候选）
    }

    // ---- Pass 2：替换 / 插入 / 追加 ----------------------------------------
    // 键行已解析过行内注释的值可能带行尾注释——load 读值时剥注释，写回
    // 重写整行为 "key = value"（原行内注释随替换丢弃：机器写回的键值行
    // 不承诺保留人工行内注释；整行注释与其它键均不动）
    if (key_line >= 0) {
        lines[static_cast<std::size_t>(key_line)] = key + " = " + value;
    } else if (sec_head >= 0) {
        // 节存在键不存在：插到节内最后一个键值行之后（无键值行则紧跟节头）
        const std::size_t at =
            (last_kv >= 0) ? static_cast<std::size_t>(last_kv) + 1
                           : static_cast<std::size_t>(sec_head) + 1;
        lines.insert(lines.begin() + static_cast<long>(at), key + " = " + value);
    } else {
        // 节不存在：末尾追加（文件已有内容时先空一行分隔）
        if (!lines.empty() && !lines.back().empty()) lines.emplace_back();
        lines.push_back("[" + section + "]");
        lines.push_back(key + " = " + value);
    }

    // ---- 重组写回（二进制：不做任何编码/行尾转换）--------------------------
    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        out += lines[i];
        out += eol;
    }
    std::ofstream out_f(path, std::ios::binary | std::ios::trunc);
    if (!out_f) return false;
    out_f.write(out.data(), static_cast<std::streamsize>(out.size()));
    return out_f.good();
}

std::string exeDirectory() {
#ifdef _WIN32
    char buf[MAX_PATH] = {};
    const UINT n = ::GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return {};
    const std::string full(buf, n);
    const auto pos = full.find_last_of("\\/");
    return pos == std::string::npos ? std::string{} : full.substr(0, pos);
#else
    char buf[4096];
    const ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return {};
    const std::string full(buf, static_cast<std::size_t>(n));
    const auto pos = full.find_last_of('/');
    return pos == std::string::npos ? std::string{} : full.substr(0, pos);
#endif
}

} // namespace ms::config
