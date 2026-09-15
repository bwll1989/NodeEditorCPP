cmake_minimum_required(VERSION 3.10)
set(Module_Name "ISFNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

# 第三方：https://github.com/mrRay/VVISF-GL → 3rdParty/VVISF-GL
set(VVISF_GL_ROOT "${DEPENDS_DIR}/VVISF-GL")
if(NOT EXISTS "${VVISF_GL_ROOT}/VVISF/include/VVISF.hpp")
    message(FATAL_ERROR "VVISF-GL not found at ${VVISF_GL_ROOT}. Clone https://github.com/mrRay/VVISF-GL into 3rdParty/VVISF-GL")
endif()

if(NOT TARGET VVGL)
    add_subdirectory("${VVISF_GL_ROOT}" "${CMAKE_BINARY_DIR}/3rdParty/VVISF-GL")
endif()

# audioFFT：与 AudioAnalysisNode 相同，嵌入 Gist + kissfft
set(Gist_DIR "${DEPENDS_DIR}/Gist-1.0.7")
if(NOT EXISTS "${Gist_DIR}/src/Gist.h")
    message(FATAL_ERROR "Gist not found at ${Gist_DIR}")
endif()
message("-- ISFNode: Found GIST ${Gist_DIR}")
set(GIST_SOURCES
    ${Gist_DIR}/src/Gist.cpp
    ${Gist_DIR}/src/CoreFrequencyDomainFeatures.cpp
    ${Gist_DIR}/src/CoreTimeDomainFeatures.cpp
    ${Gist_DIR}/src/MFCC.cpp
    ${Gist_DIR}/src/OnsetDetectionFunction.cpp
    ${Gist_DIR}/src/WindowFunctions.cpp
    ${Gist_DIR}/src/Yin.cpp
    ${Gist_DIR}/libs/kiss_fft130/kiss_fft.c
)

add_library(${Module_Name} SHARED
    ${GIST_SOURCES}
    ISFRenderer.hpp
    ISFRenderer.cpp
    ISFAudioCook.hpp
    ISFInterface.hpp
    ISFDataModel.hpp
    PluginDefinition.hpp
    PluginDefinition.cpp
)

target_include_directories(${Module_Name} PRIVATE
    "${VVISF_GL_ROOT}/VVGL/include"
    "${VVISF_GL_ROOT}/VVISF/include"
    "${VVISF_GL_ROOT}/external/GLEW/include"
    "${Gist_DIR}/src"
    "${Gist_DIR}/libs/kiss_fft130"
)

target_link_libraries(${Module_Name} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::OpenGL
    ${QtNodes_LIBRARIES}
    VVGL
    VVISF
    DataTypes
    StatusContainer
    BaseClass
    TimestampGenerator
    GUIElements
    MediaManger
    AppConfig
    opengl32
)

target_compile_definitions(${Module_Name} PRIVATE
    UNTITLED_LIBRARY
    -DNODE_EDITOR_SHARED
    -DUSE_KISS_FFT
)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
    PREFIX ""
    SUFFIX ".node")
