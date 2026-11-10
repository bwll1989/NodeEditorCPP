cmake_minimum_required(VERSION 3.10)

set(Module_Name "CurvesNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 函数级注释：将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

# 曲线家族插件（自包含，不引用 TimeLineNode 目录）：
# - Core/        = 曲线家族共享层（独立副本）：CurvePath（向心 Catmull-Rom 数据容器）、
#                  Curve1DCore（1D 采样算法，纯数据层）、Curve1DEditor（函数图编辑器）、
#                  Curve2DEditor（XY 正交编辑器）、CurveViewport（三维 GL 视口）、CurvePalette（8 色板）
# - Curve1D/2D/3D/ = 三个子节点（DataModel + Panel + Dialog），GUI 与数据分离
# - PluginDefinition = 单插件注册三个子节点模型（分类 Curve1D/Curve2D/Curve3D）
add_library(${Module_Name} SHARED
        PluginDefinition.hpp
        PluginDefinition.cpp

        Core/CurvePath.hpp
        Core/CurvePath.cpp
        Core/CurvePalette.hpp
        Core/Curve1DCore.hpp
        Core/Curve1DCore.cpp
        Core/Curve1DEditor.hpp
        Core/Curve1DEditor.cpp
        Core/Curve2DEditor.hpp
        Core/Curve2DEditor.cpp
        Core/CurveViewport.hpp
        Core/CurveViewport.cpp

        Curve1D/Curve1DDataModel.hpp
        Curve1D/Curve1DPanel.hpp
        Curve1D/Curve1DPanel.cpp
        Curve1D/Curve1DDialog.hpp
        Curve1D/Curve1DDialog.cpp

        Curve2D/Curve2DDataModel.hpp
        Curve2D/Curve2DPanel.hpp
        Curve2D/Curve2DPanel.cpp
        Curve2D/Curve2DDialog.hpp
        Curve2D/Curve2DDialog.cpp

        Curve3D/Curve3DDataModel.hpp
        Curve3D/Curve3DPanel.hpp
        Curve3D/Curve3DPanel.cpp
        Curve3D/Curve3DDialog.hpp
        Curve3D/Curve3DDialog.cpp
)

# 裸文件名 include 解析：插件根（PluginDefinition 同级的三个子目录）+ Core
target_include_directories(${Module_Name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Core
)

set_target_properties(${Module_Name} PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
)

target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::OpenGL
        Qt${QT_VERSION_MAJOR}::OpenGLWidgets
        ${QtNodes_LIBRARIES}
        DataTypes
        PropertyTreeWidget
        StatusContainer
        BaseClass
        GUIElements
)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
