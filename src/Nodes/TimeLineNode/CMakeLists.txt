cmake_minimum_required(VERSION 3.5)
set(Module_Name "TimeLineNode")
project(${Module_Name})
set(CMAKE_CXX_STANDARD 17)
set(MAKE_CXX_STANDARD_REQUIRED ON)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 函数级注释：将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPLUGIN_VERSION="${BUILD_TS}")

# ----------------------------------------------------------------------------
# 内置时间线引擎（原 QtTimeLine 与 6 种 clip 已打散融入本节点，并合并为单头/单实现）
#
# 设计要点：
#  - 与主 timeline（原 QtTimeLine 体系 + plugins/TimeLine/*.plugin）完全隔离：
#    主 timeline 后续升级不影响本节点功能。
#  - TimelineEngine.h/.cpp 由原引擎的 18 个头 + 9 个实现合并而成，含
#    Model/View/TrackList/Delegate/Toolbar、clip 基类（TimeLineClipModel，原
#    AbstractClipModel——2026-11-13 改名以与 BaseClass.dll 的同名基类区分）、
#    剪辑工厂接口、代理基类（NodeClipDelegateModel）与两个属性面板。
#  - 以 NODE_TIMELINE_STATIC 编译，NODE_TIMELINE_PUBLIC 展开为空，
#    不依赖旧 QtTimeLine 库（dllimport）。
#  - 资源前缀 :/tlnode/icons（tlnode_resources.qrc，prefix "tlnode"），
#    与旧 QtTimeLine 的 :/icons 资源树隔离；静态资源需在
#    TimeLineDataModel 构造函数中 Q_INIT_RESOURCE(tlnode_resources)。
#  - clip 不走 QPluginLoader：BasePluginLoader::loadPlugins() 静态注册
#    内置的 6 个剪辑工厂（实现位于 TimelineEngine.cpp）。
#  - 剪辑代理基类 NodeClipDelegateModel 继承本节点内的 TimeLineClipModel，
#    绝不与 BaseClass.dll 的 AbstractClipModel 混用（继承链不同，
#    混用必崩）。
# ----------------------------------------------------------------------------

set(TL_ENGINE_HEADERS
        TimelineEngine.h
        # 内置 6 种 clip（静态注册，不再作为外部插件加载）
        artnetclipmodel.hpp
        artnetclipplugin.hpp
        curveclipmodel.hpp
        curveclipplugin.hpp
        CurvePath.hpp
        CurveViewport.hpp
        imageclipmodel.hpp
        imageclipplugin.hpp
        mappingclipmodel.hpp
        mappingclipplugin.hpp
        triggerclipmodel.hpp
        triggerclipplugin.hpp
        videoclipmodel.hpp
        videoclipplugin.hpp
)

set(TL_ENGINE_SOURCES
        TimelineEngine.cpp
        artnetclipmodel.cpp
        CurvePath.cpp
        CurveViewport.cpp
        tlnode_resources.qrc
)

add_library(${Module_Name} SHARED
        TimeLineDataModel.hpp
        PluginDefinition.hpp
        PluginDefinition.cpp
        TimeLineNodeClock.cpp
        TimeLineNodeClock.hpp
        TimeLineNodeModel.cpp
        TimeLineNodeModel.h
        TimeLineNodeView.cpp
        TimeLineNodeView.h
        TimeLineNodeWidget.cpp
        TimeLineNodeWidget.hpp
        TrackListNodeView.cpp
        TrackListNodeView.h
        NodeTimeSync.cpp
        NodeTimeSync.hpp
        TimeLineNodeToolbar.cpp
        TimeLineNodeToolbar.h
        TimelineInterface.hpp
        ${TL_ENGINE_HEADERS}
        ${TL_ENGINE_SOURCES}
)

set_target_properties(${Module_Name} PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
)

target_link_libraries(${Module_Name} PRIVATE
                        Qt${QT_VERSION_MAJOR}::Core
                        Qt${QT_VERSION_MAJOR}::Gui
                        Qt${QT_VERSION_MAJOR}::Widgets
                        Qt${QT_VERSION_MAJOR}::Network
                        Qt${QT_VERSION_MAJOR}::OpenGL
                        Qt${QT_VERSION_MAJOR}::OpenGLWidgets
                        Qt${QT_VERSION_MAJOR}::Qml
                        Qt${QT_VERSION_MAJOR}::WebSockets
                        ${QtNodes_LIBRARIES}
                        DataTypes
                        PropertyTreeWidget
                        StatusContainer
                        BaseClass
                        # 内置 clip 依赖的主工程组件（公共基础库，非隔离范围）
                        GUIElements
                        OscListWidget
                        OSCTransmitter
                        ClientController
                        MediaManger
                        AppConfig
                        ArtnetService
)

# VideoClip/ArtnetClip 需要 FFmpeg（avformat/avcodec/avutil），用全路径避免依赖 link_directories
set(TL_FFMPEG_LIB_DIR "${DEPENDS_DIR}/ffmpeg-8.1.1-full_build-shared/lib")
target_link_libraries(${Module_Name} PRIVATE
    "${TL_FFMPEG_LIB_DIR}/avformat.lib"
    "${TL_FFMPEG_LIB_DIR}/avcodec.lib"
    "${TL_FFMPEG_LIB_DIR}/avutil.lib"
)

# 内置引擎以静态方式编入本节点（副本头 NODE_TIMELINE_PUBLIC 为空），
# 不定义 NODE_TIMELINE_SHARED，也不链接旧 QtTimeLine 库。
target_compile_definitions(${Module_Name} PRIVATE UNTITLED_LIBRARY -DNODE_EDITOR_SHARED NODE_TIMELINE_STATIC)
SET_TARGET_PROPERTIES(${Module_Name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIRECTORY}"
        PREFIX ""
        SUFFIX ".node")
