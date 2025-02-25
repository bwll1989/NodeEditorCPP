cmake_minimum_required(VERSION 3.5)
set(Module_Name "TimeLineNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(PLUGIN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/DataFlow)
#set(QRC_FILES
#        ./resources/resource.qrc
#)
add_library(${Module_Name} SHARED
#
        ../../Common/GUI/Timeline/timelinemodel.hpp
        ../../Common/GUI/Timeline/trackmodel.hpp
        ../../Common/GUI/Timeline/clipmodel.hpp
        ../../Common/GUI/Timeline/sequencemodel.hpp
        ../../Common/GUI/Timeline/timelineview.hpp
        ../../Common/GUI/Timeline/clipdelegate.hpp
        ../../Common/GUI/Timeline/tracklistview.hpp
        ../../Common/GUI/Timeline/trackdelegate.hpp
        ../../Common/GUI/Timeline/timelinetypes.h
        ../../Common/GUI/Timeline/timelinewidget.hpp
        TimeLineDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        )

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} )

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")


