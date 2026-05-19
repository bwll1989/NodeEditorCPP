import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: parent ? parent.width : 400
    height: btn.implicitHeight

    property var node
    property string text: "按钮"
    property string onClickedFn: ""

    Button {
        id: btn
        text: root.text
        onClicked: {
            if (!root.node) return
            if (root.onClickedFn && root.onClickedFn.length > 0) {
                root.node.callJsFunction(root.onClickedFn, [])
            } else {
                root.node.runScript()
            }
        }
    }
}