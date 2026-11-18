# ============================================================================
#  Mesh 片段插件（mesh.plugin）——3D 模型片段（阶段 3A 2026-11-18）。
#  构建/输出/链接约定详见 ../Video/CMakeLists.txt 头注释（片段插件同款）：
#  MODULE + PREFIX "" + SUFFIX ".plugin"、产物直落 bin/plugins/TimeLine/、
#  ARCHIVE 重定向 build/archive/TimeLine。链接 = BaseClass（SceneClipModel
#  基类 dllimport）+ Qt6（MediaCore 不需要——无专属编辑区、无解码）。
#  【勿把 BaseClass 库头列入 SOURCES】AUTOMOC 只扫 target sources（详见
#  Video/CMakeLists.txt 同名注释）。
# ============================================================================
cmake_minimum_required(VERSION 3.10)

set(Module_Name "plugin_mesh")

project(${Module_Name})

add_library(${Module_Name} MODULE
        MeshClipModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp)

set_target_properties(${Module_Name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "mesh"
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
