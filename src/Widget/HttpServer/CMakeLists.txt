cmake_minimum_required(VERSION 3.10)
project(HttpServer LANGUAGES CXX)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)
add_library(HttpServer STATIC
    HttpServer.cpp
    HttpServer.hpp
)

target_include_directories(HttpServer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HttpServer
    PUBLIC
        Poco::Net
        Poco::Util
        Poco::Foundation
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
)

