cmake_minimum_required(VERSION 3.10)
set(Module_Name "JavaScriptNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
# set(LUA_LIBRARIES "${DEPENDS_DIR}/mingw-w64-x86_64/mingw-w64-x86_64-lua-5.4.7-3-any.pkg/mingw64/lib/liblua.a")
set(QScintilla_DIR "${DEPENDS_DIR}/QScintilla_src-2.14.1/src/install" )
set(QScintilla_LIBRARIES "${QScintilla_DIR}/lib/qscintilla2_qt6.lib")
INCLUDE_DIRECTORIES("${QScintilla_DIR}/include")
message("--Found QScintilla  ${QScintilla_DIR}")

set(SOURCE_FILES
        JavaScriptInterface.hpp
        JavaScriptDataModel.hpp
        JavaScriptDataModel.cpp
#        SupportWidgets.hpp
        ../../Common/Devices/JSEngineDefines/JSEngineDefines.hpp
        ../../Common/Devices/JSEngineDefines/SupportWidgets.hpp
        JsCodeEditor.cpp
        JsCodeEditor.h)
add_library(${Module_Name} SHARED
        ${SOURCE_FILES}
        PluginDefinition.hpp
        PluginDefinition.cpp)

#set_target_properties(TextSourceNode PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Qml
        ${QtNodes_LIBRARIES}
        ${QScintilla_LIBRARIES}
        OSCTransmitter
        DataTypes)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")


