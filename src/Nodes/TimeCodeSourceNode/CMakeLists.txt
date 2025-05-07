cmake_minimum_required(VERSION 3.5)

set(Module_Name "TimeCodeSourceNode")

project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(${Module_Name} SHARED
        ../../Common/Devices/LtcDecoder/ltcdecoder.h
        ../../Common/Devices/LtcDecoder/ltcdecoder.cpp
        TimeCodeInterface.h
        TimeCodeInterface.cpp
        TimeCodeSource.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        ../../Common/Devices/LtcReceiver/ltcreceiver.cpp
        ../../Common/Devices/LtcReceiver/ltcreceiver.h)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets 
        # Qt${QT_VERSION_MAJOR}::Multimedia
        ${QtNodes_LIBRARIES} 
        ${Libltc_LIBRARIES}
        ${portaudio_LIBRARIES}
        )

target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
