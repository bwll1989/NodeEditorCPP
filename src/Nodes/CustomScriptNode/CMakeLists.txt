cmake_minimum_required(VERSION 3.10)
set(Module_Name "CustomScriptNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

set(SOURCE_FILES
        CustomScriptInterface.hpp
        CustomScriptInterface.cpp
        CustomScriptDataModel.hpp
        CustomScriptDataModel.cpp
        JSPluginManager.hpp
        JSPluginManager.cpp
        SupportWidgets.hpp
        ../../Common/Devices/JSEngineDefines/JSEngineDefines.hpp
        )

add_library(${Module_Name} SHARED
        ${SOURCE_FILES}
        PluginDefinition.hpp
        PluginDefinition.cpp)

# 移除 QScintilla 库链接
target_link_libraries(${Module_Name} PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Concurrent 
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Network
    ${QtNodes_LIBRARIES}
    OSCTransmitter
    )

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")


