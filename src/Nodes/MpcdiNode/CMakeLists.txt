cmake_minimum_required(VERSION 3.10)
set(Module_Name "MpcdiNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

# Scalable Display MPCDI 1.0 reader (BSD). 编进静态库，避免和主程序的 CRT / 编译器 ABI 绑在预编译 DLL 上。
set(MPCDI_ROOT "${DEPENDS_DIR}/Scalable Display Reference Implementation")
set(MPCDI_SRC "${MPCDI_ROOT}/src")
set(ZLIB_SRC "${MPCDI_ROOT}/ThirdParty/zlib-1.2.7")
set(PNG_SRC "${MPCDI_ROOT}/ThirdParty/lpng161")
set(MINIZIP_SRC "${ZLIB_SRC}/contrib/minizip")
set(TINYXML_SRC "${MPCDI_ROOT}/ThirdParty/tinyxml2")

if(NOT EXISTS "${MPCDI_SRC}/IO/mpcdiReader.h")
    message(FATAL_ERROR "MPCDI reference implementation not found at ${MPCDI_ROOT}")
endif()

set(ZLIB_GEN "${CMAKE_CURRENT_BINARY_DIR}/zlib-include")
file(MAKE_DIRECTORY "${ZLIB_GEN}")
configure_file("${ZLIB_SRC}/zconf.h.included" "${ZLIB_GEN}/zconf.h" COPYONLY)

file(GLOB MPCDI_CPP
    "${MPCDI_SRC}/Base/*.cpp"
    "${MPCDI_SRC}/Container/*.cpp"
    "${MPCDI_SRC}/IO/*.cpp"
    "${MPCDI_SRC}/Utils/*.cpp"
)

set(ZLIB_C
    adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c
    infback.c inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c
)
set(PNG_C
    png.c pngerror.c pngget.c pngmem.c pngpread.c pngread.c pngrio.c
    pngrtran.c pngrutil.c pngset.c pngtrans.c pngwio.c pngwrite.c pngwtran.c pngwutil.c
)
set(MINIZIP_C ioapi.c unzip.c zip.c)

set(MPCDI_THIRD_PARTY)
foreach(src ${ZLIB_C})
    list(APPEND MPCDI_THIRD_PARTY "${ZLIB_SRC}/${src}")
endforeach()
foreach(src ${PNG_C})
    list(APPEND MPCDI_THIRD_PARTY "${PNG_SRC}/${src}")
endforeach()
foreach(src ${MINIZIP_C})
    list(APPEND MPCDI_THIRD_PARTY "${MINIZIP_SRC}/${src}")
endforeach()
list(APPEND MPCDI_THIRD_PARTY "${TINYXML_SRC}/tinyxml2.cpp")

add_library(scalable_mpcdi STATIC ${MPCDI_CPP} ${MPCDI_THIRD_PARTY})
target_include_directories(scalable_mpcdi
    PUBLIC
        "${MPCDI_SRC}/Base"
        "${MPCDI_SRC}/Container"
        "${MPCDI_SRC}/IO"
        "${MPCDI_SRC}/Utils"
    PRIVATE
        "${ZLIB_GEN}"
        "${ZLIB_SRC}"
        "${PNG_SRC}"
        "${MINIZIP_SRC}"
        "${TINYXML_SRC}"
)
target_compile_definitions(scalable_mpcdi
    PUBLIC MPCDI_STATIC
    PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS
)
set_target_properties(scalable_mpcdi PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
if(MSVC)
    target_compile_options(scalable_mpcdi PRIVATE /w /permissive)
endif()

add_library(${Module_Name} SHARED
    MpcdiWarper.hpp
    MpcdiWarper.cpp
    MpcdiInterface.hpp
    MpcdiDataModel.hpp
    PluginDefinition.hpp
    PluginDefinition.cpp
)

target_link_libraries(${Module_Name} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::OpenGL
    ${QtNodes_LIBRARIES}
    scalable_mpcdi
    DataTypes
    StatusContainer
    BaseClass
    TimestampGenerator
    GUIElements
    MediaManger
    AppConfig
    opengl32
)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

set_target_properties(${Module_Name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    PREFIX ""
    SUFFIX ".node"
)
