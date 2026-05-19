import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: parent ? parent.width : 400
    height: box.implicitHeight

    property var node
    property string key: ""
    property string text: ""
    property var defaultValue: undefined
    property string onChangedFn: ""

    function checkedFromSettings() {
        if (!node || !key) return false
        var v = node.settings[key]
        if (v === undefined || v === null) return false
        return Boolean(v)
    }

    function ensureDefault() {
        if (!node || !key) return
        var v = node.settings[key]
        if ((v === undefined || v === null) && defaultValue !== undefined) {
            node.setSettingValue(key, Boolean(defaultValue))
        }
    }

    Component.onCompleted: ensureDefault()

    CheckBox {
        id: box
        text: root.text
        checked: root.checkedFromSettings()

        onToggled: {
            if (!root.node || !root.key) return
            root.node.setSettingValue(root.key, checked)
            if (root.onChangedFn && root.onChangedFn.length > 0) {
                root.node.callJsFunction(root.onChangedFn, [root.key, checked])
            }
        }
    }
}