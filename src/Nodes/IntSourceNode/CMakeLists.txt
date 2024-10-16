cmake_minimum_required(VERSION 3.5)
set(Module_Name "IntSourceNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)

add_library(${Module_Name} SHARED
        IntSourceDataModel.hpp
        IntSourceInterface.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        ../../Common/GUI/qt-value-slider/intslider.hpp
        ../../Common/GUI/qt-value-slider/intslider.cpp
        ../../Common/GUI/qt-value-slider/valueslider.hpp
        ../../Common/GUI/qt-value-slider/valueslider.cpp)
#set_target_properties(TextSourceNode PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES})
target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES LINKER_LANGUAGE C
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PREFIX ""
        SUFFIX ".node")
