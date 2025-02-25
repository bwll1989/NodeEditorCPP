import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    color: "#2d2d2d"

    TableView {
        id: tableView
        anchors.fill: parent
        clip: true
        
        model: logModel
        
        delegate: Rectangle {
            implicitWidth: tableView.width
            implicitHeight: 30
            color: "transparent"
            
            Row {
                spacing: 5
                padding: 5
                
                Text {
                    text: time
                    color: "#cccccc"
                    width: 100
                }
                
                Text {
                    text: type
                    color: {
                        switch(type) {
                            case "Error": return "#ff6b6b"
                            case "Warning": return "#ffd93d"
                            default: return "#6bff6b"
                        }
                    }
                    width: 80
                }
                
                Text {
                    text: message
                    color: "#ffffff"
                    width: tableView.width - 190
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    function clear() {
        logModel.clear()
    }
} 