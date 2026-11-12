cmake_minimum_required(VERSION 3.10)

set(Module_Name "GestureRecorder")
project(${Module_Name} LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(GESTURE_RECORDER_SOURCES
        main.cpp
        GestureFeature.hpp
        GestureLibrary.hpp
        MediaPipeHandEngine.hpp
        PalmAnchors.inc
        recorder.qrc
        app.rc
)

add_executable(${Module_Name} ${GESTURE_RECORDER_SOURCES})

target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        ${OpenCV_LIBRARIES}
        GUIElements
)

set_target_properties(${Module_Name} PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME GestureRecorder
)
