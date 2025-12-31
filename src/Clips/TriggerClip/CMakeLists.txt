cmake_minimum_required(VERSION 3.16)
project(TriggerClip VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON) 
# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)

# 查找Qt包
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Qml)

# 设置基础目录变量

set(Devices_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Common/Devices)
set(GUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Common/GUI)
set(QtTimeLine_DIR "${DEPENDS_DIR}/QtTimeLine")
set(QtTimeLine_LIBRARIES "${QtTimeLine_DIR}/install/lib/QtTimeLine.lib")
INCLUDE_DIRECTORIES("${QtTimeLine_DIR}/install/include")
# 设置源文件
set(SOURCES
    triggerclipmodel.hpp
    triggerclipplugin.hpp
    ${QtTimeLine_DIR}/install/include/AbstractClipModel.hpp
        ../AbstractClipDelegateModel.cpp
        ../AbstractClipDelegateModel.h
)

#    ${Devices_DIR}/OSCSender/OSCSender.h
#    ${Devices_DIR}/OSCSender/OSCSender.cpp
#    ${tinyosc_DIR}/tinyosc.c
#    ${tinyosc_DIR}/tinyosc.h)

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
    ${QtTimeLine_LIBRARIES}
        OSCTransmitter
    OscListWidget
    Qt${QT_VERSION_MAJOR}::Qml
        GUIElements
        StatusContainer
)

# 包含目录
target_include_directories(TriggerClip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget
    ${GUI_DIR}/OscListWidget
    ${Devices_DIR}/OSCSender

)
# 添加编译定义
target_compile_definitions(TriggerClip PRIVATE
        NODE_TIMELINE_SHARED
        NODE_TIMELINE_EXPORTS
)