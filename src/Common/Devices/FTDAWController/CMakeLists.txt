cmake_minimum_required(VERSION 3.10)

project(FTDAWController LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

set(FTDAWController_sources
        FTDAWController.cpp
        FTDAWController.h
        ../TcpClient/TcpClient.cpp
        ../TcpClient/TcpClient.h
        ../TcpClient/TcpClientWorker.cpp
        ../TcpClient/TcpClientWorker.h
        )

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(FTDAWController SHARED ${FTDAWController_sources})

# 将 Qt 依赖改为 PUBLIC，保证使用方自动继承 QtNetwork 的包含和使用需求
target_link_libraries(FTDAWController PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)

target_compile_definitions(FTDAWController PRIVATE FTDAWController_LIBRARY)