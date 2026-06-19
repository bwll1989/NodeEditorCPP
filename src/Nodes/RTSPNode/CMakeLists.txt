cmake_minimum_required(VERSION 3.10)

set(Module_Name "RTSPNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
link_directories("${FFMPEG_LIB_DIRS}")
add_library(${Module_Name} SHARED
        RtspStreamReceiver.h
        RtspStreamReceiver.cpp
        RTSPInDataModel.hpp
        RTSPInInterface.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        ../../Common/BaseClass/AbstractDelegateModel.cpp
        ../../Common/BaseClass/AbstractDelegateModel.h)

target_link_libraries(${Module_Name}
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        ${OpenCV_LIBRARIES}
        avcodec
        avformat
        avutil
        swscale
        PropertyTreeWidget
        StatusContainer
        DataTypes
        TimestampGenerator)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
