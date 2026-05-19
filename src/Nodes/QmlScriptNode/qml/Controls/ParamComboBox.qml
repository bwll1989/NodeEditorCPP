import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: parent ? parent.width : 400
    height: row.implicitHeight

    property var node
    property string key: ""
    property string label: ""
    property var options: []
    property var defaultValue: undefined
    property string onChangedFn: ""

    function currentTextFromSettings() {
        if (!node || !key) return ""
        var v = node.settings[key]
        if (v === undefined || v === null) return ""
        return String(v)
    }

    function ensureDefault() {
        if (!node || !key) return
        var v = node.settings[key]
        if ((v === undefined || v === null) && defaultValue !== undefined) {
            node.setSettingValue(key, defaultValue)
        }
    }

    Component.onCompleted: ensureDefault()

    RowLayout {
        id: row
        width: parent.width
        spacing: 8

        Label {
            text: label
            Layout.preferredWidth: 140
            elide: Label.ElideRight
        }

        ComboBox {
            id: combo
            Layout.fillWidth: true
            model: root.options

            Component.onCompleted: {
                var t = root.currentTextFromSettings()
                if (!t) return
                var idx = root.options.indexOf(t)
                if (idx >= 0) currentIndex = idx
            }

            onActivated: {
                if (!root.node || !root.key) return
                var t = currentText
                root.node.setSettingValue(root.key, t)
                if (root.onChangedFn && root.onChangedFn.length > 0) {
                    root.node.callJsFunction(root.onChangedFn, [root.key, t])
                }
            }
        }
    }
}