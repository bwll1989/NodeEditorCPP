cmake_minimum_required(VERSION 3.10)

project(TimestampGenerator LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core WebSockets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebSockets Network)

set(TimestampGenerator_sources
        TimestampGenerator.cpp
        TimestampGenerator.hpp
)

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(TimestampGenerator SHARED ${TimestampGenerator_sources})

target_link_libraries(TimestampGenerator PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(TimestampGenerator PRIVATE TIMESTAMPGENERATOR_LIBRARY)