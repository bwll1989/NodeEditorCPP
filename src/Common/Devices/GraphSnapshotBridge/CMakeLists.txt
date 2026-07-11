cmake_minimum_required(VERSION 3.10)

project(GraphSnapshotBridge LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_library(GraphSnapshotBridge SHARED
        GraphSnapshotBridge.cpp
        GraphSnapshotBridge.hpp
)

target_link_libraries(GraphSnapshotBridge PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(GraphSnapshotBridge PRIVATE GRAPHSNAPSHOTBRIDGE_LIBRARY)
