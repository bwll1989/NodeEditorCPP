cmake_minimum_required(VERSION 3.5)
set(Module_Name "BoolSourceNode")
project(${Module_Name})

set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(PLUGIN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/DataFlow)

add_library(${Module_Name} SHARED
        BoolSourceDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp)
#set_target_properties(TextSourceNode PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} Qt${QT_VERSION_MAJOR}::Network)
target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
