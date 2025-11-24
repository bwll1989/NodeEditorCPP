cmake_minimum_required(VERSION 3.10)

project(DelayListWidget LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network Gui Qml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network Gui Qml)
include_directories(${tinyosc_DIR})
set(DelayListWidget_sources
        DelayMessageListWidget.cpp
        DelayMessageListWidget.hpp
        DelayMessageItemWidget.cpp
        DelayMessageItemWidget.hpp
)

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(DelayListWidget SHARED ${DelayListWidget_sources} )

target_link_libraries(DelayListWidget PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Qml
        OSCTransmitter
)

target_compile_definitions(DelayListWidget PRIVATE DELAYLISTWIDGET_LIBRARY)