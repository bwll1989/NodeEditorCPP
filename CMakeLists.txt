cmake_minimum_required(VERSION 3.5)

set(ProjectName "NodeStudio")
project(${ProjectName} VERSION 1.1 LANGUAGES CXX)
# set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)

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
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/DataTypes)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/Clips)
add_definitions(-DQT_MESSAGELOGCONTEXT)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(Spdlog_INCLUDE_DIRS "${DEPENDS_DIR}/spdlog/include" )
    set(Spdlog_LIBRARIES "${DEPENDS_DIR}/spdlog/MinGW-64/lib/libspdlog.a")

    set(QtNodes_DIR "${DEPENDS_DIR}/nodeeditor/MinGW-64/lib/cmake/QtNodes")
    find_package(QtNodes REQUIRED)

    set(QHotKey_INCLUDE_DIRS "${DEPENDS_DIR}/QHotkey-master/MinGW-64/include" )
    set(QHotKey_LIBRARIES "${DEPENDS_DIR}/QHotkey-master/MinGW-64/lib/libqhotkey.a")

    set(ADS_INCLUDE_DIRS "${DEPENDS_DIR}/Qt-Advanced-Docking-System/src" )
    set(ADS_LIBRARIES "${DEPENDS_DIR}/Qt-Advanced-Docking-System/MinGW-64/lib/libqtadvanceddocking.a")

    # set(QtAwesome_LIBRARIES "${DEPENDS_DIR}/QtAwesome/MinGW-64/lib/libQtAwesome.a")

    set(QCodeEditor_LIBRARIES "${DEPENDS_DIR}/QCodeEditor/MinGW-64/lib/libQCodeEditor.a")

    set(portaudio_DIR "${DEPENDS_DIR}/portaudio/build/MinGW-64/include")
    set(portaudio_LIBRARIES "${DEPENDS_DIR}/portaudio/build/MinGW-64/lib/libportaudio.dll.a")

    set(Qtpropertybrowser_LIBRARIES "${DEPENDS_DIR}/qtpropertybrowser/MinGW-64/lib/libQt6PropertyBrowser.a")

    set(Libltc_LIBRARIES "${DEPENDS_DIR}/libltc_cmake/MinGW-64/lib/liblibltc.dll.a")

#    set(MLT_DIR "${DEPENDS_DIR}/mingw-w64-x86_64/mingw-w64-x86_64-mlt-7.24.0-4-any.pkg/mingw64/lib/cmake/Mlt7")
#    include_directories(MLT_DIR)
    # find_package(Mlt7 REQUIRED)

