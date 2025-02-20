cmake_minimum_required(VERSION 3.5)
set(Module_Name "FloatSourceNode")

project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#添加QtNodes库目录
add_library(${Module_Name} SHARED
        FloatSourceDataModel.hpp
        FloatSourceInterface.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        ../../Common/GUI/qt-value-slider/doubleslider.hpp
        ../../Common/GUI/qt-value-slider/doubleslider.cpp
        ../../Common/GUI/qt-value-slider/intslider.hpp
        ../../Common/GUI/qt-value-slider/intslider.cpp
        ../../Common/GUI/qt-value-slider/valueslider.hpp
        ../../Common/GUI/qt-value-slider/valueslider.cpp
)

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES})

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
