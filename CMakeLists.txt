cmake_minimum_required(VERSION 3.10)
# 标准程序
set(ProjectName "NodeStudio")
# Headless 程序
set(HeadlessProjectName ${ProjectName}Actuator)
project(${ProjectName} VERSION 1.1 LANGUAGES CXX)
# set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
string(TIMESTAMP BUILD_TS "%Y%m%d")
# 函数级注释：将版本号以宏形式注入到编译器（目录作用域，兼容 3.10）
add_definitions(-DPRODUCT_VERSION="${BUILD_TS}")
# 设置所有目标的默认输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# 设置插件的输出目录
set(PLUGIN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/DataFlow")

# 确保插件目录存在
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_DIRECTORY})

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(C)
set(DEPENDS_DIR "${CMAKE_SOURCE_DIR}/3rdParty")
set(CMAKE_PREFIX_PATH ${DEPENDS_DIR})

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/Common/Gui)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/Common/Devices)
#INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/DataTypes)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/Clips)
add_definitions(-DQT_MESSAGELOGCONTEXT)

# 设置Qt MSVC版本的路径
set(CMAKE_PREFIX_PATH "C:/Qt/6.5.3/msvc2019_64") # 请根据实际安装路径修改此处
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL") # 或 MultiThreaded
# 设置编译选项
add_compile_options("/permissive-")
add_compile_options("/Zc:__cplusplus")
add_compile_options("/utf-8")
# 设置Spdlog库
set(Spdlog_INCLUDE_DIRS "${DEPENDS_DIR}/spdlog/include" )
set(Spdlog_LIBRARIES "${DEPENDS_DIR}/spdlog/MSVC/lib/spdlog.lib")
INCLUDE_DIRECTORIES(${Spdlog_INCLUDE_DIRS})
message("--Found spdlog ${Spdlog_LIBRARIES}")

# 设置QtNodes库
set(QtNodes_DIR "${DEPENDS_DIR}/nodeeditor/MSVC/include")
set(QtNodes_LIBRARIES "${DEPENDS_DIR}/nodeeditor/MSVC/lib/QtNodes.lib")
INCLUDE_DIRECTORIES(${QtNodes_DIR})
message("-- Found QtNodes ${QtNodes_DIR}")


# 设置ADS库
set(ADS_INCLUDE_DIRS "${DEPENDS_DIR}/Qt-Advanced-Docking-System/src" )
set(ADS_LIBRARIES "${DEPENDS_DIR}/Qt-Advanced-Docking-System/MSVC/lib/qtadvanceddocking-qt6.lib")
INCLUDE_DIRECTORIES(${ADS_INCLUDE_DIRS})
message("-- Found qt6advanceddocking ${ADS_INCLUDE_DIRS}")

# 设置portaudio库
set(portaudio_DIR "${DEPENDS_DIR}/portaudio/include")
set(portaudio_LIBRARIES "${DEPENDS_DIR}/portaudio/build/MSVC/lib/portaudio.lib")
INCLUDE_DIRECTORIES(${portaudio_DIR})
message("-- Found portaudio ${portaudio_DIR}")

# 设置libltc库
set(Libltc_DIR "${DEPENDS_DIR}/libltc_cmake")
set(Libltc_LIBRARIES "${Libltc_DIR}/MSVC/lib/libltc.lib")
INCLUDE_DIRECTORIES("${DEPENDS_DIR}/libltc_cmake/src")
message("-- Found libltc ${DEPENDS_DIR}/libltc_cmake/src")

# 设置tinyosc库
set(tinyosc_DIR "${DEPENDS_DIR}/tinyosc-msvc")
include_directories(${tinyosc_DIR})
message("-- Found tinyosc ${tinyosc_DIR}")

# 设置FFMPEG库
set(FFMPEG_INCLUDE_DIRS "${DEPENDS_DIR}/ffmpeg-n7.1-184-gdc07f98934-win64-gpl-shared-7.1/include")
set(FFMPEG_LIB_DIRS "${DEPENDS_DIR}/ffmpeg-n7.1-184-gdc07f98934-win64-gpl-shared-7.1/lib")
include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories("${FFMPEG_LIB_DIRS}")
message("-- Found ffmpeg ${FFMPEG_INCLUDE_DIRS}")

