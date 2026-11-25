// ============================================================================
//  CrashDump.cpp —— Windows 崩溃现场捕获实现（见头文件说明）
//
//  设计要点：
//    * MiniDumpWriteDump 进程内动态加载（dbghelp.dll 随系统必有，静态
//      链接 dbghelp.lib 亦可，但动态加载让本 TU 在无调试环境时零负担）；
//    * dump 类型 MiniDumpNormal（线程栈 + 异常上下文 + 模块表）：定位
//      "哪一帧崩了"足够；带数据段体积暴涨反而拖慢崩溃瞬间；
//    * 过滤器返回 EXCEPTION_CONTINUE_SEARCH——写完 dump 后系统 WER
//      对话框照常出现（用户看到"程序已停止工作"属预期）；
//    * 过滤器内只用 Win32 API（CRT/C++ 运行时可能已损坏）+ stderr 单次
//      打印（fprintf 本身不再分配大块内存，可接受）。
// ============================================================================
#include "CrashDump.hpp"

#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>

#include <exception>
#include <cstdlib>

namespace {

LONG WINAPI crashDumpFilter(EXCEPTION_POINTERS* pep) {
    // MiniDumpWriteDump 动态定位：安装时已预加载 dbghelp.dll（见
    // installCrashDump），此处 GetModuleHandle 零加载零分配——崩溃若发生
    // 在 loader lock 内（DllMain 链上），LoadLibrary 会死锁丢 dump；预载
    // 失败的极端情形兜底再试一次 LoadLibrary。
    using MiniDumpWriteDump_t = BOOL(WINAPI*)(
        HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
        CONST PMINIDUMP_EXCEPTION_INFORMATION,
        CONST PMINIDUMP_USER_STREAM_INFORMATION,
        CONST PMINIDUMP_CALLBACK_INFORMATION);
    HMODULE dbg = ::GetModuleHandleW(L"dbghelp.dll");
    if (!dbg) dbg = LoadLibraryW(L"dbghelp.dll");
    if (!dbg) return EXCEPTION_CONTINUE_SEARCH;
    auto dump = reinterpret_cast<MiniDumpWriteDump_t>(
        reinterpret_cast<void*>(GetProcAddress(dbg, "MiniDumpWriteDump")));
    if (!dump) return EXCEPTION_CONTINUE_SEARCH;

    // 文件名：crash\crash-YYYYMMDD-HHMMSS.dmp（exe 旁 crash 子目录，
    // 2026-11-16 由 exe 平铺改为归入子目录——崩溃文件与 exe 目录其他
    // 产物隔离，清理/收集不误伤）。子目录创建失败（路径被同名文件占用/
    // 权限）→ 预检目录属性，失败退回 exe 同目录平铺兜底——崩溃瞬间
    // 宁可留证不追求归置。
    wchar_t exe_path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    std::wstring dir(exe_path);
    const std::size_t slash = dir.find_last_of(L"\\/");
    std::wstring dirpath =
        (slash == std::wstring::npos) ? L"." : dir.substr(0, slash);
    const std::size_t exeDirLen = dirpath.size();
    dirpath += L"\\crash";
    CreateDirectoryW(dirpath.c_str(), nullptr); // 已存在（ERROR_ALREADY_EXISTS）视同成功
    if (GetFileAttributesW(dirpath.c_str()) == INVALID_FILE_ATTRIBUTES)
        dirpath.resize(exeDirLen); // 目录没建成 → 平铺回退

    SYSTEMTIME st = {};
    GetLocalTime(&st);
    wchar_t path[MAX_PATH] = {};
    swprintf(path, MAX_PATH,
             L"%s\\crash-%04u%02u%02u-%02u%02u%02u.dmp",
             dirpath.c_str(), st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond);

    HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return EXCEPTION_CONTINUE_SEARCH;

    MINIDUMP_EXCEPTION_INFORMATION mei = {};
    mei.ThreadId          = GetCurrentThreadId();
    mei.ExceptionPointers = pep;
    mei.ClientPointers    = FALSE;
    const BOOL ok = dump(GetCurrentProcess(), GetCurrentProcessId(), f,
                         MiniDumpNormal, pep ? &mei : nullptr,
                         nullptr, nullptr);
    CloseHandle(f);

    // stderr 留痕（GUI 从终端启动可见；无终端时 dump 文件本身就是证据）
    std::fprintf(stderr,
                 "\n[crash] 未处理异常！现场已写入 %ls（%s）\n"
                 "        请把该 .dmp 文件发给开发者定位崩溃原因。\n",
                 path, ok ? "成功" : "写入失败");
    std::fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH; // 交还系统（WER 对话框照常出现）
}

} // namespace

