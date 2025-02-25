cmake_minimum_required(VERSION 3.5)
set(Module_Name "PythonNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(PYBIND11_FINDPYTHON ON)
set(QCodeEditor_INCLUDE "${DEPENDS_DIR}/QCodeEditor/include")
INCLUDE_DIRECTORIES(${QCodeEditor_INCLUDE})
set(QCodeEditor_LIBRARIES "${DEPENDS_DIR}/QCodeEditor/build/libQCodeEditor.a")
message("-- Found QCodeEditor ${QCodeEditor_INCLUDE}")
set(PLUGIN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/DataFlow)



set(PYBIND11_PYTHON_VERSION 3.12)
set(Python3_ROOT_DIR "${CMAKE_BINARY_DIR}/Python312" )
set(PYTHON_EXECUTABLE "${CMAKE_BINARY_DIR}/Python312/python.exe")
set(pybind11_DIR "${CMAKE_BINARY_DIR}/Python312/Lib/site-packages/pybind11/share/cmake/pybind11")
#find_package(Python COMPONENTS Interpreter Development)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
add_subdirectory(pybind11)
#find_package(pybind11 REQUIRED)
message("-- Found Python version ${Python3_VERSION}")
#find_package(pybind11 CONFIG REQUIRED)
message("-- Found Pybind11 version ${pybind11_VERSION}")

include_directories(${Python3_INCLUDE_DIRS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core )

add_library(${Module_Name} SHARED
        PythonNodeModel.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        PythonScriptInterface.hpp
        CodeEditor.cpp
        CodeEditor.hpp

)



target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} Python3::Python ${QCodeEditor_LIBRARIES}  pybind11::embed)
# Add pybind11 module
#pybind11_add_module(PythonDataModel myQtModule.hpp)
#target_link_libraries(qtmodule PRIVATE  Qt${QT_VERSION_MAJOR}::Core)
#add_subdirectory(pybind11)
pybind11_add_module(example example.hpp)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES LINKER_LANGUAGE C
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PREFIX "")
