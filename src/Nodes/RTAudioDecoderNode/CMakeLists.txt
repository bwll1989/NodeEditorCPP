cmake_minimum_required(VERSION 3.5)
set(Module_Name "RTAudioDecoderNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(portaudio_DIR "${DEPENDS_DIR}/portaudio/install/lib/cmake/portaudio")
find_package(portaudio CONFIG REQUIRED)
message("-- Found portaudio ${portaudio_DIR}")
set(rtaudio_DIR "${DEPENDS_DIR}/rtaudio-6.0.1/install/share/rtaudio")
find_package(rtaudio REQUIRED)
message("-- Found rtaudio ${rtaudio_DIR}")
#set(SndFile_DIR "${DEPENDS_DIR}/libsndfile/install/lib/cmake/SndFile")
#find_package(SndFile REQUIRED)
#message("-- Found SndFile ${SndFile_DIR}")


add_library(${Module_Name} SHARED
        AudioDecoderDataModel.hpp
        AudioDecoderInterface.hpp
        AudioDecoder.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp)

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
        RtAudio::rtaudio
)

target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES LINKER_LANGUAGE C
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PREFIX ""
        SUFFIX ".node")

