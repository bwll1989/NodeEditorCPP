cmake_minimum_required(VERSION 3.10)

project(ModelDataBridge LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

set(ModelDataBridge_sources
        ModelDataBridge.cpp
        ModelDataBridge.hpp
        )

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(ModelDataBridge SHARED ${ModelDataBridge_sources})

target_link_libraries(ModelDataBridge PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} OSCTransmitter DataTypes)

target_compile_definitions(ModelDataBridge PRIVATE MODELDATABRIDGE_LIBRARY -DNODE_EDITOR_SHARED)