# 设置Eigen3库
set(Eigen3_DIR "${DEPENDS_DIR}/eigen-3.4.0/INSTALL/share/eigen3/cmake")
find_package(Eigen3 REQUIRED)
INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})
message("-- Found Eigen3 ${EIGEN3_INCLUDE_DIR}")


# 设置OpenCV库
set(OpenCV_DIR "${DEPENDS_DIR}/opencv/build")
#set(OpenCV_LIBRARIES "${DEPENDS_DIR}/opencv/build/x64/vc16/lib")
find_package(OpenCV REQUIRED)	# 找到opencv库
include_directories(${OpenCV_INCLUDE_DIRS})
message("-- Found OpenCV ${OpenCV_DIR}")

# 设置QtTimeLine库
set(QtTimeLine_DIR "${DEPENDS_DIR}/QtTimeLine")
set(QtTimeLine_LIBRARIES "${QtTimeLine_DIR}/install/lib/QtTimeLine.lib")

INCLUDE_DIRECTORIES("${QtTimeLine_DIR}/install/include")

## 设置TabToolbar库
#set(TabToolbar_DIR "${DEPENDS_DIR}/TabToolbar")
#set(TabToolbar_LIBRARIES "${TabToolbar_DIR}/install/bin/TabToolbar.lib")
#INCLUDE_DIRECTORIES("${TabToolbar_DIR}/include")

# 设置QT库
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui  QuickWidgets Widgets Qt6Qml Qt6Quick Concurrent MultimediaWidgets Multimedia OpenGL OpenGLWidgets WebSockets Graphs)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui  QuickWidgets Widgets Network Concurrent Qml
        QmlWorkerScript
        QuickControls2 MultimediaWidgets Multimedia OpenGL OpenGLWidgets WebSockets Graphs)
