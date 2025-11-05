cmake_minimum_required(VERSION 3.10)

set(Module_Name "ArtnetInNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(${Module_Name} SHARED
        ArtnetInDataModel.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        ArtnetInInterface.hpp
        ../../Common/Devices/ArtnetReceiver/ArtnetReceiver.cpp
        ../../Common/Devices/ArtnetReceiver/ArtnetReceiver.h
        ../../Common/Devices/UdpSocket/UdpSocket.cpp
        ../../Common/Devices/UdpSocket/UdpSocket.h
        ../../../3rdParty/tinyosc-msvc/tinyosc.c
        ../../../3rdParty/tinyosc-msvc/tinyosc.h)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets  Qt${QT_VERSION_MAJOR}::Network ${QtNodes_LIBRARIES})
target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

