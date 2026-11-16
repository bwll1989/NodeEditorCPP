# ============================================================================
#  Artnet 片段插件（artnet.plugin）——结构对齐节点插件基准（每插件独立
#  CMakeLists，2026-11-15 用户定案「片段实现与节点同构」）。
#  构建/输出/链接约定详见 ../Video/CMakeLists.txt 头注释（六片段同款）。
#  本插件专属依赖（2026-11-15 片段移植）：
#    * ArtnetService —— 状态表写入/50Hz 泵广播（updateFrame/releaseOwner，
#      自启动工作线程——插件只写状态不管理泵）；
#    * MediaManger —— SelectorComboBox 的 DMX 媒体库分类（经 GUIElements
#      PUBLIC 传播 include 可达；显式链接表意图，对齐旧 TimeLineNode）；
#    * AppConfig —— MEDIA_LIBRARY_STORAGE_DIR（媒体库相对路径 → 绝对）；
#    * FFmpeg（avformat/avcodec/avutil，全路径 .lib——对齐 TimeLineNode
#      先例；include 路径来自根 CMakeLists 全局 include_directories）；
#    * MediaCore（ClipSectionContext）+ GUIElements（经 BaseClass PUBLIC
#      传播已可达，显式列出表意图）。
# ============================================================================
cmake_minimum_required(VERSION 3.10)

set(Module_Name "plugin_artnet")

project(${Module_Name})

add_library(${Module_Name} MODULE
        ArtnetClipModel.hpp
        ArtnetClipModel.cpp
        ArtnetClipSection.hpp
        ArtnetClipSection.cpp
        PluginDefinition.hpp
        PluginDefinition.cpp)

set_target_properties(${Module_Name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "artnet"
        SUFFIX ".plugin"
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY "${TIMELINE_PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${TIMELINE_PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/archive/TimeLine")

target_link_libraries(${Module_Name} PRIVATE
        BaseClass
        MediaCore
        GUIElements
        MediaManger
        AppConfig
        ArtnetService
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network)

# FFmpeg 解码（DMX 视频逐帧）——全路径 .lib，对齐 TimeLineNode 先例
#（避免依赖 link_directories；include 路径走根 CMakeLists 全局声明）
target_link_libraries(${Module_Name} PRIVATE
        "${FFMPEG_LIB_DIRS}/avformat.lib"
        "${FFMPEG_LIB_DIRS}/avcodec.lib"
        "${FFMPEG_LIB_DIRS}/avutil.lib")
