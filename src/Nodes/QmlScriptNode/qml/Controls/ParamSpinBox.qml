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
    property int from: 0
    property int to: 100
    property int step: 1
    property var defaultValue: undefined
    property string onChangedFn: ""

    function valueFromSettings() {
        if (!node || !key) return from
        var v = node.settings[key]
        if (v === undefined || v === null || v === "") return from
        return parseInt(v)
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

        SpinBox {
            id: spin
            Layout.fillWidth: true
            from: root.from
            to: root.to
            stepSize: root.step
            value: root.valueFromSettings()

            onValueModified: {
                if (!root.node || !root.key) return
                root.node.setSettingValue(root.key, value)
                if (root.onChangedFn && root.onChangedFn.length > 0) {
                    root.node.callJsFunction(root.onChangedFn, [root.key, value])
                }
            }
        }
    }
}