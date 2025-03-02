cmake_minimum_required(VERSION 3.16)
project(ImageClip VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON) 
# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)
set(FFMPEG_INCLUDE_DIRS "${DEPENDS_DIR}/ffmpeg-n7.1-39-g64e2864cb9-win64-gpl-shared-7.1/include")
set(FFMPEG_LIB_DIRS "${DEPENDS_DIR}/ffmpeg-n7.1-39-g64e2864cb9-win64-gpl-shared-7.1/lib")
include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories("${FFMPEG_LIB_DIRS}")
# 查找Qt包
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# 设置基础目录变量
set(TIMELINE_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget/TimeLineWidget)

# 设置源文件
set(SOURCES
    imageclipmodel.hpp
    imageclipdelegate.hpp
    imageclipplugin.hpp
    ${TIMELINE_SDK_DIR}/AbstractClipModel.hpp
    ${TIMELINE_SDK_DIR}/AbstractClipModel.cpp
    ${TIMELINE_SDK_DIR}/AbstractClipDelegate.hpp
    ${TIMELINE_SDK_DIR}/clippluginterface.hpp
)

# 创建插件库
add_library(ImageClip MODULE
    ${SOURCES}
)

# 设置目标属性
set_target_properties(ImageClip PROPERTIES
    PREFIX ""
    SUFFIX ".plugin"
)

# 链接Qt库
target_link_libraries(ImageClip PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
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
target_include_directories(ImageClip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget
) 