#    set(ossia_dir "${DEPENDS_DIR}/ossia/include")

    set(Libpd_INCLUDE_DIRS "${DEPENDS_DIR}/libpd/libpd_wrapper" )
    INCLUDE_DIRECTORIES(${Libpd_INCLUDE_DIRS})
    set(Libpd_LIBRARIES "${DEPENDS_DIR}/libpd/build/libs/liblibpd-multi.dll.a")
    set(PureData_INCLUDE_DIRS "${DEPENDS_DIR}/libpd/pure-data/src")
    INCLUDE_DIRECTORIES(${PureData_INCLUDE_DIRS})

    set(VST3_DIR "${DEPENDS_DIR}/vst3")
    set(VST3_LIBRARIES_DIR "${VST3_DIR}/build/lib")
    #add_subdirectory(${VST3_DIR})
    include_directories(${VST3_DIR})
    link_directories(${VST3_LIBRARIES_DIR})
    

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL") # 或 MultiThreaded
    add_compile_options("/permissive-")
    add_compile_options("/Zc:__cplusplus")
    set(Spdlog_INCLUDE_DIRS "${DEPENDS_DIR}/spdlog/include" )
    set(Spdlog_LIBRARIES "${DEPENDS_DIR}/spdlog/MSVC/lib/spdlog.lib")

    set(QtNodes_DIR "${DEPENDS_DIR}/nodeeditor/MSVC/include")
    set(QtNodes_LIBRARIES "${DEPENDS_DIR}/nodeeditor/MSVC/lib/QtNodes.lib")

    set(QHotKey_INCLUDE_DIRS "${DEPENDS_DIR}/QHotkey-master/MSVC/include" )
    set(QHotKey_LIBRARIES "${DEPENDS_DIR}/QHotkey-master/MSVC/lib/qhotkey.lib")

    set(VST3_DIR "${DEPENDS_DIR}/vst3sdk")
    set(VST3_LIBRARIES_DIR "${VST3_DIR}/MSVC/lib")
    add_subdirectory(${VST3_DIR})
    include_directories(${VST3_DIR})
    link_directories(${VST3_LIBRARIES_DIR})
    add_subdirectory(src/Nodes/VST3PluginNode)

    set(ADS_INCLUDE_DIRS "${DEPENDS_DIR}/Qt-Advanced-Docking-System/src" )
    set(ADS_LIBRARIES "${DEPENDS_DIR}/Qt-Advanced-Docking-System/MSVC/lib/Release/qt6advanceddocking.lib")

    # set(QtAwesome_LIBRARIES "${DEPENDS_DIR}/QtAwesome/MSVC/lib/QtAwesome.lib")

    set(QCodeEditor_LIBRARIES "${DEPENDS_DIR}/QCodeEditor/MSVC/lib/QCodeEditor.lib")

    set(portaudio_DIR "${DEPENDS_DIR}/portaudio/build/MSVC/include")
    set(portaudio_LIBRARIES "${DEPENDS_DIR}/portaudio/build/MSVC/lib/portaudio.lib")

    set(Qtpropertybrowser_LIBRARIES "${DEPENDS_DIR}/qtpropertybrowser/MSVC/lib/Qt6PropertyBrowser.lib")

    set(Libltc_DIR "${DEPENDS_DIR}/libltc_cmake")
    set(Libltc_LIBRARIES "${Libltc_DIR}/MSVC/lib/libltc.lib")

    target_link_libraries(${ProjectName}
            PUBLIC
            sdk
            vstgui_support
            sdk_hosting
            VST3PluginNode)
endif ()

INCLUDE_DIRECTORIES(${Spdlog_INCLUDE_DIRS})
message("--Found spdlog ${Spdlog_LIBRARIES}")


## 查找Libpd库
#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Quick QuickWidgets Widgets Qt6Qml Qt6Quick  Concurrent MultimediaWidgets Multimedia OpenGL OpenGLWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Quick QuickWidgets Widgets Network Concurrent Qml
        QmlWorkerScript
        QuickControls2 MultimediaWidgets Multimedia OpenGL OpenGLWidgets)
#添加QT库
INCLUDE_DIRECTORIES(${QtNodes_DIR})
message("-- Found QtNodes ${QtNodes_DIR}")
#添加QtNodes库
INCLUDE_DIRECTORIES(${QHotKey_INCLUDE_DIRS} )
message("-- Found QHotkey ${QHotKey_INCLUDE_DIRS}")
#添加QHotKey库
INCLUDE_DIRECTORIES(${ADS_INCLUDE_DIRS})
message("-- Found qt6advanceddocking ${ADS_INCLUDE_DIRS}")
#添加ADS
# set(QtAwesome_INCLUDE_DIRS "${DEPENDS_DIR}/QtAwesome")
# INCLUDE_DIRECTORIES(${QtAwesome_INCLUDE_DIRS})
#添加QtAwesome库
#set(FFMPEG_INCLUDE_DIRS "${DEPENDS_DIR}/ffmpeg-n6.1.1-win64-gpl-shared-6.1/include")
#set(FFMPEG_LIB_DIRS "${DEPENDS_DIR}/ffmpeg-n6.1.1-win64-gpl-shared-6.1/lib")
set(FFMPEG_INCLUDE_DIRS "${DEPENDS_DIR}/ffmpeg-n7.1-39-g64e2864cb9-win64-gpl-shared-7.1/include")
set(FFMPEG_LIB_DIRS "${DEPENDS_DIR}/ffmpeg-n7.1-39-g64e2864cb9-win64-gpl-shared-7.1/lib")
include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories("${FFMPEG_LIB_DIRS}")
#

