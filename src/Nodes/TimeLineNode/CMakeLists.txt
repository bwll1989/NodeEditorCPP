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

        TimeLineDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        TimeLineNodeClock.cpp
        TimeLineNodeClock.hpp
        TimeLineNodeModel.cpp
        TimeLineNodeModel.h
        TimeLineNodeView.cpp
        TimeLineNodeView.h
        TimeLineNodeWidget.cpp
        TimeLineNodeWidget.hpp
        TrackListNodeView.cpp
        TrackListNodeView.h
        NodeTimeSync.cpp
        NodeTimeSync.hpp
        TimeLineNodeToolbar.cpp
        TimeLineNodeToolbar.h
)

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
                        ${QtNodes_LIBRARIES}
                        ${QtTimeLine_LIBRARIES})

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED NODE_TIMELINE_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")


