cmake_minimum_required(VERSION 3.10)

project(StatusContainer LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

set(StatusContainer_sources
        StatusContainer.cpp
        StatusContainer.h
        StatusItem.cpp
        StatusItem.h
        GlobalEventBus.hpp)

add_library(StatusContainer SHARED ${StatusContainer_sources})

target_include_directories(StatusContainer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/Common/Devices>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/Common/Devices>
)

# GUIElements 对使用自定义控件的节点为传递依赖；AppConfig 仅实现期需要
target_link_libraries(StatusContainer
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        GUIElements
    PRIVATE
        AppConfig
)

target_compile_definitions(StatusContainer PRIVATE STATUSCONTAINER_LIBRARY)
