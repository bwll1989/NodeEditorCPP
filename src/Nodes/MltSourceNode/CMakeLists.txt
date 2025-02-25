cmake_minimum_required(VERSION 3.5)
set(Module_Name "MltSourceNode")
project(${Module_Name})

set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(PLUGIN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/DataFlow)

add_library(${Module_Name} SHARED
        MltSourceDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        widgets/MLTPlayerWidget.cpp
        widgets/MLTPlayerWidget.h
)
#set_target_properties(TextSourceNode PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        ${QtNodes_LIBRARIES}
#        Mlt7::mlt
        Mlt7::mlt++
        Qt${QT_VERSION_MAJOR}::OpenGL
        Qt${QT_VERSION_MAJOR}::OpenGLWidgets
        Qt${QT_VERSION_MAJOR}::QuickWidgets)
target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

