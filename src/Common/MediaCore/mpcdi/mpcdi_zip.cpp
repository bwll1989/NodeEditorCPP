#include "mpcdi_parse.hpp"

// ============================================================================
// .mpcdi 包（zip）只读解包 —— 对齐 bake.js loadZip/findInZip。
// zip 引擎 = MPCDI 官方参考实现 ThirdParty 的 minizip + zlib（纯 C，双端可编）。
// Windows 下中文/Unicode 路径：经 iowin32 的宽字符文件函数（fill_win32_filefunc64W），
// 不走 fopen（MSVC fopen 按 ANSI 代码页解释，UTF-8 中文路径会打开失败）。
// 非 Windows（NDK）：stdio 原生 UTF-8，直接用 fill_fopen64_filefunc + 路径透传。
// ============================================================================

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "ioapi.h"
#ifdef _WIN32
#include "iowin32.h"   // 依赖 ioapi.h 先定义 zlib_filefunc64_def
#endif
#include "unzip.h"

#include <cstdio>

namespace ms {

namespace {

#ifdef _WIN32
/** UTF-8 → UTF-16（Windows 路径用）。 */
std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return std::wstring();
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
                                      static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n > 0 ? n : 0), L'\0');
    if (n > 0) {
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                            w.data(), n);
    }
    return w;
}
#endif

/** 包内路径键归一化：反斜杠 → 正斜杠、去前导斜杠（对齐 bake.js 的 norm 预处理）。 */
std::string normalize_pkg_key(const std::string& raw) {
    std::string k = raw;
    for (char& c : k) {
        if (c == '\\') c = '/';
    }
    size_t b = 0;
    while (b < k.size() && k[b] == '/') ++b;
    return k.substr(b);
}

/** 路径基名（最后一个 '/' 之后），ASCII 小写化（findInZip 的兜底匹配用）。 */
std::string lower_basename(const std::string& path) {
    std::string base = path;
    const size_t slash = base.find_last_of('/');
    if (slash != std::string::npos) base = base.substr(slash + 1);
    for (char& c : base) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return base;
}

/** ASCII 小写整串（大小写不敏感匹配用）。 */
std::string ascii_lower(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
}

}  // namespace

// ---------------------------------------------------------------------------
// 容错查找（逐条对齐 bake.js findInZip 的 4 级候选）
// ---------------------------------------------------------------------------
const std::vector<std::uint8_t>* MpcdiPackageData::find(const std::string& rel,
                                                        const std::string& buffer_id) const {
    if (rel.empty()) return nullptr;
    const std::string norm = normalize_pkg_key(rel);

    // 1) 原路径；2) <buffer>/<rel>；3) <buffer>/<基名>
    if (const auto it = files.find(norm); it != files.end()) return &it->second;
    if (!buffer_id.empty()) {
        if (const auto it = files.find(buffer_id + "/" + norm); it != files.end())
            return &it->second;
        if (const auto it = files.find(buffer_id + "/" + lower_basename(norm));
            it != files.end()) return &it->second;
    }

    // 4) 全表大小写不敏感的"路径后缀 / 基名"匹配（v26 兜底行为）
    const std::string base = lower_basename(norm);
    if (base.empty()) return nullptr;
    for (const auto& [name, bytes] : files) {
        const std::string lname = ascii_lower(name);
        if (lname == base || lname.size() > base.size() + 1 &&
                                 lname.compare(lname.size() - base.size() - 1,
                                               base.size() + 1, "/" + base) == 0) {
            return &bytes;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// zip 解包主流程
// ---------------------------------------------------------------------------
bool load_mpcdi_package(const std::string& utf8_path, MpcdiPackageData& out, std::string& err) {
    out.files.clear();

#ifdef _WIN32
    // 宽字符文件函数 + 宽路径（unzOpen2_64 的 path 参数即传给 fill 函数的 zopenfile）
    const std::wstring wpath = utf8_to_wide(utf8_path);
    zlib_filefunc64_def filefunc{};
    fill_win32_filefunc64W(&filefunc);
    unzFile z = unzOpen2_64(wpath.c_str(), &filefunc);
#else
    // NDK stdio 原生 UTF-8：fopen64 文件函数 + UTF-8 路径直接透传
    zlib_filefunc64_def filefunc{};
    fill_fopen64_filefunc(&filefunc);
    unzFile z = unzOpen2_64(utf8_path.c_str(), &filefunc);
#endif
    if (!z) {
        err = "cannot open mpcdi zip: " + utf8_path;
        return false;
    }

    // 逐条目枚举并读入内存（包体积 = 兆级，全量入内存最简且够用）
    unz_global_info64 ginfo{};
    if (unzGetGlobalInfo64(z, &ginfo) != UNZ_OK) {
        err = "unzGetGlobalInfo64 failed";
        unzClose(z);
        return false;
    }

    int rc = unzGoToFirstFile(z);
    while (rc == UNZ_OK) {
        char name_buf[1024] = {};
        unz_file_info64 info{};
        if (unzGetCurrentFileInfo64(z, &info, name_buf, sizeof(name_buf) - 1,
                                  nullptr, 0, nullptr, 0) != UNZ_OK) {
            err = "unzGetCurrentFileInfo failed";
            unzClose(z);
            return false;
        }
        std::string name = normalize_pkg_key(name_buf);
        const bool is_dir = !name.empty() && (name.back() == '/' || name.back() == '\\');

        if (!is_dir && info.uncompressed_size > 0) {
            if (unzOpenCurrentFile(z) != UNZ_OK) {
                err = "unzOpenCurrentFile failed: " + name;
                unzClose(z);
                return false;
            }
            std::vector<std::uint8_t> data(static_cast<size_t>(info.uncompressed_size));
            size_t got = 0;
            while (got < data.size()) {
                const int r = unzReadCurrentFile(z, data.data() + got,
                                                 static_cast<unsigned>(data.size() - got));
                if (r < 0) {   // 负值 = 解压错误（0 = EOF）
                    err = "unzReadCurrentFile failed (corrupt?): " + name;
                    unzCloseCurrentFile(z);
                    unzClose(z);
                    return false;
                }
                if (r == 0) break;
                got += static_cast<size_t>(r);
            }
            unzCloseCurrentFile(z);
            if (got != data.size()) {
                err = "short read in zip entry: " + name;
                unzClose(z);
                return false;
            }
            out.files.emplace(std::move(name), std::move(data));
        }
        rc = unzGoToNextFile(z);
    }
    // UNZ_END_OF_LIST_OF_FILE = 正常遍历结束；其他负值 = 错误
    if (rc != UNZ_END_OF_LIST_OF_FILE) {
        err = "zip enumeration failed";
        unzClose(z);
        return false;
    }
    unzClose(z);

    if (out.files.empty()) {
        err = "mpcdi zip is empty: " + utf8_path;
        return false;
    }
    err.clear();
    return true;
}

}  // namespace ms
