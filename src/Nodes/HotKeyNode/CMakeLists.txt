cmake_minimum_required(VERSION 3.10)

set(Module_Name "HotKeyNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
# 设置QHotKey库
set(QHotKey_INCLUDE_DIRS "${DEPENDS_DIR}/QHotkey-master/MSVC/include" )
set(QHotKey_LIBRARIES "${DEPENDS_DIR}/QHotkey-master/MSVC/lib/qhotkey.lib")
INCLUDE_DIRECTORIES(${QHotKey_INCLUDE_DIRS} )
message("-- Found QHotkey ${QHotKey_INCLUDE_DIRS}")

add_library(${Module_Name} SHARED
        HotKeyDataModel.hpp
        HotKeyInterface.hpp
        HotKeyItem.hpp
        PluginDefinition.cpp
        PluginDefinition.hpp)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} ${QHotKey_LIBRARIES})
target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
