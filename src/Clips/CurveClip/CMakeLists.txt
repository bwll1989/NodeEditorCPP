cmake_minimum_required(VERSION 3.16)
project(CurveClip VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/TimeLine)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Network Qml)

set(QtTimeLine_DIR "${DEPENDS_DIR}/QtTimeLine")
set(QtTimeLine_LIBRARIES "${QtTimeLine_DIR}/install/lib/QtTimeLine.lib")
include_directories("${QtTimeLine_DIR}/install/include")

set(SOURCES
    curveclipmodel.hpp
    curveclipplugin.hpp
    CurvePath.hpp
    CurvePath.cpp
    CurveViewport.hpp
    CurveViewport.cpp
    ${QtTimeLine_DIR}/install/include/AbstractClipModel.hpp
)

add_library(CurveClip MODULE ${SOURCES})

set_target_properties(CurveClip PROPERTIES
    PREFIX ""
    SUFFIX ".plugin"
)

if(WIN32)
    target_link_libraries(CurveClip PRIVATE ws2_32)
endif()

target_link_libraries(CurveClip PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Network
    Qt6::Qml
    ${QtTimeLine_LIBRARIES}
    GUIElements
    StatusContainer
    BaseClass
    OscListWidget
)

target_include_directories(CurveClip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Widget
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Common/GUI
)

target_compile_definitions(CurveClip PRIVATE
    NODE_TIMELINE_SHARED
    NODE_TIMELINE_EXPORTS
)
