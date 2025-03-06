import QtQuick
import QtQuick.Controls
// 导入当前目录，使StageScreen可用
import "."  
Rectangle {
    id: root
    color: stage ? stage.stageBgColor : "#333333"  // 添加空检查
    width: parent ? parent.width : 800
    height: parent ? parent.height : 600
    clip: true  // Add clipping to prevent controls from drawing outside
    
    property int currentFrame: 0
    property int totalFrames: 100
    property bool isPlaying: false
    property int screenCount: 0
    property real zoomFactor: stage ? stage.zoomFactor : 0.5  // 设置默认缩放为 0.5
    onZoomFactorChanged: {
        if (stage) {
            stage.zoomFactor = zoomFactor;
        }
    }
    
    // 使用C++中的stage对象，保持原有名称但添加空检查
    property var currentStage: stage ? stage : null
    
    // 添加鼠标位置属性
    property point mousePos: Qt.point(0, 0)
    property bool containsMouse: false  // 添加这个属性
    
    // 添加当前选中的screen属性
    property var selectedScreen: null
    
    // 添加键盘事件处理
    focus: true
    Keys.onPressed: function(event) {
        if (!selectedScreen) return;
        // console.log(event.key)
        
        // 添加 Control 修饰键检查
        if (event.modifiers & Qt.AltModifier) {
            switch(event.key) {
                case Qt.Key_Left:
                    selectedScreen.x -= 1;
                    event.accepted = true;
                    break;
                case Qt.Key_Right:
                    selectedScreen.x += 1;
                    event.accepted = true;
                    break;
                case Qt.Key_Up:
                    selectedScreen.y -= 1;
                    event.accepted = true;
                    break;
                case Qt.Key_Down:
                    selectedScreen.y += 1;
                    event.accepted = true;
                    break;
                case Qt.Key_Delete:
                    if (currentStage && selectedScreen) {
                        currentStage.removeScreen(selectedScreen.modelData);
                        selectedScreen = null;
                        event.accepted = true;
                    }
                    break;
            }
        }
    }
    
    // 在 TimelineStage.qml 中添加 StageImages 组件
    StageImages {
        id: stageImages
        parent: controlsContainer
        anchors.fill: parent
        currentStage: root.currentStage
        zoomFactor: root.zoomFactor
        centerX: gridCanvas.centerX
        centerY: gridCanvas.centerY
        z: 1  // 图像在网格之上，但在 screen 之下
    }

    // Grid and guidelines
    Canvas {
        id: gridCanvas
        anchors.fill: parent
        z: 0  // 确保网格在最底层
        
        property int centerX: width / 2
        property int centerY: height / 2
        property point origin: Qt.point(controlsContainer.x + centerX * root.zoomFactor, 
                                      controlsContainer.y + centerY * root.zoomFactor)
        
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            
            // // Draw grid
            // ctx.strokeStyle = "#252525";
            // ctx.lineWidth = 1;
            
            // // Calculate grid size based on zoom
            // var gridSize = 50 * root.zoomFactor;
            
            // // Calculate visible area boundaries
            // var startX = -controlsContainer.x / root.zoomFactor;
            // var startY = -controlsContainer.y / root.zoomFactor;
            // var endX = (width - controlsContainer.x) / root.zoomFactor;
            // var endY = (height - controlsContainer.y) / root.zoomFactor;
            
            // // Adjust grid starting positions
            // var firstX = Math.floor(startX / gridSize) * gridSize;
            // var firstY = Math.floor(startY / gridSize) * gridSize;
            
            // // Draw vertical lines
            // for(var x = firstX; x <= endX; x += gridSize) {
            //     var screenX = x * root.zoomFactor + controlsContainer.x;
            //     ctx.beginPath();
            //     ctx.moveTo(screenX, 0);
            //     ctx.lineTo(screenX, height);
            //     ctx.stroke();
            // }
            
            // // Draw horizontal lines
            // for(var y = firstY; y <= endY; y += gridSize) {
            //     var screenY = y * root.zoomFactor + controlsContainer.y;
            //     ctx.beginPath();
            //     ctx.moveTo(0, screenY);
            //     ctx.lineTo(width, screenY);
            //     ctx.stroke();
            // }
            
            // Draw center crosshair
            ctx.strokeStyle = "#404040";
            ctx.lineWidth = 2;
            
            // Vertical center line
            ctx.beginPath();
            ctx.moveTo(origin.x, 0);
            ctx.lineTo(origin.x, height);
            ctx.stroke();
            
            // Horizontal center line
            ctx.beginPath();
            ctx.moveTo(0, origin.y);
            ctx.lineTo(width, origin.y);
            ctx.stroke();
            
            // Draw origin marker
            ctx.strokeStyle = "#ff0000";
            ctx.lineWidth = 2;
            
            // Cross at origin
            ctx.beginPath();
            ctx.moveTo(origin.x - 10, origin.y);
            ctx.lineTo(origin.x + 10, origin.y);
            ctx.moveTo(origin.x, origin.y - 10);
            ctx.lineTo(origin.x, origin.y + 10);
            ctx.stroke();
            
            // Draw coordinates at origin
            ctx.fillStyle = "#ff0000";
            ctx.font = "12px sans-serif";
            ctx.fillText("(0,0)", origin.x + 15, origin.y - 5);
            
            // 绘制坐标轴，降低透明度
            ctx.globalAlpha = 0.5;  // 设置透明度为50%
            ctx.lineWidth = currentStage.stageAxisWidth;
            
            // X轴
            ctx.strokeStyle = currentStage.stageAxisXColor;
            ctx.beginPath();
            ctx.moveTo(0, origin.y);
            ctx.lineTo(width, origin.y);
            ctx.stroke();
            
            // Y轴
            ctx.strokeStyle = currentStage.stageAxisYColor;
            ctx.beginPath();
            ctx.moveTo(origin.x, 0);
            ctx.lineTo(origin.x, height);
            ctx.stroke();
            
            // 恢复透明度
            ctx.globalAlpha = 1.0;
            
            // 绘制原点标记
            ctx.beginPath();
            ctx.arc(origin.x, origin.y, 5, 0, 2 * Math.PI);
            ctx.fillStyle = "#FFFFFF";
            ctx.fill();
            
            // 绘制刻度
            ctx.font = "12px Arial";
            
            // // X轴刻度 (红色)
            // ctx.fillStyle = "#FF4444";
            // ctx.textAlign = "center";
            // ctx.textBaseline = "top";
            // for(var x = -10; x <= 10; x++) {
            //     var xPos = origin.x + x * 50 * root.zoomFactor;
            //     ctx.beginPath();
            //     ctx.moveTo(xPos, origin.y - 5);
            //     ctx.lineTo(xPos, origin.y + 5);
            //     ctx.stroke();
            //     ctx.fillText(x * 50, xPos, origin.y + 10);
            // }
            
            // // Y轴刻度 (绿色)
            // ctx.fillStyle = "#44FF44";
            // ctx.textAlign = "right";
            // ctx.textBaseline = "middle";
            // for(var y = -10; y <= 10; y++) {
            //     var yPos = origin.y + y * 50 * root.zoomFactor;
            //     ctx.beginPath();
            //     ctx.moveTo(origin.x - 5, yPos);
            //     ctx.lineTo(origin.x + 5, yPos);
            //     ctx.stroke();
            //     ctx.fillText(y * 50, origin.x - 10, yPos);  // 移除负号，使坐标系与屏幕一致
            // }
        }
        
        // Redraw when container position or zoom changes
        Connections {
            target: controlsContainer
            function onXChanged() { gridCanvas.requestPaint() }
            function onYChanged() { gridCanvas.requestPaint() }
        }
        
        // Redraw when size changes
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    // Container for dynamic controls
    Item {
        id: controlsContainer
        width: root.width
        height: root.height
        x: gridCanvas.centerX * (1 - root.zoomFactor)
        y: gridCanvas.centerY * (1 - root.zoomFactor)
        z: 2  // 确保 screen 在最上层
        scale: root.zoomFactor
        transformOrigin: Item.TopLeft
        
        // 添加拖拽功能
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.MiddleButton
            onPressed: {
                drag.target = controlsContainer
            }
            onReleased: {
                drag.target = null
            }
        }
        
        // 位置变化时保存状态
        onXChanged: if (currentStage) currentStage.viewPosition = Qt.point(x, y)
        onYChanged: if (currentStage) currentStage.viewPosition = Qt.point(x, y)
        
        // 初始化时恢复位置
        Component.onCompleted: {
            if (currentStage && currentStage.viewPosition !== Qt.point(0, 0)) {
                x = currentStage.viewPosition.x;
                y = currentStage.viewPosition.y;
            }
        }
    }

    // Background mouse area for panning and context menu
    MouseArea {
        id: backgroundMouseArea
        anchors.fill: parent
        acceptedButtons: Qt.RightButton | Qt.LeftButton
        z: 0  // Keep background mouse area behind controls
        property point lastPos
        
        onPressed: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                lastPos = Qt.point(mouse.x, mouse.y)
            }
        }
        
        onPositionChanged: function(mouse) {
            if (mouse.buttons & Qt.LeftButton) {
                var delta = Qt.point(mouse.x - lastPos.x, mouse.y - lastPos.y)
                controlsContainer.x += delta.x
                controlsContainer.y += delta.y
                lastPos = Qt.point(mouse.x, mouse.y)
                gridCanvas.requestPaint()
            }
        }
        
        onClicked: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                root.selectedScreen = null;  // 点击背景取消选择
            } else if (mouse.button === Qt.RightButton) {
                contextMenu.x = mouse.x;
                contextMenu.y = mouse.y;
                contextMenu.popup();
            }
        }
        
        // Add zoom functionality
        onWheel: function(wheel) {
            if (wheel.modifiers & Qt.ControlModifier) {
                var oldZoom = root.zoomFactor;
                if (wheel.angleDelta.y > 0)
                    root.zoomFactor *= 1.1;
                else
                    root.zoomFactor *= 0.9;
                    
                // Limit zoom range
                root.zoomFactor = Math.min(Math.max(root.zoomFactor, 0.1), 10.0);
                
                // Adjust container position to keep mouse position fixed
                var mouseX = wheel.x - controlsContainer.x;
                var mouseY = wheel.y - controlsContainer.y;
                var scaleFactor = root.zoomFactor / oldZoom;
                
                controlsContainer.x += mouseX * (1 - scaleFactor);
                controlsContainer.y += mouseY * (1 - scaleFactor);
                
                gridCanvas.requestPaint();
            }
        }
    }

    // Context Menu
    Menu {
        id: contextMenu
        
        Menu {
            title: "Add Display"
            MenuItem {
                text: "Screen"
                icon.source: "qrc:/icons/icons/screen.png"
                onTriggered: {
                    screenCount++;
                    root.addControl("screen", contextMenu.x, contextMenu.y, "显示 " + screenCount);
                }
            }
        }
        
        MenuSeparator { }
        
        MenuItem {
            text: "视图归零"
            icon.source: "qrc:/icons/icons/center.png"
            onTriggered: root.resetViewToOrigin()
        }
        
        MenuItem {
            text: "清空所有"
            icon.source: "qrc:/icons/icons/clear.png"
            onTriggered: root.clearControls()
        }
    }
    
    // Methods to control the stage
    function updateFrame(frame) {
        root.currentFrame = frame;
    }
    
    function setPlaying(playing) {
        root.isPlaying = playing;
    }

    // Method to add a new control
    function addControl(type, x, y, name) {
        if (type === "screen") {
            if (currentStage) {
                // 将点击位置转换为相对于原点(0,0)的坐标
                var relX = Math.round((x - controlsContainer.x) / root.zoomFactor - gridCanvas.centerX);
                var relY = Math.round((y - controlsContainer.y) / root.zoomFactor - gridCanvas.centerY);
                
                var screen = Qt.createQmlObject('import TimelineWidget 1.0; TimelineScreen {}', currentStage);
                screen.name = name || "显示";
                screen.posX = relX;
                screen.posY = relY;
                currentStage.addScreen(screen);
            }
        }
    }

    // Method to clear all controls
    function clearControls() {
        screenCount = 0;
        if (currentStage) {
            currentStage.clearScreens();
        }
    }

    // 监听stage的screens变化
    Connections {
        target: currentStage
        function onScreensChanged() {
            // 强制更新Repeater
            screenRepeater.model = null;
            screenRepeater.model = currentStage ? currentStage.getScreens() : [];
        }
    }
    
    // 添加一个专门用于存放 screens 的容器
    Item {
        id: screensContainer
        parent: controlsContainer
        anchors.fill: parent
        z: 2  // 确保 screen 在最上层
        
        // 给Repeater添加id
        Repeater {
            id: screenRepeater
            model: currentStage ? currentStage.getScreens() : []
            delegate: StageScreen {
                id: screenItem
                parent: screensContainer  // 确保所有screen都是screensContainer的子项
                name: modelData.name
                screenWidth: modelData.screenWidth
                screenHeight: modelData.screenHeight
                x: modelData.posX + gridCanvas.centerX
                y: modelData.posY + gridCanvas.centerY
                rotation: modelData.rotation
                isSelected: root.selectedScreen === screenItem
                
                // 添加点击选择处理
                onClicked: {
                    root.selectedScreen = screenItem;
                    root.forceActiveFocus();  // 确保可以接收键盘事件
                }
                
                // 添加位置显示
                positionText: "X: " + modelData.posX + ", Y: " + modelData.posY

                // 修改拖拽位置更新
                onXChanged: {
                    if (currentStage) {
                        var relX = Math.round(x - gridCanvas.centerX);
                        modelData.posX = relX;
                    }
                }
                onYChanged: {
                    if (currentStage) {
                        var relY = Math.round(y - gridCanvas.centerY);
                        modelData.posY = relY;
                    }
                }

                // 删除处理保持不变
                onDeleteRequested: {
                    if (currentStage) {
                        currentStage.removeScreen(modelData);
                    }
                }

                // 添加对 modelData 的引用
                property var modelData: model.modelData

                // 确保宽度和高度正确应用缩放因子
                width: modelData.screenWidth
                height: modelData.screenHeight
            }
        }
    }

    // 鼠标位置提示
    Rectangle {
        id: mousePositionTip
        color: "#333333"
        border.color: "#666666"
        border.width: 1
        radius: 5
        width: positionText.width + 20
        height: positionText.height + 10
        visible: root.containsMouse
        
        // 跟随鼠标位置
        x: root.mousePos.x + 15
        y: root.mousePos.y + 15
        
        Text {
            id: positionText
            anchors.centerIn: parent
            color: "#FFFFFF"
            text: {
                // 计算鼠标在stage坐标系中的位置
                var stageX = Math.round((root.mousePos.x - controlsContainer.x) / root.zoomFactor - gridCanvas.centerX);
                var stageY = Math.round((root.mousePos.y - controlsContainer.y) / root.zoomFactor - gridCanvas.centerY);
                return "X: " + stageX + ", Y: " + stageY;
            }
        }
    }
    
    // 处理鼠标事件
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton  // 只处理移动，不处理点击
        
        onPositionChanged: {
            root.mousePos = Qt.point(mouseX, mouseY);
        }
        
        onEntered: {
            root.containsMouse = true;
        }
        
        onExited: {
            root.containsMouse = false;
        }
    }

    // 添加重置视图位置的函数
    function resetViewToOrigin() {
        // 计算将原点移动到中心所需的位置
        controlsContainer.x = gridCanvas.centerX * (1 - root.zoomFactor);
        controlsContainer.y = gridCanvas.centerY * (1 - root.zoomFactor);
    }
} 