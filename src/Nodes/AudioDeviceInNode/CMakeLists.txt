cmake_minimum_required(VERSION 3.5)
project(AudioDeviceInNode)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(portaudio_DIR "${CMAKE_SOURCE_DIR}/3rdParty/portaudio/install/lib/cmake/portaudio")
find_package(portaudio REQUIRED)
add_library(AudioDeviceInNode SHARED
        AudioDeviceInDataModel.hpp
        AudioDeviceInInterface.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp)

target_link_libraries(AudioDeviceInNode
        PUBLIC
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        portaudio)
target_compile_definitions(AudioDeviceInNode  PRIVATE UNTITLED_LIBRARY)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/cmake-build-debug")
SET_TARGET_PROPERTIES(AudioDeviceInNode PROPERTIES LINKER_LANGUAGE C
        ARCHIVE_OUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PREFIX "")
set_target_properties(AudioDeviceInNode PROPERTIES SUFFIX ".node")
install(TARGETS AudioDeviceInNode RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/cmake-build-debug")