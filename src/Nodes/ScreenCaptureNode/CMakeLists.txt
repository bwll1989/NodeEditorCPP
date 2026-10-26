cmake_minimum_required(VERSION 3.10)

set(Module_Name "ScreenCaptureNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

add_library(${Module_Name} SHARED
        ScreenCapture.h
        ScreenCapture.cpp
        ScreenCaptureModel.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp)

# Windows SDK 自带的 C++/WinRT 头，Ninja 生成器不一定把它加进默认 INCLUDE
file(GLOB _sdk_cppwinrt "C:/Program Files (x86)/Windows Kits/10/Include/*/cppwinrt")
if(_sdk_cppwinrt)
    list(SORT _sdk_cppwinrt)
    list(REVERSE _sdk_cppwinrt)
    list(GET _sdk_cppwinrt 0 _cppwinrt_dir)
    target_include_directories(${Module_Name} PRIVATE "${_cppwinrt_dir}")
    get_filename_component(_sdk_version "${_cppwinrt_dir}" DIRECTORY)
    get_filename_component(_sdk_version "${_sdk_version}" NAME)
    set(_sdk_um_lib "C:/Program Files (x86)/Windows Kits/10/Lib/${_sdk_version}/um/x64")
    if(EXISTS "${_sdk_um_lib}")
        target_link_directories(${Module_Name} PRIVATE "${_sdk_um_lib}")
    endif()
else()
    message(WARNING "ScreenCaptureNode: cppwinrt headers not found; Windows Graphics Capture will not compile")
endif()

target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        ${OpenCV_LIBRARIES}
        d3d11
        dxgi
        dwmapi
        windowsapp
        DataTypes
        GUIElements
        ${Qtpropertybrowser_LIBRARIES}
        PropertyTreeWidget
        StatusContainer
        BaseClass
        TimestampGenerator
)

target_compile_definitions(${Module_Name} PRIVATE
        UNTITLED_LIBRARY
        -DNODE_EDITOR_SHARED
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

if(MSVC)
    target_compile_options(${Module_Name} PRIVATE /bigobj /EHsc)
endif()

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
