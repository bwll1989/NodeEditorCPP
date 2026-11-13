#pragma once
// ===========================================================================
//  SpawnNoInherit.hpp —— bInheritHandles=FALSE 的安全进程派生（Windows）
// ---------------------------------------------------------------------------
// 【幽灵 socket 修复 2026-11-13】QProcess::startDetached 在 Windows 上以
// bInheritHandles=TRUE 创建子进程（Qt 为接管 stdio 一并继承全部可继承
// 句柄），本进程所有可继承句柄——含 Poco 的 UDP 心跳 socket（17001）、
// 素材 HTTP 监听（17002）等——会被整表复制给子进程。旧进程退出后这些
// "幽灵 socket" 随子进程继续存活，产生两类实害：
//   • 内核 socket 对象被子进程句柄续命，netstat 属主显示已死 PID；
//   • 新到 TCP 连接被投递给无人 recv 的旧监听句柄 → 永不响应
//     （display 端 "素材拉取 Timeout" 实证）；UDP 单播帧同样进黑洞
//     （display→editor 状态上报丢失）。
// 自重启（MainWindow::restartAndOpenFlow）是实证踩雷路径：旧 Flow 派生
// 新 Flow 后退出，新 Flow 的 17001/17002 全被幽灵句柄劫持。
// 处置：GUI 子进程无需继承 stdio，统一改走 CreateProcessW
// (bInheritHandles=FALSE)——与 MediaDisplay 自重启既有正确实现
// （tools/MediaDisplay/main.cpp 同参数）同一模式。
// ===========================================================================
#include <QString>
#include <QDir>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace ms {

/// 以不继承句柄方式派生 detached 进程（成功后调用方继续走自身的退出流程）。
/// \param exe     可执行文件绝对路径（native 分隔符由本函数统一转换）
/// \param rawArgs 命令行参数串（不含程序名；含空格的参数请自行加引号）
/// \return 是否成功创建进程
inline bool spawnNoInherit(const QString& exe, const QString& rawArgs) {
    const std::wstring exeW = QDir::toNativeSeparators(exe).toStdWString();
    // 命令行约定：首 token 为带引号程序名（CreateProcess 会与
    // lpApplicationName 核对后按 argv 规则解析其余部分）
    std::wstring cmd = L"\"" + exeW + L"\" " + rawArgs.toStdWString();
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    const BOOL ok = ::CreateProcessW(exeW.c_str(), cmd.data(), nullptr,
                                     nullptr, FALSE /*不继承任何句柄*/, 0,
                                     nullptr, nullptr, &si, &pi);
    if (ok) {
        ::CloseHandle(pi.hThread);
        ::CloseHandle(pi.hProcess);
    }
    return ok != FALSE;
}

} // namespace ms
#endif // _WIN32
