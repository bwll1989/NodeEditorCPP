cmake_minimum_required(VERSION 3.10)

project(OscListWidget LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network Gui)
set(tinyosc_DIR "${DEPENDS_DIR}/tinyosc-msvc")
include_directories(${tinyosc_DIR})
set(OscListWidget_sources
        OSCMessageListWidget.cpp
        OSCMessageListWidget.hpp
        OSCMessageItemWidget.cpp
        OSCMessageItemWidget.hpp
        ../../../Common/Devices/OSCSender/OSCSender.h
        ../../../Common/Devices/OSCSender/OSCSender.cpp
        ${tinyosc_DIR}/tinyosc.c
        ${tinyosc_DIR}/tinyosc.h
)

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(OscListWidget SHARED ${OscListWidget_sources})

target_link_libraries(OscListWidget PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Gui)

target_compile_definitions(OscListWidget PRIVATE OSCLISTWIDGET_LIBRARY)