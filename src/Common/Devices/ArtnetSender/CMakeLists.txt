cmake_minimum_required(VERSION 3.10)

project(ArtnetTransmitter LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Network)

set(ArtnetTransmitter_sources
        ArtnetTransmitter.cpp
        ArtnetTransmitter.h
        ArtnetFrame.h
        ArtnetFrame.cpp
        ../UdpSocket/UdpSocket.cpp
        ../UdpSocket/UdpSocket.h
)

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(ArtnetTransmitter SHARED ${ArtnetTransmitter_sources})

target_link_libraries(ArtnetTransmitter PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Network)

target_compile_definitions(ArtnetTransmitter PRIVATE ARTNETTRANSMITTER_LIBRARY)