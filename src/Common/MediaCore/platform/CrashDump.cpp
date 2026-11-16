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

#else // 非 Windows：空实现（跨平台编译不破）

void installCrashDump() {}

#endif // _WIN32
