cmake_minimum_required(VERSION 3.10)

project(NDVController LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

set(NDVController_sources
        NDVController.cpp
        NDVController.h
)

add_library(NDVController SHARED ${NDVController_sources})

target_link_libraries(NDVController PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)

target_compile_definitions(NDVController PRIVATE NDVController_LIBRARY)
