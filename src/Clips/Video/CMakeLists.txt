# ============================================================================
#  Video 片段插件（video.plugin）——结构对齐节点插件基准（每插件独立
#  CMakeLists，2026-11-15 用户定案「片段实现与节点同构」）：
#    * MODULE 库 + PREFIX "" + SUFFIX ".plugin"（QPluginLoader 按
#      "*.plugin" 过滤，本质仍是 Windows DLL）；
#    * 产物直落 bin/plugins/TimeLine/（TIMELINE_PLUGIN_OUTPUT_DIRECTORY，
#      根 CMakeLists 定义；该目录同时是 WorkScenePluginLoader 默认扫描
#      目录——无需 POST_BUILD 拷贝，重启即生效）；
#    * ARCHIVE（Windows DLL 的 import .lib + .exp）是链接副产物——插件由
#      QPluginLoader 运行时加载，无人链接其 import lib；落到插件目录会
#      污染可整目录分发的 bin/plugins/TimeLine/（2026-11-13 路径审查），
#      重定向到 build 内部目录；
#    * 链 BaseClass（AbstractClipModel/SceneClipModel 基类 dllimport——
#      跨模块 qobject_cast/dynamic_cast 均可用）+ Qt6。
#  【勿把 BaseClass 库头列入 SOURCES】AUTOMOC 只扫 target sources，列入
#  便在插件内重复生成 moc，与 dllimport 的 staticMetaObject 冲突
# （C2491/LNK2019）。库头经 include 路径解析；插件自有头必须列入。
# ============================================================================
cmake_minimum_required(VERSION 3.10)

set(Module_Name "plugin_video")

project(${Module_Name})

add_library(${Module_Name} MODULE
        VideoClipModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp)

set_target_properties(${Module_Name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "video"
        SUFFIX ".plugin"
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY "${TIMELINE_PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${TIMELINE_PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/archive/TimeLine")

target_link_libraries(${Module_Name} PRIVATE
        BaseClass
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets)
