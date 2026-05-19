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
    property string placeholder: ""
    property var defaultValue: undefined
    property string onChangedFn: ""

    function textFromSettings() {
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

        TextField {
            id: field
            Layout.fillWidth: true
            placeholderText: root.placeholder
            text: root.textFromSettings()

            onEditingFinished: {
                if (!root.node || !root.key) return
                root.node.setSettingValue(root.key, text)
                if (root.onChangedFn && root.onChangedFn.length > 0) {
                    root.node.callJsFunction(root.onChangedFn, [root.key, text])
                }
            }
        }
    }
}