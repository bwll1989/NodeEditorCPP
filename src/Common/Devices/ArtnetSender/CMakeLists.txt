cmake_minimum_required(VERSION 3.10)

project(ArtnetService LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Network)

set(ArtnetService_sources
        ArtnetService.cpp
        ArtnetService.h
        ArtnetStateTable.h
        ArtnetFrame.h
        ArtnetFrame.cpp
        ../UdpSocket/UdpSocket.cpp
        ../UdpSocket/UdpSocket.h
)

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(ArtnetService SHARED ${ArtnetService_sources})

# AppConfig：发送心跳读取全局合并策略（ConfigManager::getArtnetMergeMode）
target_link_libraries(ArtnetService PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Network AppConfig)

target_compile_definitions(ArtnetService PRIVATE ARTNETSERVICE_LIBRARY)