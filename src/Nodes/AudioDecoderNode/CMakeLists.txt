cmake_minimum_required(VERSION 3.5)
set(Module_Name "AudioDecoderNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${Module_Name} SHARED
        AudioDecoderDataModel.hpp
        AudioDecoderInterface.hpp
        AudioDecoder.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        ../../Common/GUI/QJsonModel/QJsonModel.hpp
        ../../Common/GUI/QJsonModel/QJsonModel.cpp
        ../../Common/GUI/QJsonModel/details/QUtf8.hpp
        ../../Common/Devices/AudioPipe/AudioPipe.cpp
        ../../Common/Devices/AudioPipe/AudioPipe.h)

target_link_libraries(${Module_Name}
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
#        SndFile::sndfile
        avcodec-60
        avdevice-60
        avfilter-9
        avformat-60
        avutil-58
        postproc-57
        swresample-4
        swscale-7
        portaudio
)

target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES LINKER_LANGUAGE C
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PREFIX ""
        SUFFIX ".node")

