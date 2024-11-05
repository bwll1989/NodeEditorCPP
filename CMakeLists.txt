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
set(DEPENDS_DIR "${CMAKE_SOURCE_DIR}/3rdParty")
set(CMAKE_PREFIX_PATH ${DEPENDS_DIR})
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/Common/Gui)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/Common/Devices)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/DataTypes)
add_definitions(-DQT_MESSAGELOGCONTEXT)

set(Spdlog_INCLUDE_DIRS "${DEPENDS_DIR}/spdlog/build/install/include" )
set(Spdlog_LIBRARIES "${DEPENDS_DIR}/spdlog/build/install/lib/libspdlog.a")
INCLUDE_DIRECTORIES(${Spdlog_INCLUDE_DIRS})
message("--Found spdlog ${Spdlog_LIBRARIES}")


set(Libpd_INCLUDE_DIRS "${DEPENDS_DIR}/libpd/libpd_wrapper" )
INCLUDE_DIRECTORIES(${Libpd_INCLUDE_DIRS})
set(Libpd_LIBRARIES "${DEPENDS_DIR}/libpd/build/liblibpd.dll")
set(PureData_INCLUDE_DIRS "${DEPENDS_DIR}/libpd/pure-data/src")
INCLUDE_DIRECTORIES(${PureData_INCLUDE_DIRS})
# 查找Libpd库


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Quick QuickWidgets Widgets Qt6Qml Qt6Quick  Concurrent MultimediaWidgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Quick QuickWidgets Widgets Network Concurrent MultimediaWidgets Multimedia)
#添加QT库

set(QtNodes_DIR "${DEPENDS_DIR}/nodeeditor/install/lib/cmake/QtNodes")
find_package(QtNodes REQUIRED)
message("-- Found QtNodes ${QtNodes_DIR}")
#添加QtNodes库

set(QHotKey_INCLUDE_DIRS "${DEPENDS_DIR}/QHotkey-master/install/include" )
INCLUDE_DIRECTORIES(${QHotKey_INCLUDE_DIRS})
set(QHotKey_LIBRARIES "${DEPENDS_DIR}/QHotkey-master/install/lib/libqhotkey.a")
message("-- Found QHotkey ${QHotKey_INCLUDE_DIRS}")
#添加QHotKey库

set(ADS_INCLUDE_DIRS "${DEPENDS_DIR}/Qt-Advanced-Docking-System/build/install/include" )
set(ADS_LIBRARIES "${DEPENDS_DIR}/Qt-Advanced-Docking-System/build/install/bin/libqt6advanceddocking.dll")
INCLUDE_DIRECTORIES(${ADS_INCLUDE_DIRS})
message("-- Found qt6advanceddocking ${ADS_INCLUDE_DIRS}")

set(QtAwesome_INCLUDE_DIRS "${DEPENDS_DIR}/QtAwesome/install/include")
set(QtAwesome_LIBRARIES "${DEPENDS_DIR}/QtAwesome/install/lib/libQtAwesome.a")
INCLUDE_DIRECTORIES(${QtAwesome_INCLUDE_DIRS})
#添加libevent库

set(FFMPEG_INCLUDE_DIRS "${DEPENDS_DIR}/ffmpeg-n6.1.1-win64-gpl-shared-6.1/include")
set(FFMPEG_LIB_DIRS "${DEPENDS_DIR}/ffmpeg-n6.1.1-win64-gpl-shared-6.1/bin")
include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories("${FFMPEG_LIB_DIRS}")

set(QCodeEditor_INCLUDE "${DEPENDS_DIR}/QCodeEditor/include")
INCLUDE_DIRECTORIES(${QCodeEditor_INCLUDE})
set(QCodeEditor_LIBRARIES "${DEPENDS_DIR}/QCodeEditor/build/libQCodeEditor.a")
message("-- Found QCodeEditor ${QCodeEditor_INCLUDE}")

set(Eigen3_DIR "${DEPENDS_DIR}/eigen-3.4.0/INSTALL/share/eigen3/cmake")
find_package(Eigen3 REQUIRED)
INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})

set(portaudio_DIR "${DEPENDS_DIR}/portaudio/install/lib/cmake/portaudio")
find_package(portaudio CONFIG REQUIRED)
message("-- Found portaudio ${portaudio_DIR}")

set(SndFile_DIR "${DEPENDS_DIR}/libsndfile/install/cmake")
find_package(SndFile REQUIRED)
message("-- Found SndFile ${SndFile_DIR}")

set(OpenCV_DIR "${DEPENDS_DIR}/opencv-4.10.0/build/install")
find_package(OpenCV REQUIRED)
INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS})
if(OpenCV_FOUND)

    message("-- Found OpenCV ${OpenCV_DIR}")

endif()

