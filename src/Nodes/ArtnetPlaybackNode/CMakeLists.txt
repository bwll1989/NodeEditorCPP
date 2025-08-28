cmake_minimum_required(VERSION 3.10)

set(Module_Name "ArtnetPlaybackNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Enable Qt automoc
set(CMAKE_AUTOMOC ON)

add_library(${Module_Name} SHARED
        ArtnetPlaybackDataModel.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        ArtnetPlaybackInterface.hpp
        ArtnetPlaybackDataModel.cpp)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        ${QtNodes_LIBRARIES}
        ${Qtpropertybrowser_LIBRARIES}
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale
)
target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

