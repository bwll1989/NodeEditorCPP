cmake_minimum_required(VERSION 3.10)

set(Module_Name "LibretroNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

add_library(${Module_Name} SHARED
        LibretroCore.h
        LibretroCore.cpp
        LibretroDataModel.hpp
        LibretroInterface.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
)

target_link_libraries(${Module_Name}
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        ${OpenCV_LIBRARIES}
        PropertyTreeWidget
        StatusContainer
        BaseClass
        DataTypes
        TimestampGenerator
        GUIElements
        MediaManger
        AppConfig)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

# 默认 core 随构建部署到 bin/；示例 ROM 部署到媒体库存储目录（启动扫描自动入库）
# 注意：带 ../.. 的混合路径在 CLion 捆绑 cmake 下 copy 会失败，必须 get_filename_component 绝对化
get_filename_component(LIBRETRO_CORES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdParty/libretro-cores" ABSOLUTE)
set(MEDIA_LIB_DIR "$ENV{USERPROFILE}/Documents/Flow/Medias")
add_custom_command(TARGET ${Module_Name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MEDIA_LIB_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIBRETRO_CORES_DIR}/mgba_libretro.dll"
        "${CMAKE_BINARY_DIR}/bin/mgba_libretro.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIBRETRO_CORES_DIR}/meteorain.gba"
        "${MEDIA_LIB_DIR}/meteorain.gba"
        COMMENT "Deploy libretro core (mGBA) to bin + MeteoRain ROM to media library")
