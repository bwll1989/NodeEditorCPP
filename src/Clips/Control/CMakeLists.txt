# ============================================================================
#  Control 片段插件（control.plugin）——结构对齐节点插件基准（每插件独立
#  CMakeLists，2026-11-15 用户定案「片段实现与节点同构」）。
#  构建/输出/链接约定详见 ../Video/CMakeLists.txt 头注释（六片段同款）。
#  专属编辑区依赖（Control 现有）：MediaCore（ClipSectionContext）+
#  GUIElements（经 BaseClass PUBLIC 传播已可达，显式列出表意图）+
#  OscListWidget（OSC 消息列表控件——其头引 QJSEngine（表达式求值），
#  其 Qml 依赖是 PRIVATE 链不传播，插件 TU 需要头路径自链，对齐
#  OscListWidget）+ StatusContainer（parseOSC 发送入口，PUBLIC include
#  含 src 根）。
# ============================================================================
cmake_minimum_required(VERSION 3.10)

set(Module_Name "plugin_control")

project(${Module_Name})

add_library(${Module_Name} MODULE
        ControlClipModel.hpp
        ControlClipSection.hpp
        ControlClipSection.cpp
        PluginDefinition.hpp
        PluginDefinition.cpp)

set_target_properties(${Module_Name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "control"
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
        OscListWidget
        StatusContainer
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Qml)
