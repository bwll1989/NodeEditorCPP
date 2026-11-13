// ============================================================================
//  platform/ConsoleUtf8.hpp —— Windows 控制台 UTF-8 输出（入口初始化工具）
//
//  解决的问题：终端打印中文乱码。
//
//  根因：全项目源码经 /utf-8 编译（根 CMakeLists 统一选项，因源码含大量
//  中文注释），程序内 fprintf/printf 输出的中文字符串是 UTF-8 字节流；
//  而 Windows 控制台默认输出代码页为 GBK(936)，终端按 GBK 解释 UTF-8
//  字节即呈现乱码（如 "ç»˜åˆ¶"、"鎵撳嵃"式花字）。
//
//  处理：各可执行入口 main() 首行调用 setupConsoleUtf8()，把控制台
//  输出/输入代码页统一设为 UTF-8(65001)，与程序字节流一致，中文即正常。
//
//  平台语义（重要）：
//    * Windows：
//      - 控制台程序直接运行（media_display / tools）：中文正常显示；
//      - GUI 程序从终端启动（media_editor）：attach 到的控制台同样生效；
//      - 输出重定向到文件：不受代码页影响（文件收到的本就是 UTF-8 字节，
//        用 UTF-8 查看器/日志平台读取即正常）；
//      - 老式 conhost（Win10 1809 之前）对 65001 兼容性差——本项目目标
//        环境 Win10+ / Windows Terminal，不做额外兼容。
//    * 其他平台：空实现（Linux/macOS 终端默认 UTF-8，无需处理）。
//
//  依赖原则说明：core 文档注明"不依赖平台 API"针对引擎逻辑（保证两端
//  行为一致）；本头是入口级初始化工具，仅 #ifdef _WIN32 内引用平台 API，
//  其他平台展开为空函数（零成本）。放 core 使全部 exe（display/editor/
//  tools）经 engine_core 的 PUBLIC include 根零配置引用。
// ============================================================================

#pragma once

#ifdef _WIN32
// WIN32_LEAN_AND_MEAN 必须在 windows.h 首次包含前置位，两个关键作用：
//  1) 排除 winsock.h —— 避免与 POCO Net（engine_core 依赖）引入的
//     winsock2.h 结构体重定义冲突（sockaddr/fd_set 等 LNK/C2011 错误）；
//  2) 排除 dlgs.h —— 其中定义了大量对话框控件 ID 宏（如 scr1=0x0490、
//     chx1、cmb1……），会污染业务代码的标识符（本地变量 scr1 直接
//     被展开成 0x0490，报 C2059"常数"）。
// NOMINMAX：排除 min/max 宏——否则 std::min/std::max 在 windows.h 之后
// 出现的编译单元被宏展开，报 C2589"::"右边的非法标记。
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>  // SetConsoleOutputCP / SetConsoleCP / CP_UTF8
#endif

namespace ms::platform {

/// 控制台代码页 → UTF-8（main() 首行调用；幂等，可安全重复调用）
inline void setupConsoleUtf8() noexcept {
#ifdef _WIN32
    ::SetConsoleOutputCP(CP_UTF8);  // stdout/stderr 输出代码页 → UTF-8
    ::SetConsoleCP(CP_UTF8);        // stdin 输入代码页（交互输入含中文时一致）
#endif
}

} // namespace ms::platform
