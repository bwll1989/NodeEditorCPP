// ============================================================================
//  log_hub —— 实现（fd 1/2 管道重定向 → 读线程拆行 → 环形缓冲）。
//  Windows：CRT _pipe + _dup2 + 文件日志（exe/logs/display.log）+ --console tee；
//  POSIX（Android）：pipe + dup2 同构（文件日志不做——logcat 天然兜底）。
//  设计见 log_hub.hpp 文件头。
// ============================================================================

#include "log_hub.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32

#include "path_utf8.hpp"
#include "config/IniConfig.hpp"   // ms::config::exeDirectory（文件日志落点）

#include <windows.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <fcntl.h>
#include <io.h>

#else  // POSIX（Android NDK 等）

#include <unistd.h>
// CRT 风格调用名 → POSIX 等价：公共代码（readerLoop/dup2/close）单一实现，
// 不在函数体里写平台分支。
#define _dup2  dup2
#define _close close
#define _read  read

#ifdef __ANDROID__
#include <android/log.h>
#define LOGHUB_TAG "FlowDisplay"   // 与旧 installLogcatRedirect 同标签（logcat -s FlowDisplay 兼容）
#endif

#endif  // _WIN32

namespace media_api::log_hub {
namespace {

// ---------------------------------------------------------------------------
// 环形缓冲容量：渲染线程 audio lvl 1s 心跳 ≈ 1 行/s，4000 行 ≈ 1 小时
// 历史；超容量 FIFO 丢弃（dropped 计数补偿，网页可提示）。
// 单行上限 4000 字节（超长截断标注——防三方库突发长行吃穿缓冲）。
// ---------------------------------------------------------------------------
constexpr std::size_t kRingCapacity = 4000;
constexpr std::size_t kMaxLineBytes = 4000;

struct State {
    std::mutex mtx;
    std::deque<Line> ring;             // FIFO（seq 升序）
    std::uint64_t last_seq = 0;        // 当前最大 seq
    std::uint64_t dropped = 0;         // 环满丢弃累计
#ifdef _WIN32
    FILE* file = nullptr;              // exe/logs/display.log（append；可空）
    HANDLE con_out = nullptr;          // tee 目标（--console attach 成功时）
#endif
};

State& state() {
    static State s;
    return s;
}

std::int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

/// 合法 UTF-8 字节序列校验（true=原样放行；false=按 ACP 转码）。
/// 只做结构校验（续字节 10xxxxxx 与最长 4 字节）——display 全链 /utf-8
/// 编译，fprintf 字面量恒 UTF-8；非 UTF-8 只可能来自本地化异常消息
/// （仅 Windows 语义；POSIX 侧日志恒 UTF-8，此校验恒真通过）。
bool validUtf8(const char* p, std::size_t n) {
    std::size_t i = 0;
    while (i < n) {
        const unsigned char c = static_cast<unsigned char>(p[i]);
        std::size_t cont = 0;                       // 期望的续字节数
        if (c < 0x80) { ++i; continue; }            // ASCII
        else if ((c & 0xE0) == 0xC0) cont = 1;      // 2 字节
        else if ((c & 0xF0) == 0xE0) cont = 2;      // 3 字节
        else if ((c & 0xF8) == 0xF0) cont = 3;      // 4 字节
        else return false;                          // 0xF8+ / 0x80-0xBF 非法首字节
        if (i + cont >= n) return false;            // 序列被行尾截断
        for (std::size_t k = 1; k <= cont; ++k) {
            if ((static_cast<unsigned char>(p[i + k]) & 0xC0) != 0x80) return false;
        }
        i += cont + 1;
    }
    return true;
}

/// 级别粗判（ASCII 小写化 + 中英双词子串匹配——display 日志两种都多）。
const char* classifyLevel(const std::string& text) {
    std::string low;
    low.reserve(text.size());
    for (char c : text) {
        const unsigned char u = static_cast<unsigned char>(c);
        low.push_back(static_cast<char>((u >= 'A' && u <= 'Z') ? u + 32 : u));
    }
    if (low.find("error") != std::string::npos ||
        low.find("fatal") != std::string::npos ||
        low.find("failed") != std::string::npos ||
        low.find("错误") != std::string::npos ||
        low.find("失败") != std::string::npos)
        return "error";
    if (low.find("warn") != std::string::npos ||
        low.find("警告") != std::string::npos)
        return "warn";
    return "info";
}

/// 一行入枢纽（锁内：seq 分配 + 环形 + 文件；锁外：tee 慢操作）。
void pushLine(std::string text) {
    // 去行尾 \r（_O_BINARY 下 CRLF 原样到达）；空行也记录（三方库的分隔空行）
    while (!text.empty() && (text.back() == '\r' || text.back() == '\n'))
        text.pop_back();
    bool truncated = false;
    if (text.size() > kMaxLineBytes) {
        text.resize(kMaxLineBytes);
        truncated = true;
    }
#ifdef _WIN32
    // 编码修正：非合法 UTF-8 → 按 ACP 转码（std::filesystem 异常等本地化
    // 消息；acp_to_utf8 见 path_utf8.cpp——网页 JSON 直出防乱码）
    if (!validUtf8(text.data(), text.size()))
        text = media_api::acp_to_utf8(text);
#endif
    if (truncated) text += "…(截断)";

    auto& st = state();
    Line line;
    line.ts_ms = nowMs();
    line.text = std::move(text);
    line.level = classifyLevel(line.text);       // 小写化+子串匹配（锁外，快路径）
    {
        std::lock_guard<std::mutex> lk(st.mtx);
        line.seq = ++st.last_seq;
        st.ring.push_back(line);
        if (st.ring.size() > kRingCapacity) {
            st.ring.pop_front();
            ++st.dropped;
        }
#ifdef _WIN32
        if (st.file) {
            std::fwrite(line.text.data(), 1, line.text.size(), st.file);
            std::fputc('\n', st.file);
            std::fflush(st.file);                // 崩溃前的行也保得住
        }
#endif
    }
#ifdef _WIN32
    // tee 在锁外（WriteConsoleW 慢操作不拖 HTTP 消费者）
    if (st.con_out != nullptr && !line.text.empty()) {
        const int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                             line.text.data(),
                                             static_cast<int>(line.text.size()),
                                             nullptr, 0);
        if (wlen > 0) {
            std::wstring w(static_cast<std::size_t>(wlen), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, line.text.data(),
                                static_cast<int>(line.text.size()), &w[0], wlen);
            w += L"\r\n";
            DWORD written = 0;
            if (!WriteConsoleW(st.con_out, w.data(),
                               static_cast<DWORD>(w.size()), &written, nullptr)) {
                // 控制台已关（用户 Ctrl+点击关闭窗口）——停用 tee 防空转
                st.con_out = nullptr;
            }
        }
    }
#endif
#ifdef __ANDROID__
    // logcat 镜像（对位 Windows 的 --console tee；2026-11-24 自
    // flow_display_main.cpp 的 installLogcatRedirect 收编——fd 1/2 只允许
    // 一次重定向，单一读线程双消费者：环形缓冲（网页日志窗口）+ logcat）
    if (!line.text.empty()) {
        const int prio = std::strcmp(line.level, "error") == 0
                             ? ANDROID_LOG_ERROR
                         : std::strcmp(line.level, "warn") == 0
                             ? ANDROID_LOG_WARN
                             : ANDROID_LOG_INFO;
        __android_log_print(prio, LOGHUB_TAG, "%s", line.text.c_str());
    }
#endif
}

/// 读线程：阻塞消费管道 → 拆行 → pushLine。
/// 残包缓冲：一行可能跨多次 read 到达（64KB 管道一次写不下时），残留
/// 拼接直到 \n；行间无界残留（写者只写不 flush 换行）无过期策略——
/// display 全链输出均带换行，理论不存在。
void readerLoop(int read_fd) {
    std::string pending;
    char buf[8192];
    for (;;) {
        // Windows _read 返回 int；POSIX read 返回 ssize_t（buf 8192 上限，
        // 值域无损收窄）
        const int n = static_cast<int>(::_read(read_fd, buf, sizeof(buf)));
        if (n <= 0) return;                      // 写端全关/错误：线程终结
        pending.append(buf, static_cast<std::size_t>(n));
        std::size_t start = 0;
        for (;;) {
            const auto pos = pending.find('\n', start);
            if (pos == std::string::npos) break;
            pushLine(pending.substr(start, pos - start));
            start = pos + 1;
        }
        pending.erase(0, start);
        if (pending.size() > kMaxLineBytes) {    // 无换行洪流：按上限强制成行
            pushLine(pending);
            pending.clear();
        }
    }
}

#ifdef _WIN32
/// exe/logs/ 目录（失败返回空——文件日志静默放弃，不影响主链）。
std::string logsDir() {
    const std::string exe = ms::config::exeDirectory();
    if (exe.empty()) return {};
    return exe + "/logs";
}
#endif

}  // namespace

