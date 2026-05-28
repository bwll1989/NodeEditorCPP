import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

Item {
    id: root
    anchors.fill: parent

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("运行脚本")
                onClicked: node.runScript()
            }

            Button {
                text: qsTr("更新界面")
                onClicked: node.refreshUi()
            }

            Button {
                text: qsTr("编辑脚本")
                onClicked: node.openEditor()
            }

            Item { Layout.fillWidth: true }

            BusyIndicator {
                implicitWidth: 20
                implicitHeight: 20
                running: node.executing
                visible: node.executing
            }
        }

        Label {
            Layout.fillWidth: true
            visible: node.scriptError.length > 0
            color: "#e74c3c"
            wrapMode: Text.WordWrap
            text: node.scriptError
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                anchors.fill: parent
                clip: true

                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    enabled: !node.initializing

                    Repeater {
                        model: node.uiSchema
                        delegate: SchemaField {
                            required property var modelData
                            required property int index

                            Layout.fillWidth: true
                            spec: modelData
                            backend: node
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: !node.uiSchema || node.uiSchema.length === 0
                        opacity: 0.65
                        wrapMode: Text.WordWrap
                        text: qsTr("uiSchema 为空。请在 initInterface() 中调用 Node.setUiSchema([...])。")
                    }
                }
            }
        }
    }
}
