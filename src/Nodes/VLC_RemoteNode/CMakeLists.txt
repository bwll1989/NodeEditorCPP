# VLC Remote 节点插件构建脚本
# 产物：VLC_RemoteNode.node（动态库，由 Flow 节点编辑器加载）

cmake_minimum_required(VERSION 3.10)
set(Module_Name "VLC_RemoteNode")
project(${Module_Name})

set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)

# 插件版本号使用编译日期
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

add_library(${Module_Name} SHARED
        VlcRemoteDataModel.hpp      # 节点核心逻辑
        VlcRemoteInterface.hpp        # 嵌入式 UI
        VlcHttpClient.h               # VLC HTTP 客户端
        PluginDefinition.hpp          # 插件接口声明
        PluginDefinition.cpp          # 插件注册实现
)

target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        Qt${QT_VERSION_MAJOR}::Network
        BuildInNodes
        DataTypes
)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

# 输出到插件目录，后缀 .node
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
