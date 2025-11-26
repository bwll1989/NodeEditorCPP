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

    ColumnLayout {
        anchors.fill: parent
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
            onDataChanged: function(json) {
                autoSaving = true
                savedData = json
                autoSaving = false
                // console.log("Auto-saved timeline data (savedData updated).");
            }

            // 播放头变化：同步顶部显示与 C++ 侧
            onPlayheadTimeChange: function(time, valueX, valueY, isPlaying) {
                valueDisplay.text = time.toFixed(2) + " s  " + valueY.toFixed(2) + " %"
                valueDisplay.color = isPlaying ? "#FFFFFF" : "#666666"
                // 兼容之前 C++ 侧的字符串数组格式
                CppBridge.updatePlayheadTime([
                    "event.playheadTimeChange:",
                    time.toFixed(3),
                    valueX.toFixed(3),
                    valueY.toFixed(3),
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

                // // 撤销
                // Button { text: "Undo"; onClicked: timeline.undo() }

                // 播放/暂停
                Button {
                    text: "Play"
                    onClicked: if (!timeline.playhead.isPlaying) timeline.play(); else timeline.pause()
                }

                // 停止或回到起始位置
                Button {
                    text: "Stop"
                    onClicked: if (timeline.playhead.isPlaying) {timeline.pause(); timeline.seek(0)} else timeline.seek(0)
                }
                CheckBox {
                    text: "Loop"
                    checked: timeline.isLoop
                    onCheckedChanged:{
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
            }
        }
    }
}