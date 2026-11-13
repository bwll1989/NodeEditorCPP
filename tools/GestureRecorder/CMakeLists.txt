# ============================================================================
#  tools/GestureRecorder —— 手势库录制工具（MediaPipe 手部关键点 → .gesture 模板）
# ----------------------------------------------------------------------------
#  由根 CMakeLists.txt 通过 add_subdirectory(tools/GestureRecorder) 引入，
#  沿用父工程的 Qt/OpenCV 探测结果，自身不重复探测第三方库，保持 tools/ 纯净。
#  产物：GestureRecorder.exe（WIN32 GUI 程序）；录制的 .gesture 动作库由
#  src/Nodes/MediaPipe/GestureNode（手势模板匹配节点）加载使用。
# ============================================================================
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
