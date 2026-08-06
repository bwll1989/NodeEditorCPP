cmake_minimum_required(VERSION 3.10)
set(Module_Name "ModBusMaster")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
add_library(${Module_Name} SHARED
        PluginDefinition.hpp
        PluginDefinition.cpp
        ModBusMasterCoilControllerDataModel.hpp
        ModBusMasterCoilControllerDataModel.cpp
        ModBusMasterCoilControllerInterface.hpp
        ModBusMasterDiscreteInputDataModel.hpp
        ModBusMasterDiscreteInputDataModel.cpp
        ModBusMasterDiscreteInputInterface.hpp
        ../../Common/Devices/TcpClient/TcpClient.h
        ../../Common/Devices/TcpClient/TcpClient.cpp
        ../../Common/Devices/TcpClient/TcpClientWorker.h
        ../../Common/Devices/TcpClient/TcpClientWorker.cpp
)
target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
        Qt${QT_VERSION_MAJOR}::Network
        ${QtNodes_LIBRARIES}
        PropertyTreeWidget
        StatusContainer
        BaseClass
        DataTypes
        TimestampGenerator)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
