cmake_minimum_required(VERSION 3.10)
set(Module_Name "FaustNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

# ---- Faust 2.88.0（JIT 后端）----
# faust.dll 静态含 LLVM 17，导出 createDSPFactoryFromString 等 JIT 符号；
# 链接 faust.lib（import lib），部署时 faust.dll 必须在 exe 搜索路径下。
set(FAUST_ROOT "C:/Program Files/Faust" CACHE PATH "Faust installation root")
if(NOT EXISTS "${FAUST_ROOT}/include/faust/dsp/llvm-dsp.h")
    message(FATAL_ERROR "Faust not found at ${FAUST_ROOT}. Set FAUST_ROOT to the Faust installation directory.")
endif()
message(STATUS "FaustNode: found Faust at ${FAUST_ROOT}")

add_library(${Module_Name} SHARED
    FaustParamUI.hpp
    FaustAudioWorker.hpp
    FaustAudioWorker.cpp
    FaustDataModel.hpp
    PluginDefinition.hpp
    PluginDefinition.cpp
)

target_include_directories(${Module_Name} PRIVATE
    "${FAUST_ROOT}/include"
)

target_link_libraries(${Module_Name} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${QtNodes_LIBRARIES}
    GUIElements
    BaseClass
    DataTypes
    TimestampGenerator
    "${FAUST_ROOT}/lib/faust.lib"
)

target_compile_definitions(${Module_Name} PRIVATE
    UNTITLED_LIBRARY
    -DNODE_EDITOR_SHARED
    FAUST_SHARE_DIR="${FAUST_ROOT}/share/faust"
)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    PREFIX ""
    SUFFIX ".node")

# faust.dll 拷到 bin/（exe 所在目录），保证 QPluginLoader 加载本插件时能解析 JIT 符号
add_custom_command(TARGET ${Module_Name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FAUST_ROOT}/lib/faust.dll"
        "${CMAKE_BINARY_DIR}/bin/faust.dll"
    COMMENT "FaustNode: copying faust.dll to bin/"
    VERBATIM)
