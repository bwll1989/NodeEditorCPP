cmake_minimum_required(VERSION 3.10)
set(Module_Name "USR_IO808Node")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 函数级注释：将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
add_library(${Module_Name} SHARED
        PluginDefinition.hpp
        PluginDefinition.cpp
        USR-IO808DataModel.hpp
        USR-IO808DataModel.cpp
        USR-IO808Interface.hpp
        USR-IO808Interface.cpp
         ../../Common/Devices/TcpClient/TcpClient.h
        ../../Common/Devices/TcpClient/TcpClient.cpp
        ../../Common/Devices/TcpClient/TcpClientWorker.h
       ../../Common/Devices/TcpClient/TcpClientWorker.cpp)

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
        Qt${QT_VERSION_MAJOR}::Qml
        ${QtNodes_LIBRARIES}
        Qt${QT_VERSION_MAJOR}::Network
        OSCTransmitter)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

# 在编译完成后复制JS脚本文件到插件输出目录
#add_custom_command(
#        TARGET ${Module_Name}
#        POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#                ${CMAKE_CURRENT_SOURCE_DIR}/USR-IO808.js
#                ${PLUGIN_OUTPUT_DIRECTORY}/USR-IO808.js
#        COMMENT "Copying USR-IO808.js to plugin output directory"
#)




