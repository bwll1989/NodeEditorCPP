cmake_minimum_required(VERSION 3.5)
project(PlayerClip VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
enable_language(C)
# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)
set(FFMPEG_INCLUDE_DIRS "${DEPENDS_DIR}/ffmpeg-n7.1-39-g64e2864cb9-win64-gpl-shared-7.1/include")
set(FFMPEG_LIB_DIRS "${DEPENDS_DIR}/ffmpeg-n7.1-39-g64e2864cb9-win64-gpl-shared-7.1/lib")
include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories("${FFMPEG_LIB_DIRS}")
# 查找Qt包
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
set(tinyosc_DIR "${DEPENDS_DIR}/tinyosc-msvc")
include_directories(${tinyosc_DIR})
message("-- Found tinyosc ${tinyosc_DIR}")
# 设置基础目录变量
add_definitions(-DQT_MESSAGELOGCONTEXT)
set(TIMELINE_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget/TimeLineWidget/TimelineAbstract)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common/Devices)
set(Devices_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Common/Devices)
# 设置源文件
set(SOURCES
    playerclipmodel.hpp
    playerclipplugin.hpp
    ${Devices_DIR}/OSCSender/OSCSender.h
    ${Devices_DIR}/OSCSender/OSCSender.cpp
    ${tinyosc_DIR}/tinyosc.c
    ${tinyosc_DIR}/tinyosc.h
    ${TIMELINE_SDK_DIR}/AbstractClipModel.hpp
    ${TIMELINE_SDK_DIR}/AbstractClipModel.cpp
    ${TIMELINE_SDK_DIR}/AbstractClipInterface.hpp
)

# 创建插件库
add_library(PlayerClip MODULE

    ${SOURCES}
)

# 设置目标属性
set_target_properties(PlayerClip PROPERTIES
    PREFIX ""
    SUFFIX ".plugin"
)
# Windows 平台特定配置
if(WIN32)
    target_link_libraries(PlayerClip PRIVATE ws2_32)  # 添加 Winsock2 库
endif()
# 链接Qt库
target_link_libraries(PlayerClip PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    avcodec
    avdevice
    avfilter
    avformat
    avutil
    postproc
    swresample
    swscale
)

# 包含目录
target_include_directories(PlayerClip PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget
) 