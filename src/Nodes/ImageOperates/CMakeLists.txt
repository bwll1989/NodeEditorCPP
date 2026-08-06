cmake_minimum_required(VERSION 3.10)

set(Module_Name "ImageOperates")

project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
include_directories(${OpenCV_DIR})

add_library(${Module_Name} SHARED
        ImageOperateCommon.hpp
        FlipImageOperateModel.hpp
        CrossImageOperateModel.hpp
        AddImageOperateModel.hpp
        SubtractImageOperateModel.hpp
        DifferenceImageOperateModel.hpp
        BlurImageOperateModel.hpp
        CropImageOperateModel.hpp
        ChromaKeyImageOperateModel.hpp
        UnderImageOperateModel.hpp
        OverImageOperateModel.hpp
        MonochromeImageOperateModel.hpp
        LevelImageOperateModel.hpp
        ChannelImageOperateModel.hpp
        ImageSwitchDataModel.hpp
        ImageThresholdDataModel.hpp
        ImageScaleDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
)

target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::OpenGL
        ${QtNodes_LIBRARIES}
        ${OpenCV_LIBRARIES}
        PropertyTreeWidget
        DataTypes
        StatusContainer
        BaseClass
        TimestampGenerator
)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
