cmake_minimum_required(VERSION 3.10)

set(Module_Name "ArtnetOutNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

string(TIMESTAMP BUILD_TS "%Y%m%d")
# 将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
add_library(${Module_Name} SHARED
        ArtnetOutDataModel.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        ArtnetOutInterface.hpp
        ../../Common/Devices/UdpSocket/UdpSocket.cpp
        ../../Common/Devices/UdpSocket/UdpSocket.h
        ../../../3rdParty/tinyosc-msvc/tinyosc.c
        ../../../3rdParty/tinyosc-msvc/tinyosc.h)
# 添加ArtnetTransmitter的头文件路径
target_include_directories(${Module_Name} PRIVATE
        ../../Common/Devices/ArtnetSender
)
# 确保ArtnetTransmitter在ArtnetOutNode之前构建
add_dependencies(${Module_Name} ArtnetTransmitter)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network ${QtNodes_LIBRARIES} ArtnetTransmitter)
target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

