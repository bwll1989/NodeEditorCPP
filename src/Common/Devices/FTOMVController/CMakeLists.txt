cmake_minimum_required(VERSION 3.10)

project(FTOMVController LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

set(FTOMVController_sources
        FTOMVController.cpp
        FTOMVController.h
        OMVProtocol.cpp
        OMVProtocol.h
)

add_library(FTOMVController SHARED ${FTOMVController_sources})

target_link_libraries(FTOMVController PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)

target_compile_definitions(FTOMVController PRIVATE FTOMVController_LIBRARY)