#分组模块
# 添加Clips模块
add_subdirectory(src/Clips)
# 添加Nodes模块
add_subdirectory(src/Nodes)
add_subdirectory(src/Common/Devices/OSCSender)
add_subdirectory(src/Common/GUI/OscListWidget)
add_subdirectory(src/Common/GUI/Elements)
add_subdirectory(src/Common/Devices/ClientController)
add_subdirectory(src/Common/Devices/ArtnetSender)
add_subdirectory(src/Common/Devices/TimestampGenerator)
add_subdirectory(src/Common/Devices/MediaLibrary)
add_subdirectory(src/Common/Devices/ModelDataBridge)
add_subdirectory(src/Common/GUI/DelayListWidget)
add_subdirectory(src/Common/BuildInNodes)
add_subdirectory(src/DataTypes)
set(CALC_SOURCE_FILES
        main.cpp
        3rdParty/tinyosc-msvc/tinyosc.c
        3rdParty/tinyosc-msvc/tinyosc.h
        res/resources.qrc
        src/Widget/NodeWidget/CustomGraphicsView.cpp
        src/Widget/NodeWidget/CustomGraphicsView.h
        src/Widget/PluginsMangerWidget/PluginsManagerWidget.cpp
        src/Widget/PluginsMangerWidget/PluginsManagerWidget.hpp
        src/Widget/TimeLineWidget/TimeLineResource.qrc
        src/Widget/NodeWidget/CustomDataFlowGraphModel.h
        src/Widget/NodeWidget/CustomDataFlowGraphModel.cpp
        src/Widget/NodeWidget/CustomFlowGraphicsScene.cpp
        src/Widget/NodeWidget/CustomFlowGraphicsScene.h
        src/Widget/NodeWidget/DataflowViewsManger.hpp
        src/Widget/NodeWidget/DataflowViewsManger.cpp
        src/Widget/PortEditWidget/PortEditAddRemoveWidget.hpp
        src/Widget/PortEditWidget/PortEditAddRemoveWidget.cpp
        src/Widget/AboutWidget/AboutWidget.hpp
        src/Widget/MainWindow/MainWindow.hpp
        src/Widget/MainWindow/MainWindow.cpp
        src/Widget/MainWindow/MainWindowHeadLess.cpp
        src/Widget/MainWindow/MainWindowHeadLess.hpp
        src/Widget/SplashWidget/CustomSplashScreen.cpp
        src/Widget/SplashWidget/CustomSplashScreen.hpp
        src/Widget/ExternalControl/ExternalControler.cpp
        src/Widget/ExternalControl/ExternalControler.hpp
        src/Widget/ConsoleWidget/LogWidget.hpp
        src/Widget/ConsoleWidget/LogWidget.cpp
        src/Widget/ConsoleWidget/LogHandler.cpp
        src/Widget/ConsoleWidget/LogHandler.hpp
        src/Widget/MenuBarWidget/MenuBarWidget.cpp
        src/Widget/MenuBarWidget/MenuBarWidget.h
        src/Widget/PropertyWidget/PropertyWidget.cpp
        src/Widget/PropertyWidget/PropertyWidget.hpp
        src/Widget/NodeLibraryWidget/draggabletreewidget.cpp
        src/Widget/NodeLibraryWidget/draggabletreewidget.hpp
        src/Widget/NodeLibraryWidget/NodeLibraryWidget.cpp
        src/Widget/NodeLibraryWidget/NodeLibraryWidget.h
        src/Widget/NodeListWidget/NodeListWidget.cpp
        src/Widget/NodeListWidget/NodeListWidget.hpp
        src/Common/Devices/TcpServer/TcpServer.cpp
        src/Common/Devices/TcpServer/TcpServer.h
        src/Common/Devices/TcpClient/TcpClient.cpp
        src/Common/Devices/TcpClient/TcpClient.h
        src/Common/Devices/TcpClient/TcpClientWorker.h
        src/Common/Devices/TcpClient/TcpClientWorker.cpp
        src/Common/Devices/UdpSocket/UdpSocket.cpp
        src/Common/Devices/UdpSocket/UdpSocket.h
        src/Common/Devices/WebSocketServer/WebSocketServer.cpp
        src/Common/Devices/WebSocketServer/WebSocketServer.h
        src/Common/Devices/WebSocketClient/WebSocketClient.cpp
        src/Common/Devices/WebSocketClient/WebSocketClient.h
        src/Common/Devices/WebSocketClient/WebSocketClientWorker.h
        src/Common/Devices/WebSocketClient/WebSocketClientWorker.cpp
        src/Common/Devices/OSCReceiver/OSCReceiver.cpp
        src/Common/Devices/OSCReceiver/OSCReceiver.h
        src/Common/Devices/ArtnetReceiver/ArtnetReceiver.cpp
        src/Common/Devices/ArtnetReceiver/ArtnetReceiver.h
        src/Common/Devices/LTCDecoder/LTCDecoder.cpp
        src/Common/Devices/LTCDecoder/LTCDecoder.h
        src/Common/Devices/LtcReceiver/LtcReceiver.cpp
        src/Common/Devices/LtcReceiver/LtcReceiver.h
        src/Common/Devices/JSEngineDefines/JSEngineDefines.hpp
        src/Common/Devices/JSEngineDefines/SupportWidgets.hpp
        src/Common/GUI/DataTreeModel/DataTreeModel.h
        src/Common/GUI/DataTreeModel/DataTreeModel.cpp
        src/Common/GUI/DataTreeModel/QmlDataBrowser.h
        src/Common/GUI/DataTreeModel/QmlDataBrowser.cpp
        src/Widget/TimeLineWidget/timelinewidget.cpp
        src/Widget/TimeLineWidget/timelinewidget.hpp
        src/Widget/TimeLineWidget/TimelineScreenWidget/AbstractTimelineScreen.hpp
        src/Widget/TimeLineWidget/TimelineSettingWidget/LtcSettingWidget.hpp
        src/Widget/TimeLineWidget/TimelineSettingWidget/MtcSettingWidget.hpp
        src/Widget/TimeLineWidget/TimelineSettingWidget/AudioSettingWidget.hpp
        src/Widget/TimeLineWidget/TimelineSettingWidget/VideoSettingWidget.hpp
        src/Widget/TimeLineWidget/TimelineSettingWidget/TimeLineSetting.hpp
        src/Widget/TimeLineWidget/TimelineSettingWidget/TimeLineSetting.cpp
        src/Widget/StageWidget/stagewidget.hpp
        src/Widget/StageWidget/stagewidget.cpp
        src/Widget/TimeLineWidget/TimelineStageWidget/TimeLineStage.hpp
        src/Widget/TimeLineWidget/TimelineStageWidget/TimeLineStage.cpp
        src/Widget/TimeLineWidget/TimelineScreenWidget/TimeLineScreen.hpp
        src/Widget/TimeLineWidget/TimelineScreenWidget/TimeLineScreen.cpp
        src/Widget/TimeLineWidget/TimeLineClock/TimeLineClock.hpp
        src/Widget/TimeLineWidget/TimeLineClock/TimeLineClock.cpp
        src/Widget/TimeLineWidget/TimeLineClock/TimeSyncServer.hpp
        src/Widget/TimeLineWidget/TimeLineClock/TimeSyncServer.cpp
        src/Widget/TimeLineWidget/TimelineProducer/timelineimageproducer.hpp
        src/Widget/TimeLineWidget/TimeLineModel.cpp
        src/Widget/TimeLineWidget/TimeLineModel.h
        src/Widget/TimeLineWidget/TimeLineView.cpp
        src/Widget/TimeLineWidget/TimeLineView.h
        src/Widget/TimeLineWidget/TrackListView.cpp
        src/Widget/TimeLineWidget/TrackListView.h
        src/Widget/CalendarWidget/CalendarWidget.cpp
        src/Widget/CalendarWidget/CalendarWidget.hpp
        src/Widget/CalendarWidget/ScheduledTaskWidget.cpp
        src/Widget/CalendarWidget/ScheduledTaskWidget.hpp
        src/Widget/CalendarWidget/ScheduledTaskModel.cpp
        src/Widget/CalendarWidget/ScheduledTaskModel.hpp
        src/Widget/CalendarWidget/TaskItemWidget.cpp
        src/Widget/CalendarWidget/TaskItemWidget.hpp
        src/Widget/CalendarWidget/TaskListWidget.cpp
        src/Widget/CalendarWidget/TaskListWidget.hpp
        src/Widget/CalendarWidget/ScheduledTaskFilterProxyModel.cpp
        src/Widget/CalendarWidget/ScheduledTaskFilterProxyModel.hpp
        src/Widget/CalendarWidget/ScheduledTaskManager.cpp
        src/Widget/CalendarWidget/ScheduledTaskManager.hpp
        src/Widget/CalendarWidget/TaskItemDelegate.cpp
        src/Widget/CalendarWidget/TaskItemDelegate.h
        src/Widget/MediaLibraryWidget/MediaLibraryWidget.cpp
        src/Widget/MediaLibraryWidget/MediaLibraryWidget.h
        src/constantDefines.h
)
# 函数级注释：定义 Headless 版本源文件（用 headless_main.cpp 替换 main.cpp）
set(HEADLESS_SOURCE_FILES ${CALC_SOURCE_FILES})
list(REMOVE_ITEM HEADLESS_SOURCE_FILES main.cpp)
list(APPEND HEADLESS_SOURCE_FILES headless_main.cpp)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${ProjectName}
            MANUAL_FINALIZATION
            ${CALC_SOURCE_FILES}
            res/res.rc
    )
    # 函数级注释：Qt6 下添加 Headless 可执行目标
    qt_add_executable(${HeadlessProjectName}
            MANUAL_FINALIZATION
            ${HEADLESS_SOURCE_FILES}
            res/headless.rc
    )
