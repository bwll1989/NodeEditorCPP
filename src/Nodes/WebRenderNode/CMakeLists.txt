cmake_minimum_required(VERSION 3.10)

set(Module_Name "WebRenderNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 20)  # CEF 156 头要求 C++20（cef_scoped_refptr 用 std::concepts）
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

add_library(${Module_Name} SHARED
        WebRenderBrowserProcess.h
        WebRenderBrowserProcess.cpp
        WebRenderCdpClient.h
        WebRenderCdpClient.cpp
        WebRenderDataModel.hpp
        WebRenderInterface.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
)

target_link_libraries(${Module_Name}
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::WebSockets
        ${QtNodes_LIBRARIES}
        ${OpenCV_LIBRARIES}
        PropertyTreeWidget
        StatusContainer
        BaseClass
        DataTypes
        TimestampGenerator
        GUIElements
        CefBox)  # CEF OSR 后端（P1；旧 Edge/CDP 文件 P4 删）

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED
        NOMINMAX WIN32_LEAN_AND_MEAN)  # CEF 头带入 windows.h，压掉 min/max 宏

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
