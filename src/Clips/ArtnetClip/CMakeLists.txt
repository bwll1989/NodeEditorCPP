cmake_minimum_required(VERSION 3.16)
project(ArtnetClip VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON) 
# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)

# 查找Qt包
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# 设置基础目录变量
set(TIMELINE_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget/TimeLineWidget)
# 设置源文件
set(SRC
    artnetclipmodel.hpp
    artnetclipdelegate.hpp
    artnetclipplugin.hpp
    ${TIMELINE_SDK_DIR}/AbstractClipModel.hpp
    ${TIMELINE_SDK_DIR}/AbstractClipModel.cpp
    ${TIMELINE_SDK_DIR}/AbstractClipDelegate.hpp
    ${TIMELINE_SDK_DIR}/clippluginterface.hpp
)

# 创建插件库
add_library(ArtnetClip MODULE
    ${SRC}

)

# 设置目标属性
set_target_properties(ArtnetClip PROPERTIES
    PREFIX ""
    SUFFIX ".plugin"
)

# 链接Qt库
target_link_libraries(ArtnetClip PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# 包含目录
target_include_directories(ArtnetClip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget
) 