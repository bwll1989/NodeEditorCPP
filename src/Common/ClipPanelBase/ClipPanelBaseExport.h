#pragma once

// ============================================================================
//  ClipPanelBaseExport.h —— ClipPanelBase.dll 导出宏
//
//  【2026-11-13 片段插件化】通用片段 UI 框架（ClipPropertyPanel 属性面板 +
//  TweenSection 变换/关键帧合并区）自主程序迁入本 DLL——片段专属编辑区
//  随各自片段插件走（src/Clips/<Type>/，经 ClipPlugInterface::
//  createPropertySection 工厂加载），主程序不再含任何片段 UI 源码。
//
//  命名序列（用户定案）："XxxBase" = BaseClass / TimeLineBase / SpoutBase /
//  StandaloneBase / ClipPanelBase。导出宏用库名专有宏——禁用通用名
//  （DLL_EXPORT 会被各节点 PluginDefinition.hpp 的 #ifndef 抢注）。
// ============================================================================

#if defined(CLIPPANELBASE_LIBRARY)
#  define CLIPPANELBASE_EXPORT Q_DECL_EXPORT
#else
#  define CLIPPANELBASE_EXPORT Q_DECL_IMPORT
#endif
