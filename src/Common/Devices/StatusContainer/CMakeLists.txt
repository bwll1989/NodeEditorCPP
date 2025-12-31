cmake_minimum_required(VERSION 3.10)

project(StatusContainer LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

set(StatusContainer_sources
        StatusContainer.cpp
        StatusContainer.h
        StatusItem.cpp
        StatusItem.h)

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(StatusContainer SHARED ${StatusContainer_sources})

target_link_libraries(StatusContainer PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        GUIElements)

target_compile_definitions(StatusContainer PRIVATE STATUSCONTAINER_LIBRARY)