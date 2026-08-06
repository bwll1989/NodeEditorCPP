cmake_minimum_required(VERSION 3.10)

set(Module_Name "GUIElements")
project(${Module_Name} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network Gui Qml OpenGL OpenGLWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network Gui Qml OpenGL OpenGLWidgets)
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
    DataBridgeSelectorBox/DataBridgeSelectorBox.cpp
    DataBridgeSelectorBox/DataBridgeSelectorBox.hpp
    ColorEditorWidget/ColorEditorWidget.cpp
    ColorEditorWidget/ColorEditorWidget.hpp
    QGridView/QGridView.cpp
    QGridView/QGridView.h
    IntDragValueWidget/IntDragValueWidget.cpp
    IntDragValueWidget/IntDragValueWidget.hpp
    FloatDragValueWidget/FloatDragValueWidget.cpp
    FloatDragValueWidget/FloatDragValueWidget.hpp
    VectorDragValueWidget/VectorDragValueWidget.cpp
    VectorDragValueWidget/VectorDragValueWidget.hpp
    TriggerWidget/TriggerWidget.cpp
    TriggerWidget/TriggerWidget.hpp
    ImageTextureView/ImageTextureView.cpp
    ImageTextureView/ImageTextureView.hpp
    ImageTextureView/ImageTextureViewHost.cpp
    ImageTextureView/ImageTextureViewHost.hpp
    ImageTextureWindow/ImageTextureWindow.cpp
    ImageTextureWindow/ImageTextureWindow.hpp
)

target_include_directories(${Module_Name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/Common/Gui>
    $<INSTALL_INTERFACE:include/Common/Gui>
)

# 链接Qt库
target_link_libraries(${Module_Name} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    MediaManger
    ModelDataBridge
    DataTypes
    ${QtColorWidgets_LIBRARIES}
)

# 导出符号
target_compile_definitions(${Module_Name} PRIVATE GUI_ELEMENTS_LIBRARY NODE_EDITOR_SHARED)