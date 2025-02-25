import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#2d2d2d"

    TableView {
        id: tableView
        anchors.fill: parent
        clip: true
        
        columnSpacing: 1
        rowSpacing: 1
        
        model: logModel
        
        property var columnWidths: [100, 80, 150, 400]
        
        columnWidthProvider: function(column) {
            return columnWidths[column]
        }
        
        delegate: Rectangle {
            implicitHeight: 30
            color: "transparent"
            
            Text {
                anchors.fill: parent
                anchors.margins: 5
                text: display
                color: {
                    if (column === 1) { // Level column
                        switch(display) {
                            case "Debug": return "#6bff6b"
                            case "Warn": return "#ffd93d"
                            case "Critical": return "#ff6b6b"
                            case "Info": return "#cccccc"
                            case "Fatal": return "#ff0000"
                            default: return "#ffffff"
                        }
                    }
                    return "#ffffff"
                }
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }

        Row {
            y: -height
            z: 2
            Repeater {
                model: ["Timestamp", "Level", "File", "Message"]
                
                Rectangle {
                    width: tableView.columnWidths[index]
                    height: 30
                    color: "#1a1a1a"
                    
                    Text {
                        anchors.fill: parent
                        anchors.margins: 5
                        text: modelData
                        color: "#ffffff"
                        font.bold: true
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    // 右键菜单
    Menu {
        id: contextMenu
        
        MenuItem {
            text: "Clear Log"
            icon.source: "qrc:/icons/icons/clear.png"
            onTriggered: logModel.clear()
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            if (mouse.button === Qt.RightButton)
                contextMenu.popup()
        }
    }
} 