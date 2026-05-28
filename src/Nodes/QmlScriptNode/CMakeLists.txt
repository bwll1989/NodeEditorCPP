cmake_minimum_required(VERSION 3.10)
set(Module_Name "QmlScriptNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
set(QScintilla_DIR "${DEPENDS_DIR}/QScintilla_src-2.14.1/src/install" )
set(QScintilla_LIBRARIES "${QScintilla_DIR}/lib/qscintilla2_qt6.lib")
INCLUDE_DIRECTORIES("${QScintilla_DIR}/include")
message("--Found QScintilla  ${QScintilla_DIR}")

set(SOURCE_FILES
        QmlScriptDataModel.hpp
        QmlScriptDataModel.cpp
        QmlScriptRuntime.h
        QmlScriptRuntime.cpp
        QmlScriptView.h
        QmlScriptView.cpp
        ../../Common/Devices/JSEngineDefines/JSEngineDefines.hpp
        JsCodeEditor.cpp
        JsCodeEditor.h
        ./res.qrc)

add_library(${Module_Name} SHARED
        ${SOURCE_FILES}
        PluginDefinition.hpp
        PluginDefinition.cpp)

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Qml
        Qt${QT_VERSION_MAJOR}::Quick
        Qt${QT_VERSION_MAJOR}::QuickWidgets
        ${QtNodes_LIBRARIES}
        ${QScintilla_LIBRARIES}
        BuildInNodes
        DataTypes)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
