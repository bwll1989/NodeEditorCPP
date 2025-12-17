cmake_minimum_required(VERSION 3.10)

set(Module_Name "InternalCommandsNode")

project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
set(GUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Common/GUI)
set(Devices_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Common/Devices)
add_library(${Module_Name} SHARED
        InternalControlModel.hpp
        InternalControlModel.cpp
        InternalControlInterface.hpp
        OscOutGroupModel.hpp
        OscOutGroupModel.cpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        )
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
        Qt${QT_VERSION_MAJOR}::Qml
        OscListWidget
        GUIElements
        ${QtNodes_LIBRARIES}
        OSCTransmitter
        DataTypes)

target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
