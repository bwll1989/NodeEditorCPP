cmake_minimum_required(VERSION 3.10)
project(ArtnetClip VERSION 1.0 LANGUAGES CXX)

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
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network WebSockets QuickWidgets)
# 设置基础目录变量
add_definitions(-DQT_MESSAGELOGCONTEXT)
set(QtTimeLine_DIR "${DEPENDS_DIR}/QtTimeLine")
set(QtTimeLine_LIBRARIES "${QtTimeLine_DIR}/install/lib/QtTimeLine.lib")
INCLUDE_DIRECTORIES("${QtTimeLine_DIR}/install/include")
set(Devices_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Common/Devices)
# 设置源文件
set(SOURCES
    Artnetclipmodel.hpp
    Artnetclipmodel.cpp
    Artnetclipplugin.hpp
    ${QtTimeLine_DIR}/install/include/AbstractClipModel.hpp
#        ../../Widget/ExternalControl/ExternalControler.cpp
#       ../../Widget/ExternalControl/ExternalControler.hpp

)

# 创建插件库
add_library(ArtnetClip SHARED
    ${SOURCES}
)

# 设置目标属性
set_target_properties(ArtnetClip PROPERTIES
    PREFIX ""
    SUFFIX ".plugin"
)

# Windows 平台特定配置
if(WIN32)
    target_link_libraries(ArtnetClip PRIVATE ws2_32)  # 添加 Winsock2 库
endif()

# 链接Qt库
target_link_libraries(ArtnetClip PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
    Qt6::QuickWidgets
    avcodec
    avdevice
    avfilter
    avformat
    avutil
    postproc
    swresample
    swscale
    ${QtTimeLine_LIBRARIES}
    ArtnetTransmitter
        MediaManger
        GUIElements

)

# 包含目录
target_include_directories(ArtnetClip PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget
    ${Devices_DIR}/SocketTransmitter
)

# 添加编译定义
target_compile_definitions(ArtnetClip PRIVATE
    NODE_TIMELINE_SHARED
    NODE_TIMELINE_EXPORTS
)