cmake_minimum_required(VERSION 3.10)

set(Module_Name "CurveNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 函数级注释：在配置阶段生成时间戳，并注入到所有目标（或指定目标）
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 函数级注释：将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

# 如需更细粒度到某个 target（推荐），用 target_compile_definitions：
# target_compile_definitions(CurveNodeTarget PRIVATE PLUGIN_VERSION="${BUILD_TS}")
add_library(${Module_Name} SHARED
        CurveDataModel.hpp
        CurveInterface.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        res.qrc)
target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Quick
        Qt${QT_VERSION_MAJOR}::QuickWidgets
        Qt${QT_VERSION_MAJOR}::Network
        OSCTransmitter
        ${QtNodes_LIBRARIES})

set(QRC_FILES
        res.qrc
)
target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

