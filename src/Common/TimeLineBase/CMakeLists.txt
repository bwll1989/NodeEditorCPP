cmake_minimum_required(VERSION 3.10)

project(TimeLineBase LANGUAGES CXX)

# ============================================================================
#  TimeLineBase —— 时间轴视图/模型 DLL（2026-11-13 由 ms_qttimeline 更名）
# ----------------------------------------------------------------------------
#  2026-11-12 收编自原 QtTimeLine（库内 OSC 体系已剪除，无需 Qt6::Network）；
#  2026-11-13 位置迁 src/Common/（core 资产，供 Flow 编辑器 / FlowSDK /
#  第三方时间轴扩展共同引用），静态库升级 SHARED（BaseClass.dll 同款模式），
#  并按用户命名统一定案更名 TimeLineBase（"XxxBase" = 基类库序列）：
#    * 全工程唯一类层次实例——qobject_cast / 信号槽跨模块单实例，
#      消除静态库多副本 vtable 隐患；
#    * 消费者以 target 名链接即自动继承 NODE_TIMELINE_SHARED（dllimport）。
#
#  导出语义（TimelineExports.hpp 三分支；宏名沿用 NODE_TIMELINE_* 不随
#  target 更名——改宏会波及 TimeLineNode 静态副本与全消费方）：
#    - 本 DLL 构建：NODE_TIMELINE_SHARED + NODE_TIMELINE_EXPORTS → 导出
#    - 消费者（链接本 target）：仅 NODE_TIMELINE_PUBLIC = dllimport
#    - NODE_TIMELINE_STATIC：保留给 TimeLineNode 内置引擎私有副本
#      （副本头与主 timeline 完全隔离，勿混用）
#
#  片段模型层（AbstractClipModel/SceneClipModel/ClipPlugInterface/
#  TimeLineStyle/BasePluginLoader 等）不在本库——在 BaseClass.dll
#  （2026-11-13 回归合并）；本库链接 BaseClass 取头与符号。
# ============================================================================

set(TimeLineBase_sources
        BaseTimeLineModel.cpp
        BaseTimeLineView.cpp
        BaseTimeLineWidget.cpp
        BaseTrackDelegate.cpp
        BaseTrackListView.cpp
        DefaultTimeLineToolBar.cpp
        BaseTimeLineModel.h
        BaseTimeLineView.h
        BaseTimeLineWidget.h
        BaseTrackDelegate.h
        BaseTrackListView.h
        DefaultTimeLineToolBar.h
        TimelineExports.hpp
)

add_library(TimeLineBase SHARED ${TimeLineBase_sources})

target_include_directories(TimeLineBase PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/Common/TimeLineBase>
)
# 构建本 DLL 时导出符号；消费者经 PUBLIC NODE_TIMELINE_SHARED 展开为 dllimport
target_compile_definitions(TimeLineBase
    PUBLIC  NODE_TIMELINE_SHARED
    PRIVATE NODE_TIMELINE_EXPORTS
)
# DefaultTimeLineToolBar 引 ConfigManager（AppConfig.dll，APPCONFIG_EXPORT）——
# DLL 化后符号必须自足（静态库时代由最终 exe 兜底解析的隐性依赖显式化）；
# IntDragValueWidget 经 BaseClass PUBLIC 传播（Elements/ 与 GUI 双 include 根均可用）
target_link_libraries(TimeLineBase PUBLIC
    BaseClass
    AppConfig
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)