// ---------------------------------------------------------------------------
// install：重定向 fd 1/2 → 管道；起读线程；（仅 Windows）文件日志 + tee。
// ---------------------------------------------------------------------------

void install(bool tee_console) {
    static std::atomic<bool> installed{false};
    if (installed.exchange(true)) return;

    auto& st = state();

#ifdef _WIN32
    // ---- --console：挂父控制台（双击启动无父控制台 → attach 失败静默退化）
    if (tee_console) {
        if (::AttachConsole(ATTACH_PARENT_PROCESS)) {
            // QuickEdit 禁用（tee 写线程同受选择态挂起威胁——既有铁律）。
            // 不用 GetStdHandle：WINDOWS 子系统 + attach 后 CRT 缓存的
            // stdio 句柄不更新，CONIN$ 直开拿新控制台输入。
            HANDLE cin = ::CreateFileW(L"CONIN$",
                                       GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       nullptr, OPEN_EXISTING, 0, nullptr);
            if (cin != INVALID_HANDLE_VALUE) {
                DWORD mode = 0;
                if (::GetConsoleMode(cin, &mode))
                    ::SetConsoleMode(cin, (mode & ~ENABLE_QUICK_EDIT_MODE) |
                                              ENABLE_EXTENDED_FLAGS);
                ::CloseHandle(cin);
            }
            st.con_out = ::CreateFileW(L"CONOUT$", GENERIC_WRITE,
                                       FILE_SHARE_WRITE, nullptr,
                                       OPEN_EXISTING, 0, nullptr);
            if (st.con_out == INVALID_HANDLE_VALUE) st.con_out = nullptr;
        }
    }

    // ---- 文件日志兜底（exe/logs/display.log；本次启动截断重写）----------
    const std::string dir = logsDir();
    if (!dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(
            media_api::utf8_to_path(dir), ec);
        if (!ec) {
            // _wfopen（UTF-16 路径——u8Path 铁律：窄串 fopen 按 ACP 解码
            // 中文路径会失败）
            const std::wstring wpath =
                media_api::utf8_to_path(dir + "/display.log").wstring();
            st.file = ::_wfopen(wpath.c_str(), L"wb");
        }
    }
#else
    (void)tee_console;   // POSIX 无控制台 tee（Android logcat 天然兜底）
#endif

    // ---- 管道重定向 ------------------------------------------------------
    int pipe_fd[2] = {-1, -1};
#ifdef _WIN32
    if (::_pipe(pipe_fd, 128 * 1024, _O_BINARY) != 0) return;  // 理论不发生
#else
    if (::pipe(pipe_fd) != 0) return;                          // 理论不发生
#endif
    const int read_fd = pipe_fd[0];
    const int write_fd = pipe_fd[1];
#ifdef _WIN32
    // [UCRT GUI 陷阱，2026-11-24 探针实锤] WINDOWS 子系统无控制台启动
    // （双击/Start-Process）时 GetStdHandle(OUT/ERR)=NULL，CRT 的
    // stdout/stderr 流处于「未关联占位」态（_fileno() 恒 -2）——直接
    // _dup2 覆盖 fd 是**假成功**（返回 0 但流层仍 -2），后续 fprintf
    // 全部静默返回 -1 → 无黑窗版双击启动零日志的根因。三层修复：
    //   ① close 伪占位 fd；② freopen("NUL") 重建流（fileno 归位 1/2）；
    //   ③ _dup2 管道覆盖 fd（dup2 语义自动替换关闭 NUL 句柄）。
    // 有控制台场景（--console/终端启动）同样成立：控制台句柄被覆盖为管道。
    // POSIX 无此坑（fd 1/2 恒有效），不加。
    ::_close(1);
    ::_close(2);
    ::freopen("NUL", "w", stdout);
    ::freopen("NUL", "w", stderr);
#endif
    // 覆盖 stdout/stderr
    ::_dup2(write_fd, 1);
    ::_dup2(write_fd, 2);
    ::_close(write_fd);
    // 管道写端默认全缓冲——日志全延迟到 4KB/退出才到读线程， unacceptable；
    // 双双改无缓冲：每条 fprintf 立即到达读线程（日志场景系统调用开销无感）
    ::setvbuf(stdout, nullptr, _IONBF, 0);
    ::setvbuf(stderr, nullptr, _IONBF, 0);

    std::thread(readerLoop, read_fd).detach();
}

void snapshot(std::uint64_t since, std::vector<Line>& out,
              std::uint64_t& last_seq, std::uint64_t& dropped) {
    auto& st = state();
    out.clear();
    std::lock_guard<std::mutex> lk(st.mtx);
    for (const auto& l : st.ring) {
        if (l.seq > since) out.push_back(l);     // deque 有序，可二分——量小线性即可
    }
    last_seq = st.last_seq;
    dropped = st.dropped;
}

std::uint64_t lastSeq() {
    auto& st = state();
    std::lock_guard<std::mutex> lk(st.mtx);
    return st.last_seq;
}

}  // namespace media_api::log_hub
