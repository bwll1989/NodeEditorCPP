cmake_minimum_required(VERSION 3.10)

project(AppConfig LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

set(AppConfig_sources
        ConfigManager.cpp
        ConfigManager.h
        LicenseManager.cpp
        LicenseManager.h
        ConstantDefines.h
        ProjectPersistence.cpp
        ProjectPersistence.h
        AutosaveManager.cpp
        AutosaveManager.h
)

#target_link_libraries(MYLIBRARY_LIBRARY PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

add_library(AppConfig SHARED ${AppConfig_sources})

target_include_directories(AppConfig PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/Common/AppConfig>
)

# 将 Qt 依赖改为 PUBLIC，保证使用方自动继承 QtNetwork 的包含和使用需求
target_link_libraries(AppConfig PUBLIC Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(AppConfig PRIVATE APPCONFIG_LIBRARY)

set(LICENSE_ICON_PATH "${CMAKE_SOURCE_DIR}/res/icons/license.ico")
string(REPLACE "\\" "/" LICENSE_ICON_PATH "${LICENSE_ICON_PATH}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/LicenseGenerator.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/LicenseGenerator.rc"
    @ONLY
)

add_executable(LicenseGenerator
    LicenseGenerator.cpp
    LicenseGenerator.qrc
    "${CMAKE_CURRENT_BINARY_DIR}/LicenseGenerator.rc"
)
target_link_libraries(LicenseGenerator PRIVATE AppConfig Qt${QT_VERSION_MAJOR}::Widgets)
set_target_properties(LicenseGenerator PROPERTIES
    OUTPUT_NAME "LicenseGenerator"
    WIN32_EXECUTABLE TRUE
)
