cmake_minimum_required(VERSION 3.10)

project(DockHub LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Gui )
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Gui)
include_directories(${tinyosc_DIR})
set(DockHub_sources
        DockHub.cpp
        DockHub.hpp
)

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(DockHub SHARED ${DockHub_sources} )

target_link_libraries(DockHub PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Gui
        ${ADS_LIBRARIES}
)

target_compile_definitions(DockHub PRIVATE DOCKHUB_LIBRARY)
