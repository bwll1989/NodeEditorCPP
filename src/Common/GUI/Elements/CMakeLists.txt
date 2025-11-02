cmake_minimum_required(VERSION 3.10)

set(Module_Name "GUIElements")
project(${Module_Name} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network Gui Qml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network Gui Qml)
# 创建共享库
add_library(${Module_Name} SHARED
    FaderWidget/FaderWidget.cpp
    FaderWidget/FaderWidget.h
    MartixWidget/MatrixWidget.cpp
    MartixWidget/MatrixWidget.h
    XYPad/XYPad.cpp
    XYPad/XYPad.h
    BarButton/BarButton.cpp
    BarButton/BarButton.h
    FlowWidget/FlowWidget.cpp
    FlowWidget/FlowWidget.h
    SelectorComboBox/SelectorComboBox.cpp
    SelectorComboBox/SelectorComboBox.hpp
)

# 链接Qt库
target_link_libraries(${Module_Name} PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
)

# 导出符号
target_compile_definitions(${Module_Name} PRIVATE GUI_ELEMENTS_LIBRARY)