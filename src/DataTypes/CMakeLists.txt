# 模块：BuildInNodes
# 说明：将内置节点统一编译成静态库，供主工程或其它模块链接使用

cmake_minimum_required(VERSION 3.10)
project(DataTypes LANGUAGES CXX)

# 启用 Qt 的自动处理（如需）
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt（兼容 Qt5/Qt6）
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

# 源文件列表
set(DataTypes_sources
    AudioTimestampRingQueue.cpp
    AudioTimestampRingQueue.h
    AudioData.h
    AudioData.cpp
    ImageData.h
    ImageData.cpp
    NodeDataList.hpp
    RectData.h
    RectsData.h
    VariableData.h
    VariableData.cpp
    DataTypesExport.h
)

# 生成静态库（也可改为 SHARED，如需导出符号）
add_library(DataTypes SHARED ${DataTypes_sources})

# 链接 Qt 和 QtNodes（QtNodes_LIBRARIES 在主工程中全局设置）
target_link_libraries(DataTypes PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    ${OpenCV_LIBRARIES}
    ${QtNodes_LIBRARIES}
)

# 暴露包含目录，便于外部通过 Common/BuildInNodes/... 头文件路径包含
target_include_directories(DataTypes PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(DataTypes PRIVATE DATATYPES_LIBRARY -DNODE_EDITOR_SHARED)
