cmake_minimum_required(VERSION 3.5)

set(Module_Name "VST3PluginNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(SMTG_ADD_VST3_HOSTING_SAMPLES 0)
set(SMTG_ADD_VST3_HOSTING_SAMPLES 0 CACHE INTERNAL "")
include_directories(${VST3_DIR})
add_library(${Module_Name} SHARED
        VST3PluginDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        VST3PluginInterface.hpp
        ../../Common/GUI/QPropertyBrowser/QPropertyBrowser.cpp
        ../../Common/GUI/QPropertyBrowser/QPropertyBrowser.h
        ${VST3_DIR}/public.sdk/source/vst/hosting/module_win32.cpp
        ${VST3_DIR}/public.sdk/source/vst/hosting/plugprovider.cpp
        ${VST3_DIR}/public.sdk/source/vst/hosting/plugprovider.h
        UI/WindowContainer.hpp
        Container.cpp
        Container.h
)
#set(APP_PROPERTIES
#        LINK_FLAGS "/SUBSYSTEM:windows"
##)
#set_target_properties(${Module_Name}
#        PROPERTIES
##        ${APP_PROPERTIES}
#)
#set_target_properties(TextSourceNode PROPERTIES LINKER_LANGUAGE C)
link_directories(${VST3_LIBRARIES_DIR})
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        ${Qtpropertybrowser_LIBRARIES}
        base
        pluginterfaces
        sdk
        sdk_common
        sdk_hosting
)
target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES LINKER_LANGUAGE C
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PREFIX ""
        SUFFIX ".node")

