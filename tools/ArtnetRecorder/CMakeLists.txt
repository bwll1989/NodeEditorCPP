# ============================================================================
#  tools/ArtnetRecorder —— Artnet DMX 录制/回放独立工具（tools/ 子项目之一）
# ----------------------------------------------------------------------------
#  由根 CMakeLists.txt 通过 add_subdirectory(tools/ArtnetRecorder) 引入，
#  沿用父工程的 Qt/FFmpeg 探测结果（QT_VERSION_MAJOR、FFMPEG_LIB_DIRS），
#  自身不重复探测第三方库，保持 tools/ 纯净。
#
#  结构说明：
#   - 产物：ArtnetRecorder.exe（WIN32 GUI 程序，不弹控制台）；
#   - res.rc 已并入 PROJECT_SOURCES，Qt5/Qt6 两条分支共用同一份源清单
#     （版本图标/清单资源一致，勿再在某一分支单独追加 .rc 造成重复）；
#   - FFmpeg 以短名（avcodec/avformat…）链接，依赖下方 link_directories
#     指向根工程探测所得 FFMPEG_LIB_DIRS；若迁移到 imported target，
#     需同步替换 target_link_libraries 中的短名并删除 link_directories。
# ============================================================================

# FFmpeg 短名链接的搜索目录（目录级属性，作用于本目录内目标，须在 target 定义前设置）
link_directories("${FFMPEG_LIB_DIRS}")

# 项目源文件（含 .qrc 资源与 .rc 版本资源）
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

# 创建可执行文件（本工程仅面向 Windows 桌面，无 ANDROID/macOS 分支）
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ArtnetRecorder
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(ArtnetRecorder
        ${PROJECT_SOURCES}
    )
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

# 目标属性：Windows GUI 程序
set_target_properties(ArtnetRecorder PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# Qt6 最终化（资源/翻译等收尾）
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ArtnetRecorder)
endif()