set(QCodeEditor_INCLUDE "${DEPENDS_DIR}/QCodeEditor/include")
INCLUDE_DIRECTORIES(${QCodeEditor_INCLUDE})
message("-- Found QCodeEditor ${QCodeEditor_INCLUDE}")

set(Eigen3_DIR "${DEPENDS_DIR}/eigen-3.4.0/INSTALL/share/eigen3/cmake")
find_package(Eigen3 REQUIRED)
INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})

INCLUDE_DIRECTORIES(${portaudio_DIR})
message("-- Found portaudio ${portaudio_DIR}")

#set(SndFile_DIR "${DEPENDS_DIR}/libsndfile/install/cmake")
#find_package(SndFile REQUIRED)
#message("-- Found SndFile ${SndFile_DIR}")
#
set(OpenCV_DIR "${DEPENDS_DIR}/opencv/build/include")
set(OpenCV_LIBRARIES "${DEPENDS_DIR}/opencv/build/x64/vc16/lib")
INCLUDE_DIRECTORIES(${OpenCV_DIR})
link_directories("${OpenCV_LIBRARIES}")
message("-- Found OpenCV ${OpenCV_DIR}")
include_directories("${DEPENDS_DIR}/qtpropertybrowser/src")
message("-- Found Qtpropertybrowser ${Qtpropertybrowser_DIR}")

set(tinyosc_DIR "${DEPENDS_DIR}/tinyosc")
include_directories(${tinyosc_DIR})
message("-- Found tinyosc ${tinyosc_DIR}")

INCLUDE_DIRECTORIES("${DEPENDS_DIR}/libltc_cmake/src")
message("-- Found libltc ${DEPENDS_DIR}/libltc_cmake/src")


# add_subdirectory(src/Nodes/VST3PluginNode)