void installCrashDump() {
    // 【只注册一次】头文件契约：重复调用安全——SetUnhandledExceptionFilter
    // 语义是"覆盖"，加守卫保证首次安装不被后调覆盖（2026-11-06 补）。
    static const bool installed = [] {
        // 【预加载 dbghelp 2026-11-16】崩溃过滤器内只做 GetModuleHandle
        // ——崩溃发生在 loader lock 内时 LoadLibrary 死锁、堆损坏时加载
        // 易失败；正常期预载（不释放，进程存活期持有）把风险移出崩溃路径。
        LoadLibraryW(L"dbghelp.dll");
        SetUnhandledExceptionFilter(&crashDumpFilter);
        // 【terminate 也留证 2026-11-11】未捕获 C++ 异常/bad_alloc 等走
        // std::terminate——SEH 过滤器抓不到（症状：无 dmp、无 WER 对话框
        // 的瞬时闪退）。主动生成同款 dump 后 abort。
        std::set_terminate([] {
            crashDumpFilter(nullptr);
            std::abort();
        });
        return true;
    }();
    (void)installed;
}

void setCrashDumpDir(const char*) {} // Windows dump 落 exe 旁；目录注入不用

#elif defined(__ANDROID__)
// ============================================================================
//  Android 信号捕获（2026-11-24 Pura X 真机联调）
//
//  背景：卓易通/鸿蒙容器内无法用宿主 hdc 读容器 logcat/tombstone——崩溃
//  证据必须"自带降落伞"：
//    * handler 写 <dir>/web/last_crash.txt（display 自身 HTTP 静态根，
//      浏览器 http://<IP>:8995/last_crash.txt 直接可读）与
//      <dir>/last_crash.txt（保底）；
//    * 内容：信号号 / fault 地址 / 线程 id / pc 帧地址数组——符号化离线
//      做（addr2line 对照带符号 .so，帧地址与返回地址差一个偏移可接受，
//      数量级定位足够）；
//    * async-signal-safe 收敛：open/write/close + _Unwind_Backtrace
//      （纯栈遍历无堆分配）+ 手工 hex/dec 格式化（不碰 snprintf/堆）；
//    * 收尾恢复默认 handler 并重发信号——进程按系统语义终止（容器侧
//      崩溃记录照常），不吞系统崩溃链。
// ============================================================================
#include <csignal>
#include <cstring>
#include <ctime>
#include <exception>     // std::set_terminate / current_exception
#include <unistd.h>
#include <fcntl.h>
#include <unwind.h>      // _Unwind_Backtrace：<execinfo.h> backtrace 为
                         // API 33+（NDK 对 API 26 目标不声明）——libunwind
                         // 的此入口全 API 级可用且 signal-safe
#include <android/log.h>

