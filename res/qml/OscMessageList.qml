// QML：默认一行摘要，点击按钮展开编辑表单；支持拖拽启动与轻量重排
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    anchors.fill: parent
    Rectangle { anchors.fill: parent; color: "transparent" }

    ListView {
        id: listView
        anchors.fill: parent
        model: oscModel
        clip: true
        spacing: 6

        delegate: Item {
            id: rowItem
            width: listView.width
            property bool expanded: false
            height: expanded ? formBox.implicitHeight + header.implicitHeight + 12 : header.implicitHeight + 8

            Rectangle {
                id: card
                anchors.fill: parent
                radius: 6
                color: "#1e3a5f"    // 卡片背景（可按主题替换）
                border.color: "#4fa3e0"
                border.width: 1
            }

            Rectangle {
                id: accent
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 3
                radius: 2
                color: "#ffaa00"
            }

            RowLayout {
                id: header
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 6
                spacing: 10

                // 摘要字段
                Label { text: host; visible: host.length>0; font.pixelSize: 12; color: "#e6f3ff" }
                Label { text: address; font.pixelSize: 12; color: "#e6f3ff" }
                Label { text: dtype; font.pixelSize: 12; color: "#c8e2ff" }
                Label { text: dvalue; font.pixelSize: 12; color: "#c8e2ff" }
                Item { Layout.fillWidth: true }

                // 展开/折叠按钮
                ToolButton {
                    id: toggleBtn
                    text: rowItem.expanded ? "▼" : "▶"
                    onClicked: rowItem.expanded = !rowItem.expanded
                }

                // 拖拽句柄：长按启动外部拖拽，由 C++ 发起
                ToolButton {
                    id: dragBtn
                    text: "\u2630" // 菜单/拖拽图标
                    onPressedChanged: {
                        if (pressed) {
                            // 简单防抖：按下后由 C++ 启动拖拽
                            oscListWidget.startDragFromQML(index)
                        }
                    }
                }
            }

            ColumnLayout {
                id: formBox
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: header.bottom
                anchors.margins: 8
                spacing: 6
                visible: rowItem.expanded

                // 表单：双向绑定模型角色
                RowLayout {
                    Label { text: "目标"; Layout.preferredWidth: 40; color: "#e6f3ff" }
                    TextField {
                        text: host
                        onTextChanged: oscModel.setData(oscModel.index(index,0), text, oscModel.HostRole)
                        Layout.fillWidth: true
                    }
                }
                RowLayout {
                    Label { text: "地址"; Layout.preferredWidth: 40; color: "#e6f3ff" }
                    TextField {
                        text: address
                        onTextChanged: oscModel.setData(oscModel.index(index,0), text, oscModel.AddressRole)
                        Layout.fillWidth: true
                    }
                }
                RowLayout {
                    Label { text: "类型"; Layout.preferredWidth: 40; color: "#e6f3ff" }
                    ComboBox {
                        model: ["Int","Float","String"]
                        currentIndex: Math.max(0, ["Int","Float","String"].indexOf(dtype))
                        onCurrentTextChanged: oscModel.setData(oscModel.index(index,0), currentText, oscModel.TypeRole)
                    }
                }
                RowLayout {
                    Label { text: "值"; Layout.preferredWidth: 40; color: "#e6f3ff" }
                    TextField {
                        text: dvalue
                        onTextChanged: oscModel.setData(oscModel.index(index,0), text, oscModel.ValueRole)
                        Layout.fillWidth: true
                    }
                }
            }

            // 轻量重排：将拖放目标索引通知模型
            DropArea {
                anchors.fill: parent
                onDropped: {
                    if (drag.source && drag.source.hasOwnProperty("index")) {
                        oscModel.move(drag.source.index, index)
                    }
                }
            }
        }
    }
}