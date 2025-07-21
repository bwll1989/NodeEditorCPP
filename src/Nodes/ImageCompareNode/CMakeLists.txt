cmake_minimum_required(VERSION 3.5)
set(Module_Name "ImageCompareNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
INCLUDE_DIRECTORIES(${OpenCV_DIR})
add_library(${Module_Name}  SHARED
		ImageCompareDataModel.hpp
		ImageCompareInterface.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp)

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
		Qt${QT_VERSION_MAJOR}::Concurrent
		${QtNodes_LIBRARIES}
		${OpenCV_LIBRARIES})

target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")