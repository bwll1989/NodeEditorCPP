# 模块：BuildInNodes
# 说明：将内置节点统一编译成静态库，供主工程或其它模块链接使用

cmake_minimum_required(VERSION 3.10)
project(BuildInNodes LANGUAGES CXX)

# 启用 Qt 的自动处理（如需）
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt（兼容 Qt5/Qt6），补齐 Network 组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

# 源文件列表
set(BuildInNodes_sources
    AudioInDataModel.cpp
    AudioInDataModel.hpp
    AudioOutDataModel.cpp
    AudioOutDataModel.hpp
    ImageInDataModel.cpp
    ImageInDataModel.hpp
    ImageOutDataModel.cpp
    ImageOutDataModel.hpp
    VariableInDataModel.cpp
    VariableInDataModel.hpp
    VariableOutDataModel.cpp
    VariableOutDataModel.hpp
    BoolSourceDataModel.hpp
    StringSourceDataModel.hpp
    IntSourceDataModel.hpp
    FloatSourceDataModel.hpp
    BuildInNodes.hpp
    ImageShowModel.cpp
    ImageShowModel.hpp
)

# 生成静态库（也可改为 SHARED，如需导出符号）
add_library(BuildInNodes STATIC ${BuildInNodes_sources})

# 链接 Qt、QtNodes，并补齐 Qt::Network 与 OSCTransmitter
target_link_libraries(BuildInNodes PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    ${QtNodes_LIBRARIES}
    OSCTransmitter
    DataTypes
    GUIElements
    ModelDataBridge
)

# 暴露包含目录，便于外部通过 Common/BuildInNodes/... 头文件路径包含
target_include_directories(BuildInNodes PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 编译定义（与工程中节点模块保持一致，必要时可调整）
target_compile_definitions(BuildInNodes PRIVATE -DNODE_EDITOR_SHARED)
