cmake_minimum_required(VERSION 3.10)
set(Module_Name "GDTFNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

# GDTF = zip 容器（minizip + zlib）+ description.xml（tinyxml2）。
# 直接把 Scalable Display Reference Implementation/ThirdParty 的源码编进静态库（BSD），
# 与 MpcdiNode 相同模式，避免预编译库的 CRT / 编译器 ABI 绑定。
set(GDTF_THIRDPARTY_ROOT "${DEPENDS_DIR}/Scalable Display Reference Implementation/ThirdParty")
set(ZLIB_SRC "${GDTF_THIRDPARTY_ROOT}/zlib-1.2.7")
set(MINIZIP_SRC "${ZLIB_SRC}/contrib/minizip")
set(TINYXML_SRC "${GDTF_THIRDPARTY_ROOT}/tinyxml2")

if(NOT EXISTS "${TINYXML_SRC}/tinyxml2.cpp")
    message(FATAL_ERROR "GDTF third-party sources not found at ${GDTF_THIRDPARTY_ROOT}")
endif()

# zlib 源码树不带 zconf.h（构建期由 configure 生成），拷贝自带的 zconf.h.included
set(ZLIB_GEN "${CMAKE_CURRENT_BINARY_DIR}/zlib-include")
file(MAKE_DIRECTORY "${ZLIB_GEN}")
configure_file("${ZLIB_SRC}/zconf.h.included" "${ZLIB_GEN}/zconf.h" COPYONLY)

set(ZLIB_C
    adler32.c crc32.c inflate.c inftrees.c inffast.c uncompr.c zutil.c
)
set(MINIZIP_C ioapi.c unzip.c)

set(GDTF_THIRD_PARTY_SRC)
foreach(src ${ZLIB_C})
    list(APPEND GDTF_THIRD_PARTY_SRC "${ZLIB_SRC}/${src}")
endforeach()
foreach(src ${MINIZIP_C})
    list(APPEND GDTF_THIRD_PARTY_SRC "${MINIZIP_SRC}/${src}")
endforeach()
list(APPEND GDTF_THIRD_PARTY_SRC "${TINYXML_SRC}/tinyxml2.cpp")

add_library(gdtf_thirdparty STATIC ${GDTF_THIRD_PARTY_SRC})
target_include_directories(gdtf_thirdparty
    PUBLIC
        "${ZLIB_GEN}"
        "${ZLIB_SRC}"
        "${MINIZIP_SRC}"
        "${TINYXML_SRC}"
)
target_compile_definitions(gdtf_thirdparty PRIVATE _CRT_SECURE_NO_WARNINGS)
set_target_properties(gdtf_thirdparty PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
if(MSVC)
    target_compile_options(gdtf_thirdparty PRIVATE /w)
endif()

add_library(${Module_Name} SHARED
    GDTFParser.hpp
    GDTFParser.cpp
    GDTFInterface.hpp
    GDTFDataModel.hpp
    PluginDefinition.hpp
    PluginDefinition.cpp
)

target_link_libraries(${Module_Name} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    ${QtNodes_LIBRARIES}
    gdtf_thirdparty
    DataTypes
    GUIElements
    AppConfig
    BaseClass
    StatusContainer
)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

set_target_properties(${Module_Name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    PREFIX ""
    SUFFIX ".node"
)
