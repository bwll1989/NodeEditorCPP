cmake_minimum_required(VERSION 3.10)

# MTC 设备库（DLL）：MtcDecoder / MtcReceiver / MtcSender 三个设备类的统一封装
# - 对外导出三个类，rtmidi（WinMM 后端）与 winmm 依赖全部封装在 DLL 内部
# - 消费者：Flow / FlowRuntime / MTCGeneratorNode / MTCDecoderNode
project(MtcDevices LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 独立配置本子项目时兜底（常规构建中根工程已 find_package 并注入 QT_VERSION_MAJOR）
if(NOT QT_VERSION_MAJOR)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
endif()

set(MtcDevices_sources
        mtcdecoder.h
        mtcdecoder.cpp
        mtcreceiver.h
        mtcreceiver.cpp
        mtcsender.h
        mtcsender.cpp
)

add_library(MtcDevices SHARED ${MtcDevices_sources})

# MTCDEVICES_LIBRARY：编译 DLL 自身时导出符号，消费者导入（Q_DECL_EXPORT/IMPORT）
target_compile_definitions(MtcDevices PRIVATE MTCDEVICES_LIBRARY)

target_include_directories(MtcDevices PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/Common/Devices>
        $<INSTALL_INTERFACE:include>
)

# rtmidi（根 CMakeLists 定义的静态库 target，含 WinMM 后端宏与 winmm 链接）
# PUBLIC：消费者 include 本库头文件时需要 RtMidi.h 的 include 目录；
# 静态库按需拉取——消费者不引用 RtMidi 符号，因此 rtmidi 代码只存在于本 DLL 内
target_link_libraries(MtcDevices PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        rtmidi
        DataTypes)
