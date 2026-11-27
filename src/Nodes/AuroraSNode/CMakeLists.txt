cmake_minimum_required(VERSION 3.10)
set(Module_Name "AuroraSNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

set(AURORA_SDK_ROOT "${CMAKE_SOURCE_DIR}/3rdParty/aurora_remote_sdk_demo/aurora_remote_public")
set(AURORA_LIB_PATH "${AURORA_SDK_ROOT}/lib/win64")
set(AURORA_LIB_NAME slamtec_aurora_remote_sdk)

include_directories("${AURORA_SDK_ROOT}/include")
link_directories(${AURORA_LIB_PATH})

add_library(${Module_Name} SHARED
        AuroraSDataModel.hpp
        AuroraSInterface.hpp
        AuroraSWorker.hpp
        AuroraSWorker.cpp
        PluginDefinition.hpp
        PluginDefinition.cpp
)

target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        ${AURORA_LIB_NAME}
        PropertyTreeWidget
        StatusContainer
        BaseClass
        DataTypes
        GUIElements
        AppConfig
)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

# 【2026-11-27】拷到 bin 根（exe 所在目录）而非插件目录：bin 根已有同版本副本，
# 插件目录副本冗余；QPluginLoader 解析不到时回退 exe 目录（发布=整个 bin，无单插件部署场景）
add_custom_command(TARGET ${Module_Name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${AURORA_LIB_PATH}/slamtec_aurora_remote_sdk.dll"
        "${CMAKE_BINARY_DIR}/bin")
