cmake_minimum_required(VERSION 3.10)

set(Module_Name "ROINode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

add_library(${Module_Name} SHARED
        ROIDataModel.hpp
        ROICommon.hpp
        ROIGpu.hpp
        ROIInterface.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
)
target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
        Qt${QT_VERSION_MAJOR}::Multimedia
        Qt${QT_VERSION_MAJOR}::OpenGL
        Qt${QT_VERSION_MAJOR}::OpenGLWidgets
        ${QtNodes_LIBRARIES}
        ${OpenCV_LIBRARIES}
        DataTypes
        PropertyTreeWidget
        StatusContainer
        BaseClass
        GUIElements
        TimestampGenerator
)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
set_target_properties(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
