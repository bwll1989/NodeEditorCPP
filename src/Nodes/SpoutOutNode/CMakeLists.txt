cmake_minimum_required(VERSION 3.10)
set(Module_Name "SpoutOutNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
# 自动处理ui文件, 自动处理ui文件是有前提的, 必须在需要编译的头文件(add_executable)中的某个头文件中包含ui_xxx.h(比如#include "ui_model002.h")
set(CMAKE_AUTOMOC ON)
# 自动调用moc(元对象编译器)
set(CMAKE_AUTORCC ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 函数级注释：将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")
# 设置Spout库
#set(Spout_DIR "${DEPENDS_DIR}/Spout2/install")
#set(Spout_LIBRARIES "${Spout_DIR}/lib/Spout.lib")
#set(SpoutLibrary_LIBRARIES "${Spout_DIR}/lib/SpoutLibrary.lib")
#INCLUDE_DIRECTORIES("${Spout_DIR}/include/SpoutGL")
#INCLUDE_DIRECTORIES("${Spout_DIR}/include/SpoutLibrary")
#set(Spout_LIBRARIES "${Spout_DIR}/lib/Spout.lib")
set(SpoutLibrary_LIBRARIES "${DEPENDS_DIR}/Spout2-2.007.017/SPOUTSDK/SpoutLibrary/x64/Release/SpoutLibrary.lib")
INCLUDE_DIRECTORIES("${DEPENDS_DIR}/Spout2-2.007.017/SPOUTSDK/SpoutLibrary")
message("--Found Spout SDK ${Spout_DIR}")

add_library(${Module_Name}  SHARED
		SpoutOutDataModel.hpp
		SpoutOutInterface.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp)

target_link_libraries(${Module_Name}  
		PRIVATE
		Qt${QT_VERSION_MAJOR}::Widgets 
		${QtNodes_LIBRARIES}
		${OpenCV_LIBRARIES}
		${SpoutLibrary_LIBRARIES}
		Qt${QT_VERSION_MAJOR}::OpenGL
		opengl32  # 添加OpenGL32库
		glu32     # 添加GLU32库
		BuildInNodes
		DataTypes)

target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")