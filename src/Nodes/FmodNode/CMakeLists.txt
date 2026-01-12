cmake_minimum_required(VERSION 3.10)
set(Module_Name "FmodNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
# 自动处理ui文件, 自动处理ui文件是有前提的, 必须在需要编译的头文件(add_executable)中的某个头文件中包含ui_xxx.h(比如#include "ui_model002.h")
set(CMAKE_AUTOMOC ON)
# 自动调用moc(元对象编译器)
set(CMAKE_AUTORCC ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
#设置
set(FMOD_DIR "${DEPENDS_DIR}/FMOD Studio API Windows/api/core")
set(FMOD_STUDIO_DIR "${DEPENDS_DIR}/FMOD Studio API Windows/api/studio")
set(FMOD_LIBRARIES 
    "${FMOD_DIR}/lib/x64/fmod_vc.lib"
    "${FMOD_STUDIO_DIR}/lib/x64/fmodstudio_vc.lib"
)
INCLUDE_DIRECTORIES(
    "${FMOD_DIR}/inc"
    "${FMOD_STUDIO_DIR}/inc"
)
message("-- Found FMOD Core at ${FMOD_DIR}")
message("-- Found FMOD Studio at ${FMOD_STUDIO_DIR}")
# 将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
add_library(${Module_Name} SHARED
        FmodDecoderDataModel.hpp
        FmodDecoderDataModel.cpp
        FmodDecoderInterface.hpp
        FmodDecoderInterface.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        FmodDecoderWorker.cpp
        FmodDecoderWorker.h
)

target_link_libraries(${Module_Name}  PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        ${QtNodes_LIBRARIES}
        OSCTransmitter
        ${FMOD_LIBRARIES}
        TimestampGenerator
        MediaManger
        GUIElements
        DataTypes
        BuildInNodes
)

target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

