// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs
//! [0]

Item {
    id: mainView
    width: 800
    height: 600

    property int margin: 30
    property int spacing: 10
    property int radius: 6
    property int buttonMinWidth: 175

    // //! [1]
    // 函数级注释：散点数据源模型，包含 xPos/yPos/zPos 三个角色
    ListModel {
        id: seriesModel
    }
    
    // 函数级注释：向散点模型追加一个点 (x, y, z)
    function appendPoint(x, y, z) {
        seriesModel.append({ xPos: x, yPos: y, zPos: z })
    }
    function updatePoint( x, y, z) {
        if(seriesModel.count == 0)
            seriesModel.append({ xPos: x, yPos: y, zPos: z })
        else{
            seriesModel.setProperty(0, "xPos", x)
            seriesModel.setProperty(0, "yPos", y)
            seriesModel.setProperty(0, "zPos", z)
        }

    }
    
    // 函数级注释：清空散点模型中的所有点
    function clearPoints() {
        seriesModel.clear()
    }
    //! [1]

    //! [2]
    GraphsTheme {
        id: themeQt
        theme: GraphsTheme.Theme.QtGreen
        labelFont.pointSize: 40
    }

    GraphsTheme {
        id: themeQtNeonGreen
        theme: GraphsTheme.Theme.QtGreenNeon
        colorScheme: GraphsTheme.ColorScheme.Dark
    }
    //! [2]

    //! [5]
    component CustomButton : RoundButton {
        id: buttonRoot
        //! [5]
        //! [6]
        property alias source: iconImage.source

        Layout.minimumWidth: buttonMinWidth
        Layout.fillWidth: true

        radius: mainView.radius

        background: Rectangle {
            radius: mainView.radius
            color: "white"
            border.color: "black"
        }
        //! [6]
        //! [7]
        contentItem: Row {
            id: content
            Image {
                id: iconImage
            }
            Label {
                text: buttonRoot.text
                horizontalAlignment: Text.AlignLeft
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        //! [7]
    }

    //! [3]
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: margin
        spacing: spacing
        //! [3]
        //! [4]
        GridLayout {
            Layout.fillWidth: true
            rowSpacing: spacing
            columnSpacing: spacing

            columns: mainView.width < mainView.buttonMinWidth * 2 + mainView.spacing + mainView.margin * 2 // width of 2 buttons
                ? 1
                : (mainView.width < mainView.buttonMinWidth * 4 + mainView.spacing * 3 + mainView.margin * 2 // width of 4 buttons
                    ? 2
                    : 4)
            //! [4]
            //! [8]
            CustomButton {
                id: shadowButton
                text: graph.shadowQuality === Graphs3D.ShadowQuality.None ?
                    qsTr("Show Shadows") : qsTr("Hide Shadows")
                onClicked: {
                    graph.shadowQuality = graph.shadowQuality === Graphs3D.ShadowQuality.None ?
                        Graphs3D.ShadowQuality.High :
                        Graphs3D.ShadowQuality.None
                }
            }
            //! [8]

            CustomButton {
                id: smoothButton
                text: qsTr("Smooth Series One")

                onClicked: {
                    text = graph.meshSmooth ? qsTr("Smooth Series One") : qsTr("Flat Series One")
                    graph.meshSmooth = !graph.meshSmooth
                }
            }

            CustomButton {
                id: cameraButton
                text: qsTr("Camera Placement")

                onClicked: {
                    graph.cameraPreset = graph.cameraPreset === Graphs3D.CameraPreset.Front ?
                        Graphs3D.CameraPreset.IsometricRightHigh :
                        Graphs3D.CameraPreset.Front
                }
            }

            CustomButton {
                id: backgroundButton
                text: qsTr("Hide Background")

                onClicked: {
                    graph.theme.plotAreaBackgroundVisible = !graph.theme.plotAreaBackgroundVisible
                    text = graph.theme.plotAreaBackgroundVisible ? qsTr("Hide Graph Background") : qsTr("Show Graph Background")
                }
            }
        }

        //! [9]
        // 散点图组件，注入数据模型与主题
        Graph {
            id: graph
            Layout.fillWidth: true
            Layout.fillHeight: true
            // 函数级注释：将散点数据模型传递到 Graph 组件内部的 Scatter3D 代理
            seriesDataModel: seriesModel
            theme: themeQt
        }
        //! [9]
        GridLayout {
            Layout.fillWidth: true
            rowSpacing: spacing
            columnSpacing: spacing

            columns: mainView.width < mainView.buttonMinWidth * 2 + mainView.spacing + mainView.margin * 2 // width of 2 buttons
                ? 1
                : (mainView.width < mainView.buttonMinWidth * 4 + mainView.spacing * 3 + mainView.margin * 2 // width of 4 buttons
                    ? 2
                    : 4)
            //! [10]
            CustomButton {
                id: themeButton
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: false
                //! [10]
                text: qsTr("Change Theme")
                onClicked: {
                    graph.theme = graph.theme.theme === GraphsTheme.Theme.QtGreenNeon ? themeQt : themeQtNeonGreen
                    backgroundButton.text = graph.theme.plotAreaBackgroundVisible ? qsTr("Hide Graph Background") : qsTr("Show Graph Background")
                }
            }
            //! [10]
            CustomButton {
                id: themeButton2
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: false
                //! [10]
                text: qsTr("Clear Scene")

                onClicked: {
                    // 函数级注释：清空散点数据
                    clearPoints()

                }
            }
        }
    }
}
