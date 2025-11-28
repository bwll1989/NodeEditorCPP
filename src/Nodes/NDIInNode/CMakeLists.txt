cmake_minimum_required(VERSION 3.10)
set(Module_Name "NDIInNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
# 设置NDI库
set(NDI_DIR "${DEPENDS_DIR}/NDI 6 SDK/include")
set(NDI_LIBRARIES "${DEPENDS_DIR}/NDI 6 SDK/Lib/x64/Processing.NDI.Lib.x64.lib")
INCLUDE_DIRECTORIES(${NDI_DIR})
message("-- Found NDI ${NDI_DIR}")

add_library(${Module_Name}  SHARED
		NDIInDataModel.hpp
		NDIInInterface.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp)

target_link_libraries(${Module_Name}  
		PRIVATE
		Qt${QT_VERSION_MAJOR}::Widgets
		Qt${QT_VERSION_MAJOR}::Network
		${QtNodes_LIBRARIES}
		${OpenCV_LIBRARIES}
		${NDI_LIBRARIES}
		Qt${QT_VERSION_MAJOR}::OpenGL
		OSCTransmitter
		opengl32  # 添加OpenGL32库
		glu32     # 添加GLU32库
		DataTypes)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")