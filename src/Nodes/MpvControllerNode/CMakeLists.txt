cmake_minimum_required(VERSION 3.10)
set(Module_Name "MpvControllerNode")
project(${Module_Name})

set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
add_library(${Module_Name} SHARED
        MpvControllerDataModel.hpp
        MpvControllerInterface.hpp
        ../../Common/GUI/QPropertyBrowser/QPropertyBrowser.cpp
        ../../Common/GUI/QPropertyBrowser/QPropertyBrowser.h

        PluginDefinition.hpp
        PluginDefinition.cpp
        HttpClient.cpp
        HttpClient.h)
#set_target_properties(TextSourceNode PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} Qt${QT_VERSION_MAJOR}::Network ${Qtpropertybrowser_LIBRARIES})
target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

