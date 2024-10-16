cmake_minimum_required(VERSION 3.5)
set(Module_Name "TimeLineNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)

#set(QRC_FILES
#        ./resources/resource.qrc
#)
add_library(${Module_Name} SHARED
#
        src/timelinemodel.hpp
        src/trackmodel.hpp
        src/clipmodel.hpp
        src/sequencemodel.hpp
        src/timelineview.hpp
        src/clipdelegate.hpp
        src/tracklistview.hpp
        src/trackdelegate.hpp
        src/timelinetypes.h
        src/timelinewidget.hpp
        TimeLineDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        )

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} )

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES LINKER_LANGUAGE C
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PREFIX ""
        SUFFIX ".node")


