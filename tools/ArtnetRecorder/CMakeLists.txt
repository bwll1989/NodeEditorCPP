# ArtnetRecorder子项目的CMakeLists.txt
# 注意：这里不需要重复设置cmake_minimum_required和project，因为已经在父项目中设置了
link_directories("${FFMPEG_LIB_DIRS}")
# 定义项目源文件
set(PROJECT_SOURCES
        Recorder.cpp
        ArtnetRecorder.cpp
        ArtnetRecorder.h
        ArtnetReceiver.cpp
        ArtnetReceiver.h
        DmxGridView.cpp
        DmxGridView.h
        VideoRecorder.cpp
        VideoRecorder.h
        Defines.h
        resources.qrc
        res.rc
)

# 创建可执行文件
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ArtnetRecorder
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(ArtnetRecorder SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(ArtnetRecorder
            ${PROJECT_SOURCES}
            res.rc
        )
    endif()
endif()

# 链接库
target_link_libraries(ArtnetRecorder PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::OpenGL
        Qt${QT_VERSION_MAJOR}::OpenGLWidgets
        Qt${QT_VERSION_MAJOR}::Network
        GUIElements
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
)

# 设置目标属性
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ArtnetRecorder)
endif()
set_target_properties(ArtnetRecorder PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE  # Windows下设置为TRUE以创建Windows应用程序
)

# 安装配置
include(GNUInstallDirs)
install(TARGETS ArtnetRecorder
    BUNDLE DESTINATION ..
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Qt6最终化
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ArtnetRecorder)
endif()
