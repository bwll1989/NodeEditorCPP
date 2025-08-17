cmake_minimum_required(VERSION 3.10)
set(Module_Name "CustomScriptNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 移除 QScintilla 相关配置（代码编辑器不再需要）
# set(QScintilla_DIR "${DEPENDS_DIR}/QScintilla_src-2.14.1/src/install" )
# set(QScintilla_LIBRARIES "${QScintilla_DIR}/lib/qscintilla2_qt6.lib")
# INCLUDE_DIRECTORIES("${QScintilla_DIR}/include")
# message("--Found QScintilla  ${QScintilla_DIR}")

set(SOURCE_FILES
        CustomScriptInterface.hpp
        CustomScriptInterface.cpp
        CustomScriptDataModel.hpp
        CustomScriptDataModel.cpp
        JSPluginManager.hpp
        JSPluginManager.cpp
        SupportWidgets.hpp
        ../../Common/Devices/JSEngineDefines/JSEngineDefines.hpp
        # 移除 JsCodeEditor 相关文件
        # JsCodeEditor.cpp
        # JsCodeEditor.h
        )

add_library(${Module_Name} SHARED
        ${SOURCE_FILES}
        PluginDefinition.hpp
        PluginDefinition.cpp)

# 移除 QScintilla 库链接
target_link_libraries(${Module_Name} PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Concurrent 
    Qt${QT_VERSION_MAJOR}::Qml 
    ${QtNodes_LIBRARIES}
    # ${QScintilla_LIBRARIES}  # 移除代码编辑器库
    )

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")


