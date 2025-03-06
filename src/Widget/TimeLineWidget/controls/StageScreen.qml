import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

StageControlBase {
    id: root
    width: screenWidth * root.zoomFactor
    height: screenHeight * root.zoomFactor
    color: "transparent"  // Make background transparent
    
    property int screenWidth: 1920
    property int screenHeight: 1080
    property string resolution: screenWidth + "x" + screenHeight
    property string name: "显示 1"
    property int posX: 0
    property int posY: 0
    property real scale: 0.5  // Add scale property
    property bool isSelected: false // Add selected state property
    property string positionText: ""  // 添加位置文本属性
    signal clicked()  // 添加点击信号
    
    // 添加鼠标悬停状态
    property bool isHovered: false
    
    // Make the control draggable and resizable
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true  // 启用悬停检测
        drag.target: root.isSelected ? parent : null
        drag.threshold: 0
        preventStealing: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        z: 2
        
        onEntered: root.isHovered = true
        onExited: root.isHovered = false
        
        onPressed: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                root.clicked();  // 立即选中
            }
        }
        
        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                contextMenu.popup();
            }
            mouse.accepted = true;
        }
        
        onDoubleClicked: function(mouse) {
            mouse.accepted = true;
            modelData.showProperties();  // 使用C++方法显示属性面板
        }
        
        onPositionChanged: function(mouse) {
            if (drag.active && root.isSelected) {  // 确保只有选中时才更新位置
                mouse.accepted = true;
                posX = parent.x;
                posY = parent.y;
            }
        }

        // 根据状态更新鼠标指针
        cursorShape: {
            if (root.isSelected) return Qt.SizeAllCursor;
            if (root.isHovered) return Qt.PointingHandCursor;
            return Qt.ArrowCursor;
        }
    }
    
    // Main rectangle frame
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        
        // 根据状态设置边框颜色
        border.color: {
            if (root.isSelected) {
                return currentStage.screenSelectedColor;
            } else if (root.isHovered) {
                return currentStage.screenHoverColor;
            }
            return currentStage.screenNormalColor;
        }
        
        // 根据状态设置边框宽度
        border.width: {
            if (root.isSelected) {
                return currentStage.screenBorderSelectedWidth;
            } else if (root.isHovered) {
                return currentStage.screenBorderHoverWidth;
            }
            return currentStage.screenBorderNormalWidth;
        }
        
        // 根据状态设置不透明度
        opacity: {
            if (root.isSelected) return 1.0;
            if (root.isHovered) return 0.9;
            return 0.8;
        }
        
        radius: 2

        // 选中时的虚线边框
        Canvas {
            anchors.fill: parent
            visible: root.isSelected
            onPaint: {
                var ctx = getContext("2d");
                ctx.strokeStyle = currentStage.screenSelectedColor;
                ctx.lineWidth = currentStage.screenBorderSelectedWidth;
                ctx.setLineDash([5, 5]);
                ctx.strokeRect(0, 0, width, height);
            }
        }
        
        // Screen name
        Column {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 5
            spacing: 2
            
            Text {
                text: root.name
                color: "#FFFFFF"
                font.pixelSize: 25
            }
            
            Text {
                text: root.positionText
                color: "#AAAAAA"
                font.pixelSize: 20
            }
        }
        
        // Resolution text
        Text {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 5
            text: resolution
            color: "#ffffff"
            font.pixelSize: 20
        }
    }
    
    // Context menu
    Menu {
        id: contextMenu
        
        MenuItem {
            text: qsTr("属性")
            icon.source: "qrc:/icons/icons/property.png"
            onTriggered: modelData.showProperties()  // 使用C++方法显示属性面板
        }
        
        MenuItem {
            text: qsTr("删除")
            icon.source: "qrc:/icons/icons/trash.png"
            onTriggered: root.deleteRequested()
        }
    }
    
    // 添加删除信号
    signal deleteRequested()
    
    // 添加状态过渡动画
    Behavior on opacity { NumberAnimation { duration: 200 } }
    Behavior on border.width { NumberAnimation { duration: 200 } }
} 