cmake_minimum_required(VERSION 3.10)

project(PropertyTreeWidget LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network Gui Qml )
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network Gui Qml )

set(PropertyTreeWidget_sources
        PropertyTreeWidget.h
        PropertyTreeWidget.cpp
)

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(PropertyTreeWidget SHARED ${PropertyTreeWidget_sources} )

target_link_libraries(PropertyTreeWidget PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Qml
        ${Qtpropertybrowser_LIBRARIES}
        OSCTransmitter
        GUIElements
        AppConfig
)

target_compile_definitions(PropertyTreeWidget PRIVATE PROPERTYTREEWIDGET_LIBRARY)