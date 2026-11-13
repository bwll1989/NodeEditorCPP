#pragma once

// ============================================================================
//  TimelineExports —— 导出/静态宏（原 TimelineExports + Compiler +
//  OperatingSystem 三件套合并精简；本工程仅 MSVC/Windows 实编，其余分支
//  只保留主流交叉编译选项）
// ============================================================================

// ---- 编译器检测 -------------------------------------------------------------
#if defined(_MSC_VER)
#define NODE_TIMELINE_COMPILER "Microsoft Visual C++"
#define NODE_TIMELINE_COMPILER_MICROSOFT
#elif defined(__MINGW32__) || defined(__MINGW64__)
#define NODE_TIMELINE_COMPILER "MinGW"
#define NODE_TIMELINE_COMPILER_MINGW
#elif defined(__clang__)
#define NODE_TIMELINE_COMPILER "Clang"
#define NODE_TIMELINE_COMPILER_CLANG
#elif defined(__GNUC__)
#define NODE_TIMELINE_COMPILER "GNU"
#define NODE_TIMELINE_COMPILER_GNU
#define NODE_TIMELINE_COMPILER_GNU_VERSION_MAJOR __GNUC__
#else
#error "Current compiler is not supported."
#endif

// ---- 平台检测 ---------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
#define NODE_TIMELINE_PLATFORM "Windows"
#define NODE_TIMELINE_PLATFORM_WINDOWS
#else
#error "Timeline library supports Windows only in this project."
#endif

// ---- 符号可见性 -------------------------------------------------------------
#ifdef NODE_TIMELINE_PLATFORM_WINDOWS
#define NODE_TIMELINE_EXPORT __declspec(dllexport)
#define NODE_TIMELINE_IMPORT __declspec(dllimport)
#define NODE_TIMELINE_LOCAL
#elif NODE_TIMELINE_COMPILER_GNU_VERSION_MAJOR >= 4 || defined(NODE_TIMELINE_COMPILER_CLANG)
#define NODE_TIMELINE_EXPORT __attribute__((visibility("default")))
#define NODE_TIMELINE_IMPORT __attribute__((visibility("default")))
#define NODE_TIMELINE_LOCAL __attribute__((visibility("hidden")))
#else
#define NODE_TIMELINE_EXPORT
#define NODE_TIMELINE_IMPORT
#define NODE_TIMELINE_LOCAL
#endif

#ifdef __cplusplus
#define NODE_TIMELINE_DEMANGLED extern "C"
#else
#define NODE_TIMELINE_DEMANGLED
#endif

// ---- 静态/共享二选一（TimeLineNode 静态副本定义 NODE_TIMELINE_STATIC → PUBLIC 空；
// ---- BaseClass.dll 侧 clip 基类以 NODE_TIMELINE_SHARED 构建 → PUBLIC 导出）----
#if defined(NODE_TIMELINE_SHARED) && !defined(NODE_TIMELINE_STATIC)
#ifdef NODE_TIMELINE_EXPORTS
#define NODE_TIMELINE_PUBLIC NODE_TIMELINE_EXPORT
#else
#define NODE_TIMELINE_PUBLIC NODE_TIMELINE_IMPORT
#endif
#define NODE_TIMELINE_PRIVATE NODE_TIMELINE_LOCAL
#elif !defined(NODE_TIMELINE_SHARED) && defined(NODE_TIMELINE_STATIC)
#define NODE_TIMELINE_PUBLIC
#define NODE_TIMELINE_PRIVATE
#elif defined(NODE_TIMELINE_SHARED) && defined(NODE_TIMELINE_STATIC)
#ifdef NODE_TIMELINE_EXPORTS
#error "Cannot build as shared and static simultaneously."
#else
#error "Cannot link against shared and static simultaneously."
#endif
#else
#ifdef NODE_TIMELINE_EXPORTS
#error "Choose whether to build as shared or static."
#else
#error "Choose whether to link against shared or static."
#endif
#endif
