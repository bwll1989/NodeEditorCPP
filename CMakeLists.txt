cmake_minimum_required(VERSION 3.5)
#-DCMAKE_TOOLCHAIN_FILE=C:/Users/bwll1/scoop/apps/vcpkg/current/scripts/buildsystems/vcpkg.cmake
#CLION构建选项，下面set不生效时可以使用
#set(CMAKE_TOOLCHAIN_FILE "C:/Users/bwll1/scoop/apps/vcpkg/2024.07.12/scripts/buildsystems/vcpkg.cmake"
#        CACHE STRING "Vcpkg toolchain file")
##设置VCPKG
project(NodeEditorCPP VERSION 1.1 LANGUAGES CXX)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_language(C)
set(DEPENDS_DIR "${CMAKE_SOURCE_DIR}/3rdParty")
set(CMAKE_PREFIX_PATH ${DEPENDS_DIR})
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}")
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/Common/Gui)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/Common/Devices)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/DataTypes)
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

    set(QtAwesome_LIBRARIES "${DEPENDS_DIR}/QtAwesome/MinGW-64/lib/libQtAwesome.a")

    set(QCodeEditor_LIBRARIES "${DEPENDS_DIR}/QCodeEditor/MinGW-64/lib/libQCodeEditor.a")

    set(portaudio_DIR "${DEPENDS_DIR}/portaudio/build/MinGW-64/include")
    set(portaudio_LIBRARIES "${DEPENDS_DIR}/portaudio/build/MinGW-64/lib/libportaudio.dll.a")

    set(Qtpropertybrowser_LIBRARIES "${DEPENDS_DIR}/qtpropertybrowser/MinGW-64/lib/libQt6PropertyBrowser.a")

    set(Libltc_LIBRARIES "${DEPENDS_DIR}/libltc_cmake/MinGW-64/lib/liblibltc.dll.a")

