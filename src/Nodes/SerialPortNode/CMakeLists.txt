cmake_minimum_required(VERSION 3.10)
set(Module_Name "SerialPortNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS SerialPort)

add_library(${Module_Name} SHARED
        SerialPortDataModel.hpp
        SerialPortInterface.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        ../../Common/Devices/SerialPort/SerialPort.h
        ../../Common/Devices/SerialPort/SerialPort.cpp
        ../../Common/Devices/SerialPort/SerialPortWorker.h
        ../../Common/Devices/SerialPort/SerialPortWorker.cpp
)

target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::SerialPort
        ${QtNodes_LIBRARIES}
        BuildInNodes
        DataTypes
)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
