import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// 函数级注释：主界面容器，包含 WebEngineView（承载时间轴 HTML/JS）与底部控制栏。
// 负责监听 WebView 的 console 输出，将播放头时间/百分比/状态同步到 C++（QMLDataModel）。

Rectangle {
    visible: true
    width: 860
    height: 300
    color: "#1d1d1d"
    z: -1
    // 函数级注释：保存时间轴 JSON 字符串；C++ 侧 load() 会写入此属性
    property string savedData: ""
    // 函数级注释：标记页面是否加载完成，完成后允许导入
    property bool pageReady: false

    // 函数级注释：标记是否处于“自动保存写入 savedData”的过程中，用于避免循环导入
    property bool autoSaving: false

    // 函数级注释：当根项 savedData 变化且页面已就绪时，自动导入时间轴数据。
    // 增加防护：若是自动保存触发的变化（autoSaving 为 true），则不导入，避免死循环。
    onSavedDataChanged: {
        if (pageReady && savedData && savedData.length > 0 && !autoSaving) {
            timeline.importData(JSON.parse(savedData))
            // console.log("Imported timeline on savedData change (QML)");
        }
    }
    RowLayout{
        anchors.fill: parent
        spacing: 0
        
            /**
            * @brief 当前曲线选择下拉框
            * 绑定到 timeline.curves 名称列表，切换时调用 setActiveCurve
            */
        ColumnLayout {
            id: curveSelector
            Layout.fillHeight: true
            anchors.margins: 10
            spacing: 10
            width:parent.width * 0.1
            ListView {
                    id: curvesList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    clip: true

                    // 使用对象数组作为模型，modelData 即为 { name, points, ... }
                    model: (timeline && timeline.curves) ? timeline.curves : []

                    // 初始选中与 timeline.activeCurveIndex 同步（若存在）
                    currentIndex: (timeline && timeline.activeCurveIndex >= 0) ? timeline.activeCurveIndex : 0

                    // 选中变更时，通知 timeline（带防抖：相等则不触发）
                    onCurrentIndexChanged: {
                        if (timeline && timeline.activeCurveIndex !== curvesList.currentIndex) {
                            timeline.setActiveCurve(curvesList.currentIndex)
                        }
                    }
                // 选中项高亮外观
                    highlight: Rectangle {
                        color: "#2a2a2a"
                        radius: 6
                        border.color: "#444"
                        border.width: 1
                    }
                    highlightMoveDuration: 120
                    delegate: Rectangle {
                        width: curvesList.width
                        height: 30
                        color: "transparent" 
                        Rectangle {
                            anchors.fill: parent
                            radius: 6
                            color: {
                                var selected = curvesList.currentIndex === index
                                var base = index % 2 === 0 ? '#2b2a2a' : "#1b1b1b"
                                return selected ? "#303030" : (hoverArea.containsMouse ? "#262626" : base)
                            }
                        }

                        // 左侧颜色标识条：与绘图颜色一致
                        Rectangle {
                            anchors.right: parent.right
                            anchors.rightMargin: 0
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 4
                            radius: 2
                            color: timeline.getCurveColor(index)
                        }

                        // 名称文本
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            text: (modelData && modelData.name) ? modelData.name : ("Curve " + (index + 1))
                            color: timeline.getCurveColor(index)
                            font.bold: curvesList.currentIndex === index
                            elide: Text.ElideRight
                        }

                        // 悬停/点击交互
                        MouseArea {
                            id: hoverArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (curvesList.currentIndex !== index)
                                    curvesList.currentIndex = index
                            }
                        }}}

                // 与 timeline.activeCurveIndex 变化保持同步（避免相等时重复设置）
                Connections {
                    target: timeline
                    function onActiveCurveIndexChanged() {
                        if (curvesList.currentIndex !== timeline.activeCurveIndex) {
                            curvesList.currentIndex = timeline.activeCurveIndex
                        }
                    }
                    function onCurveCountChanged() {
                            // 记录旧索引，刷新模型
                            var oldIndex = curvesList.currentIndex
                            curvesList.model = (timeline && timeline.curves) ? timeline.curves : []

                            // 计算新范围并 clamp 选中索引
                            var maxIdx = (timeline && timeline.curves) ? timeline.curves.length - 1 : -1
                            if (maxIdx >= 0) {
                                var newIndex = Math.min(Math.max(0, oldIndex), maxIdx)
                                if (curvesList.currentIndex !== newIndex) {
                                    curvesList.currentIndex = newIndex
                                }
                                // 确保 timeline 逻辑也同步
                                if (timeline.activeCurveIndex !== newIndex) {
                                    timeline.setActiveCurve(newIndex)
                                }
                            } else {
                                curvesList.currentIndex = -1
                            }
                        }}}

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // 替换 WebEngineView 为纯 QML 时间轴组件
            TimelineEditor {
                id: timeline
                objectName: "timelineEditor"  // 供 C++ 查找（如需）

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 1
                Layout.minimumHeight: 0

                // 页面就绪标记
                Component.onCompleted: {
                    pageReady = true
                    if (savedData && savedData.length > 0) {
                        timeline.importData(JSON.parse(savedData))
                    }
                }

                // 自动保存回调：写入根 savedData（带防循环标记）
                onDataChanged: function (json) {
                    autoSaving = true
                    savedData = json
                    autoSaving = false
                    // console.log("Auto-saved timeline data (savedData updated).");
                }

                // 播放头变化：同步顶部显示与 C++ 侧
            onPlayheadTimeChange: function(time, valueX, valueY, isPlaying) {
                valueDisplay.text = time.toFixed(2) + " s  " ;
                valueDisplay.color = isPlaying ? "#FFFFFF" : "#666666"
                // 兼容之前 C++ 侧的字符串数组格式
                CppBridge.updatePlayheadTime([
                    "event.playheadTimeChange:",
                    time.toFixed(3),
                    valueX.toFixed(3),
                    valueY,
                    isPlaying ? "true" : "false"
                ])
            }
            }

            // 底部工具栏保持不变，调用 QML 方法
            Rectangle {
                Layout.fillWidth: true
                height: 40
                color: "#1e1e1e"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    // 播放/暂停
                    Button {
                        text: "Play"
                        onClicked: if (!timeline.playhead.isPlaying) timeline.play(); else timeline.pause()
                    }

                    // 停止或回到起始位置
                    Button {
                        text: "Stop"
                        onClicked: if (timeline.playhead.isPlaying) {
                            timeline.pause();
                            timeline.seek(0)
                        } else timeline.seek(0)
                    }
                    CheckBox {
                        text: "Loop"
                        checked: timeline.isLoop
                        onCheckedChanged: {
                            timeline.isLoop = checked
                            CppBridge.updateLoop(checked)
                        }
                    }
                    // // 可选：缩放控制（不依赖滚轮）
                    // Button { text: "Zoom+"; onClicked: timeline.viewportDuration = Math.max(timeline.minViewportDuration, timeline.viewportDuration - 5); }
                    // Button { text: "Zoom-"; onClicked: timeline.viewportDuration = Math.min(timeline.maxViewportDuration, timeline.viewportDuration + 5); }

                    // 顶部状态显示
                    Text {
                        id: valueDisplay
                        text: "Ready"
                        color: "#666"
                        font.family: "Monaco"
                        font.pointSize: 12
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                    }


                    /**
                     * @brief 添加新曲线按钮
                     */
                    Button {
                        text: "Add Curve"
                        onClicked: timeline.addCurve()
                    }

                    /**
                     * @brief 删除当前曲线按钮（至少保留一条）
                     */
                    Button {
                        text: "Remove Curve"
                        onClicked: timeline.removeCurve()
                    }}}}
    }
}
    

