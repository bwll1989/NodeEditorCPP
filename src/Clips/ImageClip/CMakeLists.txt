cmake_minimum_required(VERSION 3.5)
project(ImageClip VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
enable_language(C)
# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)

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
        imageclipmodel.hpp
        imageclipplugin.hpp
        ${QtTimeLine_DIR}/install/include/AbstractClipModel.hpp
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
# Windows 平台特定配置
if(WIN32)
    target_link_libraries(ImageClip PRIVATE ws2_32)  # 添加 Winsock2 库
endif()
# 链接Qt库
target_link_libraries(ImageClip PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::WebSockets
        Qt6::QuickWidgets
        ${QtTimeLine_LIBRARIES}
        ClientController
)

# 包含目录
target_include_directories(ImageClip PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget
        ${Devices_DIR}/Singleton
        ${Devices_DIR}/SocketTransmitter
)

# 添加编译定义
target_compile_definitions(ImageClip PRIVATE
        NODE_TIMELINE_SHARED
        NODE_TIMELINE_EXPORTS
)