import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    width: 100
    height: 40
    color: "#252525"
    border.color: "#454545"
    border.width: 1
    radius: 4

    property string controlName: "Control"
    property bool isEditing: false

    // Make control draggable
    MouseArea {
        anchors.fill: parent
        drag.target: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        
        onClicked: function(mouse){
            if (mouse.button === Qt.RightButton)
                controlMenu.popup()
        }
    }

    // Control context menu
    Menu {
        id: controlMenu
        
        MenuItem {
            text: "Rename"
            icon.source: "qrc:/icons/icons/property.png"
            onTriggered: {
                root.isEditing = true;
                nameInput.forceActiveFocus();
            }
        }
        
        MenuItem {
            text: "Delete"
            icon.source: "qrc:/icons/icons/trash.png"
            onTriggered: root.destroy()
        }
    }


} 