namespace {

char g_dir[512] = {0}; // filesDir（启动期 setCrashDumpDir 注入）
volatile sig_atomic_t g_in_handler = 0;

// _Unwind_Backtrace 回调：收集 PC 帧（coplevel≤64）
struct BtCtx {
    void** frames;
    int    max;
    int    n;
};

_Unwind_Reason_Code btCallback(struct _Unwind_Context* ctx, void* arg) {
    auto* c = static_cast<BtCtx*>(arg);
    if (c->n >= c->max) return _URC_END_OF_STACK;
    if (const uintptr_t ip = _Unwind_GetIP(ctx); ip != 0)
        c->frames[c->n++] = reinterpret_cast<void*>(ip);
    return _URC_NO_REASON;
}

void writeAll(int fd, const char* p, size_t n) {
    while (n > 0) {
        const ssize_t w = ::write(fd, p, n);
        if (w <= 0) return;
        p += w;
        n -= static_cast<std::size_t>(w);
    }
}

// 手工无符号转 hex（崩溃路径不碰 snprintf/CRT 数字格式化）
std::size_t u2hex(unsigned long v, char* out) {
    static const char kDig[] = "0123456789abcdef";
    char tmp[24];
    int n = 0;
    do { tmp[n++] = kDig[v & 0xF]; v >>= 4; } while (v);
    for (int i = 0; i < n; ++i) out[i] = tmp[n - 1 - i];
    out[n] = '\0';
    return static_cast<std::size_t>(n);
}

std::size_t u2dec(unsigned long v, char* out) {
    char tmp[24];
    int n = 0;
    do { tmp[n++] = static_cast<char>('0' + v % 10); v /= 10; } while (v);
    for (int i = 0; i < n; ++i) out[i] = tmp[n - 1 - i];
    out[n] = '\0';
    return static_cast<std::size_t>(n);
}

void crashHandler(int sig, siginfo_t* info, void* /*uctx*/) {
    // 重入防御：handler 内再崩（堆/栈已坏）直接交默认终止
    if (g_in_handler) {
        ::signal(sig, SIG_DFL);
        ::raise(sig);
        return;
    }
    g_in_handler = 1;

    void* frames[64];
    BtCtx bt{frames, 64, 0};
    _Unwind_Backtrace(&btCallback, &bt);
    const int nfr = bt.n;

    char buf[4096];
    std::size_t off = 0;
    auto put = [&](const char* s) {
        while (*s && off < sizeof(buf) - 1) buf[off++] = *s++;
    };
    auto puthex = [&](unsigned long v) {
        char h[26];
        h[0] = '0';
        h[1] = 'x';
        u2hex(v, h + 2);
        put(h);
    };
    auto putdec = [&](unsigned long v) {
        char d[26];
        u2dec(v, d);
        put(d);
    };

    put("[FlowDisplay crash] signal=");
    putdec(static_cast<unsigned long>(sig));
    put(" fault_addr=");
    puthex(info ? reinterpret_cast<unsigned long>(info->si_addr) : 0UL);
    put(" pid=");
    putdec(static_cast<unsigned long>(::getpid()));
    put(" time=");
    putdec(static_cast<unsigned long>(::time(nullptr)));
    put("\nbacktrace(pc 帧地址; 用带符号 .so + addr2line -e libflow_display.so 符号化):\n");
    for (int i = 0; i < nfr; ++i) {
        put("  #");
        putdec(static_cast<unsigned long>(i));
        put(" ");
        puthex(reinterpret_cast<unsigned long>(frames[i]));
        put("\n");
    }
    put("基址换算：另读 last_crash_maps.txt 中目标 so 行，"
        "file_off = pc - start + offset列\n");
    // 【v11 OOM 取证】崩溃瞬间内存水位：「同步媒体闪退」已符号化实锤为
    // 堆耗尽（抛点=无害小字符串拼接，连异常对象都分配不出 →
    // __cxa_allocate_exception 直呼 terminate、无活动异常）——水位线
    // 直接佐证 OOM 终值。
    {
        const int sfd = ::open("/proc/self/status", O_RDONLY);
        if (sfd >= 0) {
            static char sbuf[4096]; // 静态省 handler 栈（g_in_handler 串行）
            std::size_t total = 0;
            for (;;) {
                if (total >= sizeof(sbuf) - 1) break;
                const ssize_t n = ::read(sfd, sbuf + total,
                                         sizeof(sbuf) - 1 - total);
                if (n <= 0) break;
                total += static_cast<std::size_t>(n);
            }
            ::close(sfd);
            sbuf[total] = '\0';
            put("内存水位:\n");
            std::size_t i = 0;
            while (i < total) {
                std::size_t e = i;
                while (e < total && sbuf[e] != '\n') ++e;
                const char saved = sbuf[e];
                sbuf[e] = '\0';
                if (std::strncmp(sbuf + i, "VmSize", 6) == 0 ||
                    std::strncmp(sbuf + i, "VmRSS",  5) == 0 ||
                    std::strncmp(sbuf + i, "VmHWM",  5) == 0) {
                    put("  ");
                    put(sbuf + i);
                    put("\n");
                }
                sbuf[e] = saved;
                i = e + 1;
            }
        }
    }
    buf[off] = '\0';

    // 落盘（web 静态根优先——浏览器可读；filesDir 根保底）。
    // 【v10 顺序教训】maps 必须最先写：v9 现场 crash.txt 完整而 maps 缺失
    // ——handler 跑在崩溃线程原栈（无 SA_ONSTACK），栈余量耗尽于中段；
    // maps 是帧地址符号化的唯一钥匙，不许死在半路。
    if (g_dir[0]) {
        // ---- ① maps 全量（各 so 加载基址 + offset 列，addr2line 必需）----
        // 纯 open/read/write 系统调用，async-signal-safe（无堆分配、无 CRT
        // 格式化）；缓冲静态化省 16KB handler 栈（g_in_handler 保证串行）。
        static char mbuf[16384];
        const char* mrels[2] = {"/web/last_crash_maps.txt",
                                "/last_crash_maps.txt"};
        for (const char* mrel : mrels) {
            char mpath[640];
            std::size_t d = 0;
            while (g_dir[d] && d < sizeof(mpath) - 1) { mpath[d] = g_dir[d]; ++d; }
            const char* r = mrel;
            while (*r && d < sizeof(mpath) - 1) mpath[d++] = *r++;
            mpath[d] = '\0';
            const int mfd = ::open(mpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (mfd < 0) continue;
            const int rfd = ::open("/proc/self/maps", O_RDONLY);
            if (rfd >= 0) {
                for (;;) {
                    const ssize_t n = ::read(rfd, mbuf, sizeof(mbuf));
                    if (n <= 0) break;
                    writeAll(mfd, mbuf, static_cast<std::size_t>(n));
                }
                ::close(rfd);
            }
            ::close(mfd);
        }
        // ---- ② backtrace 文本 ----
        char path[640];
        const char* rels[2] = {"/web/last_crash.txt", "/last_crash.txt"};
        for (const char* rel : rels) {
            std::size_t d = 0;
            while (g_dir[d] && d < sizeof(g_dir) - 1) { path[d] = g_dir[d]; ++d; }
            const char* r = rel;
            while (*r && d < sizeof(path) - 1) path[d++] = *r++;
            path[d] = '\0';
            const int fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd >= 0) {
                writeAll(fd, buf, off);
                ::close(fd);
            }
        }
    }
    __android_log_print(ANDROID_LOG_ERROR, "FlowDisplay.Crash", "%s", buf);

    // 交还系统：按默认语义终止（SIGABRT→abort 语义；SIGSEGV→段错误终止）
    ::signal(sig, SIG_DFL);
    ::raise(sig);
}

} // namespace

