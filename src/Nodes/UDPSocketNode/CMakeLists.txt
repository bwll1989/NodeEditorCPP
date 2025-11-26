cmake_minimum_required(VERSION 3.10)
set(Module_Name "UDPSocketNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 函数级注释：将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
set(MAKE_CXX_STANDARD_REQUIRED ON)
add_library(${Module_Name} SHARED
        UDPSocketDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        UDPSocketInterface.hpp
        ../../Common/Devices/UdpSocket/UdpSocket.h
        ../../Common/Devices/UdpSocket/UdpSocket.cpp)

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} Qt${QT_VERSION_MAJOR}::Network OSCTransmitter)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

