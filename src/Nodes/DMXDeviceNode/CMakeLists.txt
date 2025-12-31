cmake_minimum_required(VERSION 3.10)

set(Module_Name "DMXDeviceNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Enable Qt automoc
set(CMAKE_AUTOMOC ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
add_library(${Module_Name} SHARED
        ../../Common/Devices/TcpServer/TcpServer.cpp
        ../../Common/Devices/TcpServer/TcpServer.h
        DMXDeviceDataModel.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        DMXDeviceInterface.hpp)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets  Qt${QT_VERSION_MAJOR}::Network ${QtNodes_LIBRARIES} DataTypes BuildInNodes)
target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

