cmake_minimum_required(VERSION 3.10)
set(Module_Name "ViosoNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

# 第三方：https://github.com/vioso/VIOSO_API → 3rdParty/VIOSO_API
set(VIOSO_API_ROOT "${DEPENDS_DIR}/VIOSO_API")
if(NOT EXISTS "${VIOSO_API_ROOT}/Include/VIOSOWarpBlend.h")
    message(FATAL_ERROR "VIOSO API not found at ${VIOSO_API_ROOT}. Clone https://github.com/vioso/VIOSO_API into 3rdParty/VIOSO_API")
endif()

include_directories("${VIOSO_API_ROOT}/Include")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(VIOSO_LIB "${VIOSO_API_ROOT}/lib/VIOSOWarpBlend64.lib")
    set(VIOSO_DLL "${VIOSO_API_ROOT}/bin/VIOSOWarpBlend64.dll")
else()
    set(VIOSO_LIB "${VIOSO_API_ROOT}/lib/VIOSOWarpBlend.lib")
    set(VIOSO_DLL "${VIOSO_API_ROOT}/bin/VIOSOWarpBlend.dll")
endif()

add_library(${Module_Name} SHARED
    ViosoWarper.hpp
    ViosoWarper.cpp
    ViosoInterface.hpp
    ViosoDataModel.hpp
    PluginDefinition.hpp
    PluginDefinition.cpp
)

target_link_libraries(${Module_Name} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::OpenGL
    ${QtNodes_LIBRARIES}
    ${VIOSO_LIB}
    DataTypes
    StatusContainer
    BaseClass
    TimestampGenerator
    GUIElements
    MediaManger
    AppConfig
    opengl32
)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    PREFIX ""
    SUFFIX ".node")

# 运行时 DLL 需与主程序同目录
if(EXISTS "${VIOSO_DLL}")
    add_custom_command(TARGET ${Module_Name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VIOSO_DLL}"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
        COMMENT "Copy VIOSOWarpBlend DLL next to application")
endif()
