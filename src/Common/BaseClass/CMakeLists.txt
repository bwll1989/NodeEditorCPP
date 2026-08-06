cmake_minimum_required(VERSION 3.10)

project(BaseClass LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

set(BaseClass_sources
        BaseClassExport.h
        AbstractDelegateModel.h
        AbstractDelegateModel.cpp
        AbstractClipDelegateModel.h
        AbstractClipDelegateModel.cpp
)

add_library(BaseClass SHARED ${BaseClass_sources})

target_include_directories(BaseClass
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/Common/Devices>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/Common/Gui>
        $<INSTALL_INTERFACE:include>
        $<INSTALL_INTERFACE:include/Common/Devices>
        $<INSTALL_INTERFACE:include/Common/Gui>
)

target_link_libraries(BaseClass
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        QtNodes::QtNodes
        QtTimeLine::QtTimeLine
        StatusContainer
        PropertyTreeWidget
)

target_compile_definitions(BaseClass
    PRIVATE
        BASECLASS_LIBRARY
        NODE_EDITOR_SHARED
        NODE_TIMELINE_SHARED
)
