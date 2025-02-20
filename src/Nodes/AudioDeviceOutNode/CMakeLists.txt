cmake_minimum_required(VERSION 3.5)
set(Module_Name "AudioDeviceOutNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(${Module_Name} SHARED
        AudioDeviceOutDataModel.hpp
        AudioDeviceOutInterface.hpp
        AudioOutputSelector.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        ../../Common/GUI/QJsonModel/QJsonModel.hpp
        ../../Common/GUI/QJsonModel/QJsonModel.cpp
        ../../Common/GUI/QJsonModel/details/QUtf8.hpp
        ../../Common/Devices/AudioPipe/AudioPipe.cpp
        ../../Common/Devices/AudioPipe/AudioPipe.h
)

target_link_libraries(${Module_Name}
        PUBLIC
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        ${portaudio_LIBRARIES})

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")