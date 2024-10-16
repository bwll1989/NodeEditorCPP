cmake_minimum_required(VERSION 3.5)
set(Module_Name "ScriptEngine")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
pybind11_add_module(ScriptEngine ScriptInterface.cpp)

#target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES}  Python3::Python ${QCodeEditor_LIBRARIES} pybind11::embed pybind11::module)
# Add pybind11 module
#pybind11_add_module(PythonDataModel myQtModule.hpp)
#target_link_libraries(qtmodule PRIVATE  Qt${QT_VERSION_MAJOR}::Core)


SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        OUTPUT_NAME ${Module_Name}
        SUFFIX ".pyd")