#include_directories(${ossia_dir})
#set(Python3_ROOT_DIR "${CMAKE_BINARY_DIR}/Python312" )
#set(PYTHON_EXECUTABLE "${CMAKE_BINARY_DIR}/Python312/python.exe")
#set(pybind11_DIR "${CMAKE_BINARY_DIR}/Python312/Lib/site-packages/pybind11/share/cmake/pybind11")
#set(PYBIND11_FINDPYTHON ON)
#find_package(Python3 3.12.6 COMPONENTS Interpreter Development REQUIRED)
#find_package(pybind11 REQUIRED)
#message("-- Found Python version ${Python3_VERSION}")
#message("-- Found Pybind11 version ${pybind11_VERSION}")
#include_directories(${Python3_INCLUDE_DIRS})
#include_directories(${pybind11_INCLUDE_DIRS})
#分组模块
# 添加Clips模块
add_subdirectory(src/Clips)
# 添加Nodes模块
add_subdirectory(src/Nodes)
set(CALC_SOURCE_FILES
        main.cpp
        3rdParty/tinyosc/tinyosc.c
        3rdParty/tinyosc/tinyosc.h
        src/DataTypes/NodeDataList.hpp
        src/Widget/NodeWidget/CustomGraphicsView.cpp
        src/Widget/NodeWidget/CustomGraphicsView.h
        src/Nodes/NodeRegistry.cpp
        src/Nodes/NodeRegistry.h
        src/Widget/PluginsMangerWidget/PluginsManagerWidget.cpp
        src/Widget/PluginsMangerWidget/PluginsManagerWidget.hpp
        src/Widget/CurveWidget/CurveEditor.cpp
        src/Widget/CurveWidget/CurveEditor.h
        src/Widget/CurveWidget/CurveEditor.ui
        res/res.rc
        res/resources.qrc
        src/Widget/TimeLineWidget/timelineResource.qrc
        src/Widget/NodeWidget/CustomDataFlowGraphModel.h
        src/Widget/NodeWidget/CustomDataFlowGraphModel.cpp
        src/Widget/NodeWidget/CustomFlowGraphicsScene.cpp
        src/Widget/NodeWidget/CustomFlowGraphicsScene.h
        src/Widget/PortEditWidget/PortEditAddRemoveWidget.hpp
        src/Widget/PortEditWidget/PortEditAddRemoveWidget.cpp
        src/Widget/AboutWidget/AboutWidget.hpp
        src/Widget/MainWindow/MainWindow.hpp
        src/Widget/MainWindow/MainWindow.cpp
        src/Widget/CurveWidget/CurveEditor.cpp
        src/Widget/CurveWidget/CurveEditor.h
        src/Widget/CurveWidget/CurveEditor.ui
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
        src/Widget/PortEditWidget/PortEditWidget.cpp
        src/Widget/PortEditWidget/PortEditWidget.h
        src/Widget/NodeLibraryWidget/draggabletreewidget.cpp
        src/Widget/NodeLibraryWidget/draggabletreewidget.hpp
        src/Widget/NodeLibraryWidget/NodeLibraryWidget.cpp
        src/Widget/NodeLibraryWidget/NodeLibraryWidget.h
        src/Widget/NodeListWidget/NodeListWidget.cpp
        src/Widget/NodeListWidget/NodeListWidget.hpp
        src/Common/GUI/QJsonModel/QJsonModel.cpp
        src/Common/GUI/QJsonModel/QJsonModel.hpp
        src/Common/GUI/QJsonModel/details/QUtf8.hpp
        src/Common/GUI/qt-value-slider/doubleslider.cpp
        src/Common/GUI/qt-value-slider/doubleslider.hpp
        src/Common/GUI/qt-value-slider/intslider.cpp
        src/Common/GUI/qt-value-slider/intslider.hpp
        src/Common/GUI/qt-value-slider/valueslider.cpp
        src/Common/GUI/qt-value-slider/valueslider.hpp
        src/Common/GUI/Elements/XYPad/XYPad.cpp
        src/Common/GUI/Elements/XYPad/XYPad.h
        src/Common/GUI/Elements/BarButton/BarButton.cpp
        src/Common/GUI/Elements/BarButton/BarButton.h
        src/Common/GUI/Elements/FlowWidget/FlowWidget.cpp
        src/Common/GUI/Elements/FlowWidget/FlowWidget.h
        src/Common/GUI/Elements/MartixWidget/MatrixWidget.cpp
        src/Common/GUI/Elements/MartixWidget/MatrixWidget.h
        src/Common/GUI/Elements/FaderWidget/FaderWidget.cpp
        src/Common/GUI/Elements/FaderWidget/FaderWidget.h
        src/Common/Devices/TcpServer/TcpServer.cpp
        src/Common/Devices/TcpServer/TcpServer.h
        src/Common/Devices/TcpClient/TcpClient.cpp
        src/Common/Devices/TcpClient/TcpClient.h
        src/Common/Devices/UdpSocket/UdpSocket.cpp
        src/Common/Devices/UdpSocket/UdpSocket.h
        src/Common/Devices/AudioPipe/AudioPipe.cpp
        src/Common/Devices/AudioPipe/AudioPipe.h
        src/Common/Devices/OSCReceiver/OSCReceiver.cpp
        src/Common/Devices/OSCReceiver/OSCReceiver.h
        src/Common/Devices/OSCSender/OSCSender.cpp
        src/Common/Devices/OSCSender/OSCSender.h
        src/Common/Devices/ArtnetReceiver/ArtnetReceiver.cpp
        src/Common/Devices/ArtnetReceiver/ArtnetReceiver.h
        src/DataTypes/DecimalData.h
        src/DataTypes/TextData.h
        src/DataTypes/FloatData.h
        src/DataTypes/VariantData.h
        src/DataTypes/AudioData2.h
        src/DataTypes/PixmapData.h
        src/DataTypes/AudioData.h
        src/Common/GUI/QPropertyBrowser/QPropertyBrowser.cpp
        src/Common/GUI/QPropertyBrowser/QPropertyBrowser.h
        src/Widget/TimeLineWidget/videoplayerwidget.hpp
        src/Widget/TimeLineWidget/TimelineAbstract/AbstractClipDelegate.hpp
        src/Widget/TimeLineWidget/TimelineAbstract/AbstractClipModel.cpp
        src/Widget/TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp
        src/Widget/TimeLineWidget/TimelineAbstract/AbstractClipInterface.hpp
        src/Widget/TimeLineWidget/TimelineMVC/pluginloader.cpp
        src/Widget/TimeLineWidget/TimelineMVC/pluginloader.hpp
        src/Widget/TimeLineWidget/TimelineMVC/timelinemodel.cpp
        src/Widget/TimeLineWidget/TimelineMVC/timelinemodel.hpp
        src/Widget/TimeLineWidget/TimelineMVC/timelinestyle.hpp
        src/Widget/TimeLineWidget/timelinetypes.h
        src/Widget/TimeLineWidget/TimelineMVC/timelineview.cpp
        src/Widget/TimeLineWidget/TimelineMVC/timelineview.hpp
        src/Widget/TimeLineWidget/timelinewidget.cpp
        src/Widget/TimeLineWidget/timelinewidget.hpp
        src/Widget/TimeLineWidget/TimelineMVC/trackdelegate.hpp
        src/Widget/TimeLineWidget/TimelineMVC/tracklistview.cpp
        src/Widget/TimeLineWidget/TimelineMVC/tracklistview.hpp
        src/Widget/TimeLineWidget/TimelineMVC/trackmodel.hpp
        src/Widget/TimeLineWidget/videoplayerwidget.cpp
        src/Widget/TimeLineWidget/ClipPropertyWidget/clipproperty.hpp
        src/Widget/TimeLineWidget/ClipPropertyWidget/clipproperty.cpp
        src/Widget/TimeLineWidget/timelinetoolbar.hpp
        src/Widget/TimeLineWidget/timelinetoolbar.cpp
        src/Widget/TimeLineWidget/TimelineSettingWidget/timelinesettingsdialog.hpp
        src/Widget/TimeLineWidget/TimelineSettingWidget/timelinesettingsdialog.cpp
        src/Widget/StageWidget/StageWidget.hpp
        src/Widget/StageWidget/StageWidget.cpp
        src/Widget/TimeLineWidget/TimelineAbstract/AbstractTimelineScreen.hpp
        src/Widget/TimeLineWidget/TimelineStageWidget/timelinestage.hpp
        src/Widget/TimeLineWidget/TimelineStageWidget/timelinestage.cpp
        src/Widget/TimeLineWidget/TimelineScreenWidget/timelinescreen.hpp
        src/Widget/TimeLineWidget/TimelineScreenWidget/timelinescreen.cpp
        src/Widget/TimeLineWidget/TimelineMVC/timecodegenerator.hpp
        src/Widget/TimeLineWidget/TimelineMVC/timecodegenerator.cpp
        src/Widget/TimeLineWidget/timelineimageproducer.hpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${ProjectName}
            MANUAL_FINALIZATION
            ${CALC_SOURCE_FILES}
    )

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ${ProjectName} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(${ProjectName} SHARED
                ${CALC_SOURCE_FILES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(${ProjectName}
                ${CALC_SOURCE_FILES}
                res/res.rc
        )
    endif()
endif()

set(QRC_FILES
        resources.qrc
        timelineResource.qrc
        )
target_compile_definitions(${ProjectName} PRIVATE -DNODE_EDITOR_SHARED)
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
            Opengl32
            glu32
            ${portaudio_LIBRARIES}
            ${QtNodes_LIBRARIES}
            ${Spdlog_LIBRARIES}
            ${ADS_LIBRARIES}
            ${Libpd_LIBRARIES}
            ${QHotKey_LIBRARIES}
            ${QCodeEditor_LIBRARIES}
            ${Qtpropertybrowser_LIBRARIES}
            ${Libltc_LIBRARIES}
            Eigen3::Eigen

)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.

#if(${QT_VERSION} VERSION_LESS 6.1.0)
#  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${ProjectName})
#endif()
set_target_properties(${ProjectName} PROPERTIES
    ${BUNDLE_ID_OPTION}
#    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
#    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
#    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "${ProjectName}"

)

include(GNUInstallDirs)
install(TARGETS ${ProjectName}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${ProjectName})
endif()
if(WIN32)
    # Windows 下的处理逻辑
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
elseif(UNIX)

endif()