else()
    if(ANDROID)
        add_library(${ProjectName} SHARED
                ${CALC_SOURCE_FILES}
        )
    else()
        add_executable(${ProjectName}
                ${CALC_SOURCE_FILES}
                res/res.rc
        )
        # 函数级注释：Qt5/非 Android 下添加 Headless 可执行目标
        add_executable(${HeadlessProjectName}
                ${HEADLESS_SOURCE_FILES}
                res/headless.rc
        )
    endif()
endif()

set(QRC_FILES
        resources.qrc
        timelineResource.qrc
        )
target_compile_definitions(${ProjectName} PRIVATE -DNODE_EDITOR_SHARED NODE_TIMELINE_SHARED)
target_compile_definitions(${ProjectName} PRIVATE CLIENTCONTROLLER_LIBRARY)
target_compile_definitions(${ProjectName} PRIVATE OSCLISTWIDGET_LIBRARY)
#target_compile_definitions(${ProjectName} PRIVATE OSCTRANSMITTER_LIBRARY)
target_link_libraries(${ProjectName}
        PUBLIC
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Gui
            Qt${QT_VERSION_MAJOR}::Core
            Qt${QT_VERSION_MAJOR}::Qml
            Qt${QT_VERSION_MAJOR}::QmlWorkerScript
            Qt${QT_VERSION_MAJOR}::QuickControls2
            Qt${QT_VERSION_MAJOR}::Quick
            Qt${QT_VERSION_MAJOR}::QuickWidgets
            Qt${QT_VERSION_MAJOR}::Network
            Qt${QT_VERSION_MAJOR}::Concurrent
            Qt${QT_VERSION_MAJOR}::Multimedia
            Qt${QT_VERSION_MAJOR}::MultimediaWidgets
            Qt${QT_VERSION_MAJOR}::OpenGL
            Qt${QT_VERSION_MAJOR}::OpenGLWidgets
            Qt${QT_VERSION_MAJOR}::WebSockets
            Qt${QT_VERSION_MAJOR}::Graphs
            GUIElements
            avcodec
            avdevice
            avfilter
            avformat
            avutil
            postproc
            swresample
            swscale
            Opengl32
            glu32
            OSCTransmitter
            DataTypes
            BuildInNodes
            ${portaudio_LIBRARIES}
            ${QtNodes_LIBRARIES}
            ${Spdlog_LIBRARIES}
            ${ADS_LIBRARIES}
            ${Libpd_LIBRARIES}
            ${Libltc_LIBRARIES}
            Eigen3::Eigen
            ${QtTimeLine_LIBRARIES}
            ${OpenCV_LIBRARIES}
            ClientController
            OscListWidget
            TimestampGenerator
            MediaManger
            ModelDataBridge
            DelayListWidget
)

