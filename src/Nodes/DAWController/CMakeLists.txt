cmake_minimum_required(VERSION 3.10)
set(Module_Name "DAWController")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

set(PROTOBUF_INSTALL_DIR "${CMAKE_SOURCE_DIR}/3rdParty/protobuf-35.1/install")
list(APPEND CMAKE_PREFIX_PATH "${PROTOBUF_INSTALL_DIR}")
find_package(protobuf CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(utf8_range CONFIG REQUIRED)

add_library(${Module_Name} SHARED
        DAWControllerDataModel.hpp
        DAWControllerInterface.hpp
        CurtainInterface.hpp
        CurtainDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        FTControlBoxDataModel.hpp
        FTControlBoxDataModel.cpp
        FTControlBoxInterface.hpp
        FTControlBoxInterface.cpp
        PeripProtoPacket.h
        PeripProtoPacket.cpp
        PeripProtoTcpServer.h
        PeripProtoTcpServer.cpp
        PeripProtoHandler.h
        PeripProtoHandler.cpp
        PeripProtoServerDataModel.hpp
        PeripProtoServerDataModel.cpp
        perip2s.pb.cc
        perip2s.pb.h
        ../../Common/Devices/TcpClient/TcpClient.h
        ../../Common/Devices/TcpClient/TcpClient.cpp
        ../../Common/Devices/TcpClient/TcpClientWorker.h
        ../../Common/Devices/TcpClient/TcpClientWorker.cpp
)

target_include_directories(${Module_Name} BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/abseil_abi_fix
)
target_include_directories(${Module_Name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROTOBUF_INSTALL_DIR}/include
)

target_link_libraries(${Module_Name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${QtNodes_LIBRARIES}
        Qt${QT_VERSION_MAJOR}::Network
        PropertyTreeWidget
        StatusContainer
        BaseClass
        DataTypes
        FTDAWController
        protobuf::libprotobuf
        absl::abseil_dll
        utf8_range::utf8_validity
        utf8_range::utf8_range
)

target_compile_definitions(${Module_Name} PRIVATE
        UNTITLED_LIBRARY
        NODE_EDITOR_SHARED
        PROTOBUF_USE_DLLS
        ABSL_CONSUME_DLL
)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")

set(PROTOBUF_RUNTIME_DLLS
        "${PROTOBUF_INSTALL_DIR}/bin/libprotobuf.dll"
        "${PROTOBUF_INSTALL_DIR}/bin/abseil_dll.dll"
        "${PROTOBUF_INSTALL_DIR}/bin/libutf8_validity.dll"
        "${PROTOBUF_INSTALL_DIR}/bin/libutf8_range.dll"
)
foreach(_dll IN LISTS PROTOBUF_RUNTIME_DLLS)
    add_custom_command(TARGET ${Module_Name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_dll}"
            "$<TARGET_FILE_DIR:${Module_Name}>"
            COMMENT "Copy protobuf runtime: ${_dll}"
    )
endforeach()

