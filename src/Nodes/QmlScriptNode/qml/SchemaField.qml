import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var spec
    required property var backend

    implicitWidth: loader.implicitWidth
    implicitHeight: loader.implicitHeight

    function commit(key, value, onChangedFn) {
        if (!backend || !key || backend.initializing) return
        backend.setSettingValue(key, value)
        if (onChangedFn && onChangedFn.length > 0) {
            backend.callJsFunction(onChangedFn, [key, value])
        }
    }

    function applyDefault(key, defaultValue) {
        if (!backend || !key || defaultValue === undefined) return false
        var cur = backend.settings[key]
        if (cur !== undefined && cur !== null) return false
        backend.applySettingDefault(key, defaultValue)
        return true
    }

    Loader {
        id: loader
        anchors.fill: parent
        property var s: spec

        sourceComponent: {
            if (!s || !s.type) return unknownComp
            switch (String(s.type)) {
            case "slider": return sliderComp
            case "spinbox": return spinComp
            case "checkbox": return checkComp
            case "lineedit": return lineComp
            case "combobox": return comboComp
            case "button": return buttonComp
            case "label": return labelComp
            default: return unknownComp
            }
        }
    }

    Component { id: unknownComp; Label { text: qsTr("未知控件: %1").arg(spec && spec.type ? spec.type : "?") } }

    Component {
        id: sliderComp
        RowLayout {
            property var s: loader.s
            property string key: s.key || ""
            property string label: s.label || key
            property real minVal: s.min !== undefined ? Number(s.min) : 0
            property real maxVal: s.max !== undefined ? Number(s.max) : 1
            property string onChangedFn: s.onChanged || ""
            property bool internalUpdate: false

            function fromSlider(v) {
                var span = maxVal - minVal
                return minVal + (v / slider.to) * span
            }

            Component.onCompleted: {
                if (!backend || !key) return
                var hadDefault = root.applyDefault(key, s.default)
                var cur = backend.settings[key]
                internalUpdate = true
                var span = maxVal - minVal
                slider.value = span === 0 ? 0 : ((Number(cur) - minVal) / span) * slider.to
                internalUpdate = false
            }

            Label { text: label; Layout.preferredWidth: 140; elide: Label.ElideRight }
            Slider {
                id: slider
                Layout.fillWidth: true
                from: 0
                to: 10000
                stepSize: 1
                onValueChanged: {
                    if (internalUpdate) return
                    root.commit(key, fromSlider(value), onChangedFn)
                }
            }
            Label {
                text: internalUpdate ? "" : String(fromSlider(slider.value).toFixed(3))
                Layout.preferredWidth: 72
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    Component {
        id: spinComp
        RowLayout {
            property var s: loader.s
            property string key: s.key || ""
            property string label: s.label || key
            property int minVal: s.min !== undefined ? Number(s.min) : 0
            property int maxVal: s.max !== undefined ? Number(s.max) : 100
            property int stepVal: s.step !== undefined ? Number(s.step) : 1
            property string onChangedFn: s.onChanged || ""
            property bool internalUpdate: false

            Component.onCompleted: {
                if (!backend || !key) return
                root.applyDefault(key, s.default)
                var cur = backend.settings[key]
                internalUpdate = true
                spin.value = cur !== undefined ? parseInt(cur) : minVal
                internalUpdate = false
            }

            Label { text: label; Layout.preferredWidth: 140; elide: Label.ElideRight }
            SpinBox {
                id: spin
                Layout.fillWidth: true
                from: minVal
                to: maxVal
                stepSize: stepVal
                onValueModified: {
                    if (internalUpdate) return
                    root.commit(key, value, onChangedFn)
                }
            }
        }
    }

    Component {
        id: checkComp
        CheckBox {
            property var s: loader.s
            property string key: s.key || ""
            property string onChangedFn: s.onChanged || ""
            property bool internalUpdate: false
            text: s.label || key

            Component.onCompleted: {
                if (!backend || !key) return
                root.applyDefault(key, s.default)
                internalUpdate = true
                checked = Boolean(backend.settings[key])
                internalUpdate = false
            }

            onToggled: {
                if (internalUpdate) return
                root.commit(key, checked, onChangedFn)
            }
        }
    }

    Component {
        id: lineComp
        RowLayout {
            property var s: loader.s
            property string key: s.key || ""
            property string label: s.label || key
            property string placeholder: s.placeholder || ""
            property string onChangedFn: s.onChanged || ""
            property bool internalUpdate: false

            Component.onCompleted: {
                if (!backend || !key) return
                root.applyDefault(key, s.default)
                internalUpdate = true
                field.text = backend.settings[key] !== undefined ? String(backend.settings[key]) : ""
                internalUpdate = false
            }

            Label { text: label; Layout.preferredWidth: 140; elide: Label.ElideRight }
            TextField {
                id: field
                Layout.fillWidth: true
                placeholderText: placeholder
                onEditingFinished: {
                    if (internalUpdate) return
                    root.commit(key, text, onChangedFn)
                }
            }
        }
    }

    Component {
        id: comboComp
        RowLayout {
            property var s: loader.s
            property string key: s.key || ""
            property string label: s.label || key
            property var options: s.options !== undefined ? s.options : []
            property string onChangedFn: s.onChanged || ""
            property bool internalUpdate: false

            Component.onCompleted: {
                if (!backend || !key) return
                root.applyDefault(key, s.default)
                var cur = backend.settings[key]
                internalUpdate = true
                if (cur !== undefined && cur !== null) {
                    var idx = options.indexOf(String(cur))
                    combo.currentIndex = idx >= 0 ? idx : 0
                }
                internalUpdate = false
            }

            Label { text: label; Layout.preferredWidth: 140; elide: Label.ElideRight }
            ComboBox {
                id: combo
                Layout.fillWidth: true
                model: options
                onActivated: {
                    if (internalUpdate) return
                    root.commit(key, currentText, onChangedFn)
                }
            }
        }
    }

    Component {
        id: buttonComp
        Button {
            property var s: loader.s
            property string onClickedFn: s.onClicked || ""
            width: parent ? parent.width : implicitWidth
            text: s.text || s.label || qsTr("按钮")

            onClicked: {
                if (!backend) return
                if (onClickedFn && onClickedFn.length > 0) {
                    backend.callJsFunction(onClickedFn, [])
                } else {
                    backend.runScript()
                }
            }
        }
    }

    Component {
        id: labelComp
        Label {
            property var s: loader.s
            width: parent ? parent.width : implicitWidth
            text: s.text || s.label || ""
            wrapMode: Text.WordWrap
        }
    }
}
