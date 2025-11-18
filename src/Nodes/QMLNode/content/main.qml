import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtWebEngine

Rectangle {
    visible: true
    width: 860
    height: 440

    property string savedData: ""

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        WebEngineView {
            id: webView
            Layout.fillWidth: true
            Layout.fillHeight: true
            url: "qrc:qml/content/main.html"

            // 函数级注释：页面加载状态变化回调，在加载成功时注册时间轴事件监听
            onLoadingChanged: function(loadingInfo) {
                if (loadingInfo.status === WebEngineLoadingInfo.LoadSucceededStatus) {
                    console.log("Timeline editor loaded successfully")

                    // 注册事件监听，向 C++ 同步播放头时间与状态
                    webView.runJavaScript(`
                        timeline.addEventListener('playheadTimeChange', function(data) {
                            console.log("event.playheadTimeChange:",
                                data.time.toFixed(2),
                                data.value.toFixed(2),
                                data.isPlaying);
                        });
                    `);
                }
            }

            // 接收 WebView 的 console 输出
            onJavaScriptConsoleMessage: function(level, message, lineNumber, sourceId) {

                // Split the message into parts
                const parts = message.split(' ');

                // Parse the message
                if (level===0 && parts[0] == "event.playheadTimeChange:") {
                    valueDisplay.text = parts[1] + " s  " + parts[2] + " %";
                    valueDisplay.color = parts[3] === "true" ? "#FFFFFF" : "#666666";
                    // 同步给 C++ 顶部控件（QTimeEdit / 按钮状态）
                    CppBridge.updatePlayheadTime(parts);
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#1e1e1e"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Button {
                    text: "Save"
                    onClicked: {
                        webView.runJavaScript("timeline.exportData()", function(result) {
                            savedData = JSON.stringify(result)
                            console.log("Exported data:", savedData)
                        })
                    }
                }

                Button {
                    text: "Load"
                    onClicked: {
                        if (savedData.length > 0) {
                            // Use template string to avoid potential string concatenation issues
                            webView.runJavaScript(`timeline.importData(${savedData})`)
                            console.log("Imported data:", savedData)
                        } 
                    }
                }

                Button {
                    text: "Undo"
                    onClicked: webView.runJavaScript("timeline.undo()")
                }

                Button {
                    text: "Play"
                    onClicked: webView.runJavaScript("if (!timeline.playhead.isPlaying) timeline.play(); else timeline.pause()")
                }

                Button {
                    text: "Stop"
                    onClicked: webView.runJavaScript("if (timeline.playhead.isPlaying) timeline.pause(); else timeline.seek(0)")
                }

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