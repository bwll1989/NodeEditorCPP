cmake_minimum_required(VERSION 3.10)
set(Module_Name "YoloPoseONNXNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
INCLUDE_DIRECTORIES(${OpenCV_DIR})
set(ONNXRUNTIME_ROOTDIR "${DEPENDS_DIR}/onnxruntime-win-x64-gpu-1.23.0")
include_directories("${ONNXRUNTIME_ROOTDIR}/include") # Windows local install
link_directories("${ONNXRUNTIME_ROOTDIR}/lib")
option(onnxruntime_USE_CUDA "Build with CUDA support" OFF)
add_library(${Module_Name}  SHARED
		YoloPoseONNXDataModel.hpp
		YoloPoseONNXInterface.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp)

target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
		Qt${QT_VERSION_MAJOR}::Concurrent
		${QtNodes_LIBRARIES}
		${OpenCV_LIBRARIES}
		onnxruntime
		onnxruntime_providers_cuda  # 如果使用CUDA
		onnxruntime_providers_shared
)

target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)

SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")