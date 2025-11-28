// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtGraphs

Item {
    property int shadowQuality: Graphs3D.ShadowQuality.High
    property int cameraPreset: Graphs3D.CameraPreset.Front
    property alias meshSmooth: scatterSeries.meshSmooth
    property alias theme: scatterGraph.theme

    // 函数级注释：由外部注入的散点数据模型（ListModel，角色为 xPos/yPos/zPos）
    property var seriesDataModel

    Scatter3D {
        id: scatterGraph
        anchors.fill: parent

        // 函数级注释：主题通过对外暴露的 property alias theme 设置，不直接引用外部 id
        // 例如在 main.qml 中通过 Graph { theme: themeQt } 传入
        shadowQuality: parent.shadowQuality
        cameraPreset: parent.cameraPreset

        axisX.segmentCount: 3
        axisX.subSegmentCount: 2
        axisX.labelFormat: "%.2f"
        axisZ.segmentCount: 2
        axisZ.subSegmentCount: 2
        axisZ.labelFormat: "%.2f"
        axisY.segmentCount: 2
        axisY.subSegmentCount: 2
        axisY.labelFormat: "%.2f"

        Scatter3DSeries {
            id: scatterSeries
            itemLabelFormat: "Series 1: X:@xLabel Y:@yLabel Z:@zLabel"

            ItemModelScatterDataProxy {
                // 函数级注释：使用注入的模型提供 x/y/z 三个位置角色
                itemModel: seriesDataModel
                xPosRole: "xPos"
                yPosRole: "yPos"
                zPosRole: "zPos"
            }
        }
    }
}
