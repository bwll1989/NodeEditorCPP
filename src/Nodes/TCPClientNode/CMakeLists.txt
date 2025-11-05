cmake_minimum_required(VERSION 3.10)
set(Module_Name "TCPClientNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
add_library(${Module_Name} SHARED
        TCPClientDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        ../../Common/Devices/TcpClient/TcpClient.h
        ../../Common/Devices/TcpClient/TcpClient.cpp
        ../../Common/Devices/TcpClient/TcpClientWorker.h
       ../../Common/Devices/TcpClient/TcpClientWorker.cpp
)

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} Qt${QT_VERSION_MAJOR}::Network OSCTransmitter)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")


