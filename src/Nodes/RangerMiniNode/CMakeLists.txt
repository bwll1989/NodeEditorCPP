cmake_minimum_required(VERSION 3.10)
set(Module_Name "RangerMiniNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

if(NOT WIN32)
    message(WARNING "RangerMiniNode currently supports Windows candleLight/WinUSB only; skipped.")
    return()
endif()

enable_language(C)

set(QCUSTOMPLOT_DIR "${CMAKE_SOURCE_DIR}/3rdParty/qcustomplot")

add_library(${Module_Name} SHARED
        RangerMiniDataModel.hpp
        RangerMiniInterface.hpp
        RangerMiniController.h
        RangerMiniController.cpp
        NavControllerNode/NavControllerDataModel.hpp
        NavControllerNode/NavControllerInterface.hpp
        NavControllerNode/RsPath.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        ${QCUSTOMPLOT_DIR}/qcustomplot.cpp
        ${QCUSTOMPLOT_DIR}/qcustomplot.h
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
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/NavControllerNode
        ${QCUSTOMPLOT_DIR}
        ../../Common/Devices/CanBus
        ../../Common/Devices/CanBus/candle_api
)

set_source_files_properties(
        ../../Common/Devices/CanBus/candle_api/candle.c
        ../../Common/Devices/CanBus/candle_api/candle_ctrl_req.c
        PROPERTIES LANGUAGE C
)

# QCustomPlot 体积大，关掉对该翻译单元的部分警告噪音（可选）
if(MSVC)
    set_source_files_properties(${QCUSTOMPLOT_DIR}/qcustomplot.cpp PROPERTIES COMPILE_FLAGS "/wd4100 /wd4127")
endif()

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

# 路径预览不需要 PDF 打印；关掉 PrintSupport 依赖（qcustomplot 的 savePdf 会空实现）
target_compile_definitions(${Module_Name} PRIVATE
        UNTITLED_LIBRARY
        NODE_EDITOR_SHARED
        QT_NO_PRINTER
)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
