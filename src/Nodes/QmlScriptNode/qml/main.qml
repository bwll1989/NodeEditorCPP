import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.github.jcelerier.CreativeControls 1.0 as CC

Item {
    id: root
    anchors.fill: parent

    property var backend: node

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Button {
                text: "运行"
                onClicked: node.runScript()
            }

            Button {
                text: "编辑脚本"
                onClicked: node.openEditor()
            }

            Item { Layout.fillWidth: true }

            BusyIndicator {
                width: 18
                height: 18
                running: node.executing
                visible: node.executing
            }
        }

        GroupBox {
            title: "控件"
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                anchors.fill: parent

                Column {
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: node.uiSchema
                        delegate: Loader {
                            width: parent.width
                            property var spec: modelData

                            sourceComponent: {
                                if (!spec || !spec.type) return fallbackText
                                switch (String(spec.type)) {
                                case "slider":
                                    return sliderComp
                                case "spinbox":
                                    return spinComp
                                case "checkbox":
                                    return checkComp
                                case "lineedit":
                                    return lineComp
                                case "combobox":
                                    return comboComp
                                case "button":
                                    return buttonComp
                                case "label":
                                    return labelComp
                                default:
                                    return fallbackText
                                }
                            }
                        }
                    }

                    Item {
                        visible: !node.uiSchema || node.uiSchema.length === 0
                        width: parent.width
                        height: visible ? implicitHeight : 0

                        Column {
                            width: parent.width
                            spacing: 6

                            Label {
                                text: "uiSchema 为空：回退到 settings 键值编辑器"
                                opacity: 0.7
                            }

                            Repeater {
                                model: Object.keys(node.settings)
                                delegate: RowLayout {
                                    width: parent.width
                                    spacing: 6

                                    Label {
                                        text: modelData
                                        Layout.preferredWidth: 140
                                        elide: Label.ElideRight
                                    }

                                    TextField {
                                        Layout.fillWidth: true
                                        text: node.settings[modelData] !== undefined ? String(node.settings[modelData]) : ""
                                        onEditingFinished: node.setSettingValue(modelData, text)
                                    }

                                    Button {
                                        text: "删除"
                                        onClicked: node.removeSetting(modelData)
                                    }
                                }
                            }
                        }
                    }

                    Component {
                        id: fallbackText
                        Label { text: "未知控件" }
                    }

                    Component {
                        id: sliderComp
                        RowLayout {
                            width: parent.width
                            spacing: 8

                            property string key: spec.key || ""
                            property string label: spec.label || spec.key || ""
                            property real minVal: spec.min !== undefined ? Number(spec.min) : 0
                            property real maxVal: spec.max !== undefined ? Number(spec.max) : 1
                            property var defaultValue: spec.default !== undefined ? spec.default : undefined
                            property string onChangedFn: spec.onChanged || ""
                            property bool internalUpdate: false

                            function toNorm(v) {
                                var span = maxVal - minVal
                                if (span === 0) return 0
                                return Math.max(0, Math.min(1, (Number(v) - minVal) / span))
                            }
                            function fromNorm(n) {
                                var span = maxVal - minVal
                                return minVal + Math.max(0, Math.min(1, Number(n))) * span
                            }
                            function readSetting() {
                                if (!root.backend || !key) return undefined
                                return root.backend.settings[key]
                            }

                            Component.onCompleted: {
                                if (!root.backend || !key) return
                                var cur = readSetting()
                                if ((cur === undefined || cur === null) && defaultValue !== undefined) {
                                    root.backend.setSettingValue(key, defaultValue)
                                    cur = defaultValue
                                }
                                internalUpdate = true
                                slider.value = toNorm(cur !== undefined ? cur : minVal)
                                internalUpdate = false
                            }

                            CC.Label {
                                text: label
                                Layout.preferredWidth: 140
                                elide: Text.ElideRight
                            }

                            CC.HSlider {
                                id: slider
                                Layout.fillWidth: true
                                height: 30
                                textVisible: false
                                value: 0.5

                                onValueChanged: {
                                    if (internalUpdate) return
                                    if (!root.backend || !key) return
                                    var actual = fromNorm(value)
                                    root.backend.setSettingValue(key, actual)
                                    if (onChangedFn && onChangedFn.length > 0) {
                                        root.backend.callJsFunction(onChangedFn, [key, actual])
                                    }
                                }
                            }

                            CC.Label {
                                text: String(fromNorm(slider.value).toFixed(3))
                                Layout.preferredWidth: 72
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }

                    Component {
                        id: spinComp
                        RowLayout {
                            width: parent.width
                            spacing: 8

                            property string key: spec.key || ""
                            property string label: spec.label || spec.key || ""
                            property int minVal: spec.min !== undefined ? Number(spec.min) : 0
                            property int maxVal: spec.max !== undefined ? Number(spec.max) : 100
                            property int stepVal: spec.step !== undefined ? Number(spec.step) : 1
                            property var defaultValue: spec.default !== undefined ? spec.default : undefined
                            property string onChangedFn: spec.onChanged || ""
                            property bool internalUpdate: false

                            Component.onCompleted: {
                                if (!root.backend || !key) return
                                var cur = root.backend.settings[key]
                                if ((cur === undefined || cur === null) && defaultValue !== undefined) {
                                    root.backend.setSettingValue(key, defaultValue)
                                    cur = defaultValue
                                }
                                internalUpdate = true
                                spin.value = cur !== undefined ? parseInt(cur) : minVal
                                internalUpdate = false
                            }

                            CC.Label {
                                text: label
                                Layout.preferredWidth: 140
                                elide: Text.ElideRight
                            }

                            SpinBox {
                                id: spin
                                Layout.fillWidth: true
                                from: minVal
                                to: maxVal
                                stepSize: stepVal

                                onValueModified: {
                                    if (internalUpdate) return
                                    if (!root.backend || !key) return
                                    root.backend.setSettingValue(key, value)
                                    if (onChangedFn && onChangedFn.length > 0) {
                                        root.backend.callJsFunction(onChangedFn, [key, value])
                                    }
                                }
                            }
                        }
                    }

                    Component {
                        id: checkComp
                        CheckBox {
                            width: parent.width

                            property string key: spec.key || ""
                            text: spec.label || spec.key || ""
                            property var defaultValue: spec.default !== undefined ? Boolean(spec.default) : undefined
                            property string onChangedFn: spec.onChanged || ""
                            property bool internalUpdate: false

                            Component.onCompleted: {
                                if (!root.backend || !key) return
                                var cur = root.backend.settings[key]
                                if ((cur === undefined || cur === null) && defaultValue !== undefined) {
                                    root.backend.setSettingValue(key, defaultValue)
                                    cur = defaultValue
                                }
                                internalUpdate = true
                                checked = Boolean(cur)
                                internalUpdate = false
                            }

                            onToggled: {
                                if (internalUpdate) return
                                if (!root.backend || !key) return
                                root.backend.setSettingValue(key, checked)
                                if (onChangedFn && onChangedFn.length > 0) {
                                    root.backend.callJsFunction(onChangedFn, [key, checked])
                                }
                            }
                        }
                    }

                    Component {
                        id: lineComp
                        RowLayout {
                            width: parent.width
                            spacing: 8

                            property string key: spec.key || ""
                            property string label: spec.label || spec.key || ""
                            property string placeholder: spec.placeholder || ""
                            property var defaultValue: spec.default !== undefined ? spec.default : undefined
                            property string onChangedFn: spec.onChanged || ""
                            property bool internalUpdate: false

                            Component.onCompleted: {
                                if (!root.backend || !key) return
                                var cur = root.backend.settings[key]
                                if ((cur === undefined || cur === null) && defaultValue !== undefined) {
                                    root.backend.setSettingValue(key, defaultValue)
                                    cur = defaultValue
                                }
                                internalUpdate = true
                                field.text = cur !== undefined ? String(cur) : ""
                                internalUpdate = false
                            }

                            CC.Label {
                                text: label
                                Layout.preferredWidth: 140
                                elide: Text.ElideRight
                            }

                            TextField {
                                id: field
                                Layout.fillWidth: true
                                placeholderText: placeholder

                                onEditingFinished: {
                                    if (internalUpdate) return
                                    if (!root.backend || !key) return
                                    root.backend.setSettingValue(key, text)
                                    if (onChangedFn && onChangedFn.length > 0) {
                                        root.backend.callJsFunction(onChangedFn, [key, text])
                                    }
                                }
                            }
                        }
                    }

                    Component {
                        id: comboComp
                        RowLayout {
                            width: parent.width
                            spacing: 8

                            property string key: spec.key || ""
                            property string label: spec.label || spec.key || ""
                            property var options: spec.options !== undefined ? spec.options : []
                            property var defaultValue: spec.default !== undefined ? spec.default : undefined
                            property string onChangedFn: spec.onChanged || ""
                            property bool internalUpdate: false

                            Component.onCompleted: {
                                if (!root.backend || !key) return
                                var cur = root.backend.settings[key]
                                if ((cur === undefined || cur === null) && defaultValue !== undefined) {
                                    root.backend.setSettingValue(key, defaultValue)
                                    cur = defaultValue
                                }
                                internalUpdate = true
                                if (cur !== undefined && cur !== null) {
                                    var idx = options.indexOf(String(cur))
                                    combo.currentIndex = idx >= 0 ? idx : 0
                                }
                                internalUpdate = false
                            }

                            CC.Label {
                                text: label
                                Layout.preferredWidth: 140
                                elide: Text.ElideRight
                            }

                            ComboBox {
                                id: combo
                                Layout.fillWidth: true
                                model: options

                                onActivated: {
                                    if (internalUpdate) return
                                    if (!root.backend || !key) return
                                    var t = currentText
                                    root.backend.setSettingValue(key, t)
                                    if (onChangedFn && onChangedFn.length > 0) {
                                        root.backend.callJsFunction(onChangedFn, [key, t])
                                    }
                                }
                            }
                        }
                    }

                    Component {
                        id: buttonComp
                        Button {
                            width: parent.width
                            text: spec.text || (spec.label || "按钮")
                            property string onClickedFn: spec.onClicked || ""

                            onClicked: {
                                if (!root.backend) return
                                if (onClickedFn && onClickedFn.length > 0) {
                                    root.backend.callJsFunction(onClickedFn, [])
                                } else {
                                    root.backend.runScript()
                                }
                            }
                        }
                    }

                    Component {
                        id: labelComp
                        CC.Label {
                            width: parent.width
                            text: spec.text || (spec.label || "")
                        }
                    }
                }
            }
        }
    }
}