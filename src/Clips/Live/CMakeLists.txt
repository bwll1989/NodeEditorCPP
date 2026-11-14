# ============================================================================
#  Live 片段插件（live.plugin）——结构对齐节点插件基准（每插件独立
#  CMakeLists，2026-11-15 用户定案「片段实现与节点同构」）。
#  构建/输出/链接约定详见 ../Video/CMakeLists.txt 头注释（六片段同款）。
#  专属编辑区依赖（Live 现有）：MediaCore（Asset 模型/LiveIngest 注册表）
#  + GUIElements（DragValue 控件，经 BaseClass PUBLIC 传播已可达，显式
#  列出表意图）。
# ============================================================================
cmake_minimum_required(VERSION 3.10)

set(Module_Name "plugin_live")

project(${Module_Name})

add_library(${Module_Name} MODULE
        LiveClipModel.hpp
        LiveClipSection.hpp
        LiveClipSection.cpp
        PluginDefinition.hpp
        PluginDefinition.cpp)

set_target_properties(${Module_Name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "live"
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
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets)
