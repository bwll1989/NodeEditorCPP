cmake_minimum_required(VERSION 3.10)
set(Module_Name "PythonScriptNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# set(LUA_LIBRARIES "${DEPENDS_DIR}/mingw-w64-x86_64/mingw-w64-x86_64-lua-5.4.7-3-any.pkg/mingw64/lib/liblua.a")
set(QScintilla_DIR "${DEPENDS_DIR}/QScintilla_src-2.14.1/src/install" )
set(QScintilla_LIBRARIES "${QScintilla_DIR}/lib/qscintilla2_qt6.lib")
INCLUDE_DIRECTORIES("${QScintilla_DIR}/include")
message("--Found QScintilla  ${QScintilla_DIR}")

set(Python3_ROOT_DIR "${DEPENDS_DIR}/Python")
set(Python3_EXECUTABLE "${Python3_ROOT_DIR}/python3.14t.exe")
set(Python3_LIBRARY "${Python3_ROOT_DIR}/libs/python314t.lib")
set(Python3_INCLUDE_DIR "${Python3_ROOT_DIR}/include")
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Found Python3: ${Python3_EXECUTABLE} (version: ${Python3_VERSION})")
include_directories(${Python3_INCLUDE_DIRS})
list(APPEND CMAKE_PREFIX_PATH "${Python3_ROOT_DIR}/Lib/site-packages/pybind11/share/cmake/pybind11")
find_package(pybind11 REQUIRED)
# find_package(Lua REQUIRED)${Python3_LIBRARIES}

set(SOURCE_FILES
        PythonScriptInterface.hpp
        PythonScriptDataModel.hpp
        PythonScriptDataModel.cpp
        SupportWidgets.hpp
        PythonWorkerThread.hpp
        PythonWorkerThread.cpp
        PythonCodeEditor.cpp
        PythonCodeEditor.h
        PythonEngineDefines.h)
add_library(${Module_Name} SHARED
        ${SOURCE_FILES}
        PluginDefinition.hpp
        PluginDefinition.cpp)
target_include_directories(${Module_Name} PRIVATE ${pybind11_INCLUDE_DIRS})
#set_target_properties(TextSourceNode PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
        Qt${QT_VERSION_MAJOR}::Qml
        Qt${QT_VERSION_MAJOR}::Network
        ${QtNodes_LIBRARIES}
        OSCTransmitter
        ${QScintilla_LIBRARIES}
        ${Python3_LIBRARIES}
        pybind11::module
        pybind11::lto
        pybind11::windows_extras
        pybind11::embed)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY
        -DNODE_EDITOR_SHARED
        PYBIND11_HAS_SUBINTERPRETER_SUPPORT=1
        PY_GIL_DISABLED=1  # 注意：这里改为1，表示禁用GIL
        Py_NOGIL=1         # 启用无GIL模式
)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")


