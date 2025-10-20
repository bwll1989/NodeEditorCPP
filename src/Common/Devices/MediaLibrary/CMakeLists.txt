cmake_minimum_required(VERSION 3.10)

project(MediaManger LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

set(MediaManger_sources
        MediaLibrary.cpp
        MediaLibrary.h
        )

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(MediaManger SHARED ${MediaManger_sources})

target_link_libraries(MediaManger PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets)

target_compile_definitions(MediaManger PRIVATE MEDIAMANGER_LIBRARY)
