# 模块：DataTypes
# 说明：节点端口数据类型共享库

cmake_minimum_required(VERSION 3.10)
project(DataTypes LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets OpenGL)

set(DataTypes_sources
        AudioTimestampRingQueue.cpp
        AudioTimestampRingQueue.h
        TimestampRingQueueLockFree.hpp
        AudioData.h
        AudioData.cpp
        GpuTextureHandle.h
        GpuTextureHandle.cpp
        ImageGpuUpload.h
        ImageGpuUpload.cpp
        ImageGpuPass.h
        ImageGpuPass.cpp
        ImageReadback.h
        ImageReadback.cpp
        ImageTimestampRingQueue.cpp
        ImageTimestampRingQueue.h
        ImageData.h
        ImageData.cpp
        NodeDataList.hpp
        VariableData.h
        VariableData.cpp
        VecData.h
        VecData.cpp
        DataTypesExport.h
)

add_library(DataTypes SHARED ${DataTypes_sources})

target_link_libraries(DataTypes
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        QtNodes::QtNodes
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::OpenGL
        ${OpenCV_LIBRARIES}
)

target_include_directories(DataTypes PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/DataTypes>
)

target_compile_definitions(DataTypes PRIVATE DATATYPES_LIBRARY NODE_EDITOR_SHARED)
