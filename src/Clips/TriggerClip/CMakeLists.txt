cmake_minimum_required(VERSION 3.16)
project(TriggerClip VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON) 
# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)

# 查找Qt包
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# 设置基础目录变量
set(TIMELINE_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget/TimeLineWidget/TimelineAbstract)
set(Devices_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Common/Devices)
# 设置源文件
set(SOURCES
    triggerclipmodel.hpp
    triggerclipplugin.hpp
    OSCMessageListWidget.hpp
    OSCMessageListWidget.cpp
    OSCMessageItemWidget.hpp
    OSCMessageItemWidget.cpp
    ${Devices_DIR}/OSCSender/OSCSender.h
    ${Devices_DIR}/OSCSender/OSCSender.cpp
    ${tinyosc_DIR}/tinyosc.c
    ${tinyosc_DIR}/tinyosc.h
    ${TIMELINE_SDK_DIR}/AbstractClipModel.hpp
    ${TIMELINE_SDK_DIR}/AbstractClipModel.cpp
    ${TIMELINE_SDK_DIR}/AbstractClipInterface.hpp)

# 创建插件库
add_library(TriggerClip MODULE
    ${SOURCES}
)

# 设置目标属性
set_target_properties(TriggerClip PROPERTIES
    PREFIX ""
    SUFFIX ".plugin"
)
# Windows 平台特定配置
if(WIN32)
    target_link_libraries(TriggerClip PRIVATE ws2_32)  # 添加 Winsock2 库
endif()
# 链接Qt库
target_link_libraries(TriggerClip PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# 包含目录
target_include_directories(TriggerClip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget
) 