cmake_minimum_required(VERSION 3.10)
set(Module_Name "WebSocketServerNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
add_library(${Module_Name} SHARED
        WebSocketServerDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        WebSocketServerInterface.hpp
        ../../Common/Devices/WebSocketServer/WebSocketServer.h
         ../../Common/Devices/WebSocketServer/WebSocketServer.cpp)

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} Qt${QT_VERSION_MAJOR}::Network ${Qtpropertybrowser_LIBRARIES} Qt${QT_VERSION_MAJOR}::WebSockets OSCTransmitter)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")


