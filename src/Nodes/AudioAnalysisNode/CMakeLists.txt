cmake_minimum_required(VERSION 3.10)

set(Module_Name "AudioAnalysisNode")

project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 设置Eigen3库
set(Gist_DIR "${DEPENDS_DIR}/Gist-1.0.7")
INCLUDE_DIRECTORIES(${Gist_DIR}/src)
INCLUDE_DIRECTORIES (${Gist_DIR}/libs/kiss_fft130)
message("-- Found GIST ${Gist_DIR}")
# 添加GIST库源文件
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
        ../../DataTypes/AudioTimestampRingQueue.h
        ../../DataTypes/AudioTimestampRingQueue.cpp
        ${GIST_SOURCES}
        AudioAnalysisInterface.h
        AudioAnalysisInterface.cpp
        AudioAnalysisDataModel.hpp
        AudioAnalysisWorker.hpp
        AudioAnalysisWorker.cpp
        PluginDefinition.cpp
        PluginDefinition.hpp

)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        ${QtNodes_LIBRARIES}
        TimestampGenerator
        GUIElements
        OSCTransmitter
        )

target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED -DUSE_KISS_FFT )

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
