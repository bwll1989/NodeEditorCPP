cmake_minimum_required(VERSION 3.10)

set(Module_Name "ImageLoadNode")

project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
INCLUDE_DIRECTORIES(${OpenCV_DIR})
#link_directories("${OpenCV_LIBRARIES}")
add_library(${Module_Name} SHARED
        ImageLoaderModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} ${OpenCV_LIBRARIES} MediaManger GUIElements)

target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