void setCrashDumpDir(const char* utf8_dir) {
    if (!utf8_dir || !*utf8_dir) return;
    std::size_t i = 0;
    for (; utf8_dir[i] && i < sizeof(g_dir) - 1; ++i) g_dir[i] = utf8_dir[i];
    g_dir[i] = '\0';
}

void installCrashDump() {
    // 【只注册一次】重复调用安全（同 Windows 版守卫语义）
    static const bool installed = [] {
        // 【terminate 留证 2026-11-24】SIGABRT 的最大来源 = uncaught 异常
        // （std::terminate 终点；libc++ 的 "terminating with uncaught
        // exception" 走 logcat——卓易通容器内宿主侧拿不到）。abort 前拦截：
        // 把异常类型/what() 落 <dir>/web/last_terminate.txt（浏览器
        // http://IP:8995/last_terminate.txt 直接读）——与 SIGABRT handler
        // 链式（本 handler 写完 std::abort() → SIGABRT → crashHandler 落
        // backtrace+maps），三文件拼出完整现场。terminate 时堆通常可用
        //（异常对象未析构），std::string 暂用无碍。
        std::set_terminate([] {
            char msg[1024];
            std::size_t off = 0;
            auto puts_ = [&msg, &off](const char* s) {
                while (*s && off < sizeof(msg) - 1) msg[off++] = *s++;
            };
            puts_("uncaught exception");
            try {
                if (auto ep = std::current_exception())
                    std::rethrow_exception(ep);
                else
                    puts_("（terminate 无活动异常——直呼 abort/断言类）");
            } catch (const std::exception& e) {
                puts_(": "); puts_(e.what());
            } catch (...) {
                puts_(": 非标准异常类型");
            }
            msg[off] = '\0';
            if (g_dir[0]) {
                char tpath[640];
                std::size_t d = 0;
                while (g_dir[d] && d < sizeof(tpath) - 1) { tpath[d] = g_dir[d]; ++d; }
                const char* trel = "/web/last_terminate.txt";
                const char* r = trel;
                while (*r && d < sizeof(tpath) - 1) tpath[d++] = *r++;
                tpath[d] = '\0';
                const int tfd = ::open(tpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (tfd >= 0) { writeAll(tfd, msg, off); ::close(tfd); }
            }
            __android_log_print(ANDROID_LOG_ERROR, "FlowDisplay.Crash", "%s", msg);
            std::abort();
        });
        // 【v10 备栈】信号 handler 换独立备栈跑（SA_ONSTACK + 128KB 静态
        // 区）：崩溃线程原栈余量不足时（深栈 terminate/段错误）handler
        // 自身再崩=只留半份现场——v9 实测 crash.txt 完整而 maps 缺失。
        // 静态 BSS 数组（非堆），g_in_handler 保证串行访问。
        static char altstack[128 * 1024];
        stack_t ss;
        ss.ss_sp    = altstack;
        ss.ss_size  = sizeof(altstack);
        ss.ss_flags = 0;
        ::sigaltstack(&ss, nullptr);
        struct sigaction sa {};
        sa.sa_sigaction = &crashHandler;
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
        ::sigaction(SIGSEGV, &sa, nullptr);
        ::sigaction(SIGBUS,  &sa, nullptr);
        ::sigaction(SIGFPE,  &sa, nullptr);
        ::sigaction(SIGILL,  &sa, nullptr);
        ::sigaction(SIGABRT, &sa, nullptr); // uncaught 异常/terminate 链终点
        ::sigaction(SIGTRAP, &sa, nullptr);
        return true;
    }();
    (void)installed;
}

#else // 其余平台：空实现（跨平台编译不破）

void setCrashDumpDir(const char*) {}

void installCrashDump() {}

#endif // _WIN32