#    set(MLT_DIR "${DEPENDS_DIR}/mlt/lib/cmake/Mlt7")
#    find_package(Mlt7 REQUIRED)
#    set(ossia_dir "${DEPENDS_DIR}/ossia/include")

    set(Libpd_INCLUDE_DIRS "${DEPENDS_DIR}/libpd/libpd_wrapper" )
    INCLUDE_DIRECTORIES(${Libpd_INCLUDE_DIRS})
    set(Libpd_LIBRARIES "${DEPENDS_DIR}/libpd/build/libs/liblibpd-multi.dll.a")
    set(PureData_INCLUDE_DIRS "${DEPENDS_DIR}/libpd/pure-data/src")
    INCLUDE_DIRECTORIES(${PureData_INCLUDE_DIRS})


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

    set(QtAwesome_LIBRARIES "${DEPENDS_DIR}/QtAwesome/MSVC/lib/QtAwesome.lib")

    set(QCodeEditor_LIBRARIES "${DEPENDS_DIR}/QCodeEditor/MSVC/lib/QCodeEditor.lib")

    set(portaudio_DIR "${DEPENDS_DIR}/portaudio/build/MSVC/include")
    set(portaudio_LIBRARIES "${DEPENDS_DIR}/portaudio/build/MSVC/lib/portaudio.lib")

    set(Qtpropertybrowser_LIBRARIES "${DEPENDS_DIR}/qtpropertybrowser/MSVC/lib/Qt6PropertyBrowser.lib")

    set(Libltc_DIR "${DEPENDS_DIR}/libltc_cmake")
    set(Libltc_LIBRARIES "${Libltc_DIR}/MSVC/lib/libltc.lib")

    target_link_libraries(NodeEditorCPP
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
set(QtAwesome_INCLUDE_DIRS "${DEPENDS_DIR}/QtAwesome")
INCLUDE_DIRECTORIES(${QtAwesome_INCLUDE_DIRS})
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

set(VST3_DIR "${DEPENDS_DIR}/vst3")
set(VST3_LIBRARIES_DIR "${VST3_DIR}/build/lib")
#add_subdirectory(${VST3_DIR})
include_directories(${VST3_DIR})
link_directories(${VST3_LIBRARIES_DIR})
add_subdirectory(src/Nodes/VST3PluginNode)

include_directories(${ossia_dir})
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
add_subdirectory(src/Nodes/TimeCodeSourceNode)  #时间码输入
add_subdirectory(src/Nodes/StringSourceNode)    #字符串显示
add_subdirectory(src/Nodes/DataInfoNode)        #数据显示
#add_subdirectory(src/Nodes/PythonNode)
#add_subdirectory(src/Nodes/NewPythonNode)
add_subdirectory(src/Nodes/ImageLoadNode)       #图像导入
add_subdirectory(src/Nodes/ImageShowNode)       #图像显示
#add_subdirectory(src/Nodes/AudioPlayNode)      #音频播放模块
add_subdirectory(src/Nodes/BoolSourceNode)      #布尔输入模块
add_subdirectory(src/Nodes/IntSourceNode)       #整数输入模块
add_subdirectory(src/Nodes/LuaScriptNode)       #Lua脚本模块
add_subdirectory(src/Nodes/TCPServerNode)       #TCP服务器模块
add_subdirectory(src/Nodes/TCPClientNode)       #TCP客户端模块
add_subdirectory(src/Nodes/FloatSourceNode)     #浮点输入模块
add_subdirectory(src/Nodes/DynamicPortNode)     #动态端口模块
add_subdirectory(src/Nodes/TimeLineNode)        #时间轴模块
add_subdirectory(src/Nodes/UDPSocketNode)       #UDP模块
add_subdirectory(src/Nodes/DelayNode)           #延迟模块
add_subdirectory(src/Nodes/LogicOperationNode)  #逻辑操作模块
add_subdirectory(src/Nodes/MathOperationNode)   #数学操作模块
add_subdirectory(src/Nodes/LFONode)             ##LFO模块
add_subdirectory(src/Nodes/QMLNode)             #QML模块
add_subdirectory(src/Nodes/AudioDecoderNode)    #音频解码模块
add_subdirectory(src/Nodes/AudioDeviceOutNode)  #音频输出模块
add_subdirectory(src/Nodes/HotKeyNode)          #热键模块
add_subdirectory(src/Nodes/ExtractNode)         #数据分离模块
add_subdirectory(src/Nodes/ImageInfoNode)       #图像信息
add_subdirectory(src/Nodes/ScaleImageNode)      #图像缩放
add_subdirectory(src/Nodes/CameraNode)          #摄像头输入
add_subdirectory(src/Nodes/CaptureNode)         #图像捕获
add_subdirectory(src/Nodes/ConvertImageNode)    #图像转换
add_subdirectory(src/Nodes/CutImageNode)        #图像切割
add_subdirectory(src/Nodes/DrawRectsNode)       #矩形绘图
add_subdirectory(src/Nodes/RectNode)            #矩形模块
add_subdirectory(src/Nodes/SizeVarNode)         #尺寸模块
add_subdirectory(src/Nodes/RectVarNode)         #矩形变量
add_subdirectory(src/Nodes/MergeNode)           #数据合并
add_subdirectory(src/Nodes/OscInNode)           #OSC输入
add_subdirectory(src/Nodes/OscOutNode)          #OSC输出
add_subdirectory(src/Nodes/ArtnetInNode)        #Arnet输入
#add_subdirectory(src/Nodes/MltSourceNode)       #Mlt输入
add_subdirectory(src/Nodes/MpvControllerNode)   #mpv控制
add_subdirectory(src/Nodes/SwitchNode)          #输入切换
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/cmake-build-debug")
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
        src/Widget/NodeWidget/CustomDataFlowGraphModel.h
        src/Widget/NodeWidget/CustomDataFlowGraphModel.cpp
        src/Widget/NodeWidget/CustomFlowGraphicsScene.cpp
        src/Widget/NodeWidget/CustomFlowGraphicsScene.h
        src/Widget/PortEditWidget/PortEditAddRemoveWidget.hpp
        src/Widget/PortEditWidget/PortEditAddRemoveWidget.cpp
        src/Widget/AboutWidget/AboutWidget.hpp
        src/Widget/MainWindow/MainWindow.cpp
        src/Widget/MainWindow/MainWindow.h
        src/Widget/CurveWidget/CurveEditor.cpp
        src/Widget/CurveWidget/CurveEditor.h
        src/Widget/CurveWidget/CurveEditor.ui
        src/Widget/SplashWidget/CustomSplashScreen.cpp
        src/Widget/SplashWidget/CustomSplashScreen.h
        src/Widget/ExternalControl/ExternalControler.cpp
        src/Widget/ExternalControl/ExternalControler.h
        src/Widget/ConsoleWidget/LogWidget.hpp
        src/Widget/ConsoleWidget/LogHandler.cpp
        src/Widget/ConsoleWidget/LogHandler.h
        src/Widget/MenuBarWidget/MenuBarWidget.cpp
        src/Widget/MenuBarWidget/MenuBarWidget.h
        src/Widget/PropertyWidget/PropertyWidget.cpp
        src/Widget/PropertyWidget/PropertyWidget.h
        src/Widget/PortEditWidget/PortEditWidget.cpp
        src/Widget/PortEditWidget/PortEditWidget.h
        src/Widget/NodeListWidget/draggabletreewidget.cpp
        src/Widget/NodeListWidget/draggabletreewidget.cpp
        src/Widget/NodeListWidget/NodeListWidget.cpp
        src/Widget/NodeListWidget/NodeListWidget.h
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
        src/Widget/StatusBar/StatusBar.cpp
        src/Widget/StatusBar/StatusBar.h
        src/Common/GUI/QPropertyBrowser/QPropertyBrowser.cpp
        src/Common/GUI/QPropertyBrowser/QPropertyBrowser.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(NodeEditorCPP
            MANUAL_FINALIZATION
            ${CALC_SOURCE_FILES}
    )

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET NodeEditorCPP APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(NodeEditorCPP SHARED
                ${CALC_SOURCE_FILES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(NodeEditorCPP
                ${CALC_SOURCE_FILES}
                res/res.rc
        )
    endif()
endif()

set(QRC_FILES
        resources.qrc
        )
target_compile_definitions(NodeEditorCPP PRIVATE -DNODE_EDITOR_SHARED)
target_link_libraries(NodeEditorCPP
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
            ${QtNodes_LIBRARIES}
            ${Spdlog_LIBRARIES}
            ${ADS_LIBRARIES}
            ${Libpd_LIBRARIES}
            ${QHotKey_LIBRARIES}
            ${QtAwesome_LIBRARIES}
            ${QCodeEditor_LIBRARIES}
            ${Qtpropertybrowser_LIBRARIES}
            ${Libltc_LIBRARIES}
            Eigen3::Eigen
            VST3PluginNode
            AudioDeviceOutNode
            AudioDecoderNode
            StringSourceNode
#            MltSourceNode
#            PythonNode
            ImageLoadNode
            ImageShowNode
            ImageInfoNode
##            AudioPlayNode
            BoolSourceNode
            IntSourceNode
            LuaScriptNode
            TCPServerNode
            FloatSourceNode
            DynamicPortNode
            TCPClientNode
            TimeLineNode
            UDPSocketNode
            DelayNode
            LogicOperationNode
            MathOperationNode
            DataInfoNode
            LFONode
            QMLNode
            HotKeyNode
            TimeCodeSourceNode
            ImageScaleNode
            CameraNode
            CaptureNode
            ConvertImageNode
            CutImageNode
            DrawRectsNode
            RectNode
            SizeVarNode
            RectVarNode
            ExtractNode
            MergeNode
            OscInNode
            OscOutNode
            ArtnetInNode
            MpvControllerNode
            SwitchNode


)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.

#if(${QT_VERSION} VERSION_LESS 6.1.0)
#  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.NodeEditorCPP)
#endif()
set_target_properties(NodeEditorCPP PROPERTIES
    ${BUNDLE_ID_OPTION}
#    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
#    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
#    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE

)

include(GNUInstallDirs)
install(TARGETS NodeEditorCPP
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(NodeEditorCPP)
endif()
if(WIN32)
    # Windows 下的处理逻辑
elseif(UNIX)

endif()