set(Qtpropertybrowser_DIR "${DEPENDS_DIR}/qtpropertybrowser")
include_directories("${Qtpropertybrowser_DIR}/src")
#link_directories("${Qtpropertybrowser_DIR}/lib")
set(Qtpropertybrowser_LIBRARIES "${Qtpropertybrowser_DIR}/lib/Qt6PropertyBrowser.dll")
message("-- Found Qtpropertybrowser ${Qtpropertybrowser_DIR}")

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
add_subdirectory(src/Nodes/GroupNode)
#字符串输入模块
add_subdirectory(src/Nodes/StringSourceNode)
add_subdirectory(src/Nodes/DataInfoNode)
#字符串显示模块
#add_subdirectory(src/Nodes/PythonNode)
#add_subdirectory(src/Nodes/NewPythonNode)
add_subdirectory(src/Nodes/ImageLoadNode)

add_subdirectory(src/Nodes/ImageShowNode)

add_subdirectory(src/Nodes/AudioPlayNode)
# #音频播放模块
add_subdirectory(src/Nodes/BoolSourceNode)
#布尔输入模块
add_subdirectory(src/Nodes/IntSourceNode)
#整数输入模块
add_subdirectory(src/Nodes/LuaScriptNode)
#Lua脚本模块
add_subdirectory(src/Nodes/TCPServerNode)
#TCP服务器模块
add_subdirectory(src/Nodes/TCPClientNode)
#TCP客户端模块
add_subdirectory(src/Nodes/FloatSourceNode)
#浮点输入模块
add_subdirectory(src/Nodes/DynamicPortNode)
#动态端口模块
add_subdirectory(src/Nodes/TimeLineNode)
#时间轴模块
add_subdirectory(src/Nodes/UDPSocketNode)
#UDP模块
add_subdirectory(src/Nodes/DelayNode)
#延迟模块
add_subdirectory(src/Nodes/LogicOperationNode)
#逻辑操作模块
add_subdirectory(src/Nodes/MathOperationNode)
#数学操作模块
add_subdirectory(src/Nodes/LFONode)
#LFO模块
add_subdirectory(src/Nodes/QMLNode)
#QML模块
add_subdirectory(src/Nodes/AudioDecoderNode)
#音频解码模块
add_subdirectory(src/Nodes/AudioDeviceOutNode)
#音频输出模块
add_subdirectory(src/Nodes/HotKeyNode)
#热键模块
add_subdirectory(src/Nodes/ExtractNode)
add_subdirectory(src/Nodes/ImageInfoNode)
add_subdirectory(src/Nodes/ScaleImageNode)
add_subdirectory(src/Nodes/CameraNode)
add_subdirectory(src/Nodes/CaptureNode)
add_subdirectory(src/Nodes/ConvertImageNode)
add_subdirectory(src/Nodes/CutImageNode)
add_subdirectory(src/Nodes/DrawRectsNode)
add_subdirectory(src/Nodes/RectNode)
add_subdirectory(src/Nodes/SizeVarNode)
add_subdirectory(src/Nodes/RectVarNode)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/cmake-build-debug")
set(CALC_SOURCE_FILES
        main.cpp
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
#        src/Nodes/NewPythonNode/PythonScriptInterface.hpp
#        src/Nodes/NewPythonNode/PythonNodeModel.hpp
#        src/Nodes/NewPythonNode/CodeEditor.hpp
#        src/Nodes/NewPythonNode/CodeEditor.cpp
#        src/Nodes/NewPythonNode/ScriptInterface.cpp
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
#message("build example python binding")
#pybind11_add_module(example src/Nodes/NewPythonNode/bindings.cpp)

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
target_link_libraries(NodeEditorCPP
        PUBLIC
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Gui
            Qt${QT_VERSION_MAJOR}::Core
            Qt${QT_VERSION_MAJOR}::Quick
            Qt${QT_VERSION_MAJOR}::QuickWidgets
            Qt${QT_VERSION_MAJOR}::Network
            Qt${QT_VERSION_MAJOR}::Concurrent
            Qt${QT_VERSION_MAJOR}::Multimedia
            Qt${QT_VERSION_MAJOR}::MultimediaWidgets
            ${OpenCV_LIBS}
            ${QtNodes_LIBRARIES}
            ${Log4Qt_LIBRARIES}
            ${Spdlog_LIBRARIES}
            ${ADS_LIBRARIES}
            ${Libpd_LIBRARIES}
            ${QHotKey_LIBRARIES}
            ${QtAwesome_LIBRARIES}
            ${QCodeEditor_LIBRARIES}
            ${Qtpropertybrowser_LIBRARIES}
            Eigen3::Eigen
#            Python3::Python
#            pybind11::embed
#            pybind11::module
            AudioDeviceOutNode
            AudioDecoderNode
            StringSourceNode
#            PythonNode
            ImageLoadNode
            ImageShowNode
            ImageInfoNode
            AudioPlayNode
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
            GroupNode
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
#        RTAudioDecoderNode

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
