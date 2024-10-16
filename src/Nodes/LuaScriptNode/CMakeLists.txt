cmake_minimum_required(VERSION 3.5)
set(Module_Name "LuaScriptNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC On)
set(CMAKE_AUTORCC ON)
#添加QtNodes库目录
set(QCodeEditor_INCLUDE "${DEPENDS_DIR}/QCodeEditor/include")
INCLUDE_DIRECTORIES(${QCodeEditor_INCLUDE})
set(QCodeEditor_LIBRARIES "${DEPENDS_DIR}/QCodeEditor/build/libQCodeEditor.a")
message("-- Found QCodeEditor ${QCodeEditor_INCLUDE}")

set(LuaBridge_INCLUDE "${DEPENDS_DIR}/LuaBridge-2.8/Source")
INCLUDE_DIRECTORIES(${LuaBridge_INCLUDE})
message("-- Found LuaBridge ${LuaBridge_INCLUDE}")
#set(Lua_DIR "${DEPENDS_DIR}/Lua/install/share/cmake/Lua" )
set(LUA_INCLUDE_DIR "${DEPENDS_DIR}/Lua/install/include")
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})
set(LUA_LIBRARIES "${DEPENDS_DIR}/Lua/install/bin/liblua54.dll")
# find_package(Lua REQUIRED)
message("-- Found Lua ${Lua_DIR}")

set(SOURCE_FILES "./resources/resources.qrc"
        LuaThread.cpp
        LuaThread.h
        LuaScriptInterface.hpp
        LuaDataModel.hpp
        CodeEditor.cpp
        CodeEditor.h)
add_library(${Module_Name} SHARED
        ${SOURCE_FILES}

        PluginDefinition.hpp
        PluginDefinition.cpp)

#set_target_properties(TextSourceNode PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(${Module_Name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${QtNodes_LIBRARIES} ${LUA_LIBRARIES} ${QCodeEditor_LIBRARIES} )

target_compile_definitions(${Module_Name}  PRIVATE UNTITLED_LIBRARY)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES LINKER_LANGUAGE C
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PREFIX ""
        SUFFIX ".node")


