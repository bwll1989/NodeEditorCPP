cmake_minimum_required(VERSION 3.5)
set(Module_Name "AudioPlayNode")
project(${Module_Name})

set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_TOOLCHAIN_FILE "C:/Users/bwll1/scoop/apps/vcpkg/2024.07.12/scripts/buildsystems/vcpkg.cmake"
#        CACHE STRING "Vcpkg toolchain file")
#set(portaudio_DIR "${DEPENDS_DIR}/portaudio/install/lib/cmake/portaudio")
#find_package(portaudio CONFIG REQUIRED)
#message("-- Found portaudio ${portaudio_DIR}")
#set(SndFile_DIR "${DEPENDS_DIR}/libsndfile/install/cmake")
#find_package(SndFile REQUIRED)
#message("-- Found SndFile ${SndFile_DIR}")
add_library(${Module_Name} SHARED
        AudioPlayDataModel.hpp
        AudioPlayInterface.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp)

target_link_libraries(${Module_Name}
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        portaudio
        SndFile::sndfile)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES LINKER_LANGUAGE C
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PREFIX ""
        SUFFIX ".node")
