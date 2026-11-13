#pragma once
// 【幽灵 socket 治本 2026-11-13】Job Object 陪葬守卫。
//
// 根因链（与 SpawnNoInherit.hpp 同源）：
//   Windows 上 QProcess 以 bInheritHandles=TRUE 派生子进程（为接管 stdio），
//   本进程全部可继承句柄——包括 Poco ServerSocket 的 TCP 监听（8992 控制台、
//   17002 素材 HTTP，均无公有原始句柄可设非继承）与 UDP socket——被子进程整表继承。
//   子进程一旦比 Flow 活得久（Flow 崩溃时析构兜底不执行；或terminate() 对
//   headless 浏览器无效），继承来的监听 socket 被孤儿"续命"成幽灵：
//   - 重启 Flow 后 8992 bind 直接 WSAEADDRINUSE（ServerSocket 单参构造
//     reuseAddress=false）→ "HTTP 服务打不开"；
//   - 或连接被投递给无人 accept 的幽灵句柄 → 服务"时好时坏"。
//   崩溃次数越多孤儿越多，症状越顽固，杀光子进程/重启机器才恢复。
//
// 治本：把每个派生的子进程 AssignProcessToJobObject 到 KILL_ON_JOB_CLOSE 的 job——
//   Flow 无论正常退出还是崩溃，内核关闭 job 最后句柄时自动杀光整棵子进程树
//   （Chromium 自派的 GPU/renderer 孙进程默认继承 job，一并覆盖），
//   幽灵 socket 失去续命者。Win8+ 支持嵌套 job，与调试器/启动器共存无碍。
//
// 用法：子进程 started 后调用 assignChildToJob(qproc->processId())。

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace ms {

/// 进程级惰性单例：陪葬 job 句柄（失败返回 nullptr，调用方静默降级）
inline HANDLE jobGuardHandle()
{
    static HANDLE job = []() -> HANDLE {
        HANDLE h = ::CreateJobObjectW(nullptr, nullptr);
        if (!h) {
            return nullptr;
        }
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!::SetInformationJobObject(h, JobObjectExtendedLimitInformation, &info, sizeof(info))) {
            ::CloseHandle(h);
            return nullptr;
        }
        return h;
    }();
    return job;
}

/// 把已启动的子进程纳入陪葬 job（孙进程默认继承 job）。
/// 返回 false 仅表示降级为旧行为（子进程可能孤儿续命），不抛错。
inline bool assignChildToJob(unsigned long pid)
{
    if (pid == 0) {
        return false;
    }
    HANDLE job = jobGuardHandle();
    if (!job) {
        return false;
    }
    HANDLE proc = ::OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE, pid);
    if (!proc) {
        return false;
    }
    const BOOL ok = ::AssignProcessToJobObject(job, proc);
    ::CloseHandle(proc);
    return ok != FALSE;
}

} // namespace ms

#endif // _WIN32
