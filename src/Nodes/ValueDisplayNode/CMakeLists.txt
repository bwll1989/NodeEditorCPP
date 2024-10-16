cmake_minimum_required(VERSION 3.5)

set(Module_Name "ValueDisplayNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${Module_Name} SHARED
        ValueDisplayDataModel.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp
        ../../Common/GUI/qt-value-slider/doubleslider.hpp
        ../../Common/GUI/qt-value-slider/doubleslider.cpp
        ../../Common/GUI/qt-value-slider/intslider.hpp
        ../../Common/GUI/qt-value-slider/intslider.cpp
        ../../Common/GUI/qt-value-slider/valueslider.hpp
        ../../Common/GUI/qt-value-slider/valueslider.cpp)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES})
target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES LINKER_LANGUAGE C
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PREFIX ""
        SUFFIX ".node")

