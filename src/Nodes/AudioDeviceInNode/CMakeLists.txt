cmake_minimum_required(VERSION 3.5)
project(AudioDeviceInNode)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PLUGIN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/DataFlow)
add_library(AudioDeviceInNode SHARED
        AudioDeviceInDataModel.hpp
        AudioDeviceInInterface.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp)

target_link_libraries(AudioDeviceInNode
        PUBLIC
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        portaudio)
target_compile_definitions(AudioDeviceInNode  PRIVATE UNTITLED_LIBRARY)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")