# 为 Headless 目标定义编译宏（与主程序一致）
target_compile_definitions(${HeadlessProjectName} PRIVATE -DNODE_EDITOR_SHARED NODE_TIMELINE_SHARED)
target_compile_definitions(${HeadlessProjectName} PRIVATE CLIENTCONTROLLER_LIBRARY)
target_compile_definitions(${HeadlessProjectName} PRIVATE OSCLISTWIDGET_LIBRARY)
# 为 Headless 目标链接与主程序一致的库（含 FFmpeg 7.1）
target_link_libraries(${HeadlessProjectName}
        PUBLIC
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Gui
            Qt${QT_VERSION_MAJOR}::Core
            Qt${QT_VERSION_MAJOR}::Qml
            Qt${QT_VERSION_MAJOR}::QmlWorkerScript
            Qt${QT_VERSION_MAJOR}::QuickControls2
            Qt${QT_VERSION_MAJOR}::Quick
            Qt${QT_VERSION_MAJOR}::QuickWidgets
            Qt${QT_VERSION_MAJOR}::Network
            Qt${QT_VERSION_MAJOR}::Concurrent
            Qt${QT_VERSION_MAJOR}::Multimedia
            Qt${QT_VERSION_MAJOR}::MultimediaWidgets
            Qt${QT_VERSION_MAJOR}::OpenGL
            Qt${QT_VERSION_MAJOR}::OpenGLWidgets
            Qt${QT_VERSION_MAJOR}::WebSockets
            Qt${QT_VERSION_MAJOR}::Graphs
            GUIElements
            avcodec
            avdevice
            avfilter
            avformat
            avutil
            postproc
            swresample
            swscale
            Opengl32
            glu32
            OSCTransmitter
            DataTypes
            BuildInNodes
            ${portaudio_LIBRARIES}
            ${QtNodes_LIBRARIES}
            ${Spdlog_LIBRARIES}
            ${ADS_LIBRARIES}
            ${Libpd_LIBRARIES}
            ${Libltc_LIBRARIES}
            Eigen3::Eigen
            ${QtTimeLine_LIBRARIES}
            ${OpenCV_LIBRARIES}
            ClientController
            OscListWidget
            TimestampGenerator
            MediaManger
            ModelDataBridge
            DelayListWidget
)
# 设置主程序属性
set_target_properties(${ProjectName} PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "${ProjectName}"
)
# 设置 Headless 目标属性与输出名，与主程序一致
set_target_properties(${HeadlessProjectName} PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME ${HeadlessProjectName}
)

install(TARGETS ${ProjectName}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# 函数级注释：安装 Headless 目标到同一目录
install(TARGETS ${HeadlessProjectName}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${ProjectName})
    # 函数级注释：Qt6 下完成 Headless 目标的终结步骤
    qt_finalize_executable(${HeadlessProjectName})
endif()
if(WIN32)
    # Windows 下的处理逻辑
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
elseif(UNIX)

endif()
