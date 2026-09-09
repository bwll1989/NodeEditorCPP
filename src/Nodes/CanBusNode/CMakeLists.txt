cmake_minimum_required(VERSION 3.10)
set(Module_Name "CanBusNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

if(NOT WIN32)
    message(WARNING "CanBusNode currently supports Windows candleLight/WinUSB only; skipped.")
    return()
endif()

enable_language(C)

add_library(${Module_Name} SHARED
        CanBusDataModel.hpp
        CanBusInterface.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        ../../Common/Devices/CanBus/CanBus.h
        ../../Common/Devices/CanBus/CanBus.cpp
        ../../Common/Devices/CanBus/CanBusWorker.h
        ../../Common/Devices/CanBus/CanBusWorker.cpp
        ../../Common/Devices/CanBus/candle_api/candle.c
        ../../Common/Devices/CanBus/candle_api/candle_ctrl_req.c
        ../../Common/Devices/CanBus/candle_api/candle.h
        ../../Common/Devices/CanBus/candle_api/candle_ctrl_req.h
        ../../Common/Devices/CanBus/candle_api/candle_defs.h
        ../../Common/Devices/CanBus/candle_api/ch_9.h
)

target_include_directories(${Module_Name} PRIVATE
        ../../Common/Devices/CanBus
        ../../Common/Devices/CanBus/candle_api
)

# candle_api is C (WinUSB)
set_source_files_properties(
        ../../Common/Devices/CanBus/candle_api/candle.c
        ../../Common/Devices/CanBus/candle_api/candle_ctrl_req.c
        PROPERTIES LANGUAGE C
)

target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        PropertyTreeWidget
        StatusContainer
        BaseClass
        DataTypes
        winusb
        setupapi
        ole32
        uuid
)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
