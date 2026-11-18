#pragma once
// CefBox 导出宏（沿用 DataTypesExport.h 模式：专有名导出宏，勿用通用 DLL_EXPORT）
#if defined(CEFBOX_LIBRARY)
#  define CEFBOX_EXPORT __declspec(dllexport)
#else
#  define CEFBOX_EXPORT __declspec(dllimport)
#endif
