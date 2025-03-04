import QtQuick
import QtQuick.Controls

Item {
    id: root
    
    // 属性
    property var currentStage: null
    property real zoomFactor: 1.0
    property int centerX: 0
    property int centerY: 0
    
    // 显示当前帧图像
    Image {
        id: frameImage
        // 设置位置 - 使用 currentStage 中的 imagePosition
        x: (currentStage ? currentStage.imagePosition.x : 0) + root.centerX
        y: (currentStage ? currentStage.imagePosition.y : 0) + root.centerY
       
        // 使用图像的实际尺寸
        width: sourceSize.width >= 0 ? sourceSize.width : 1920
        height: sourceSize.height >= 0 ? sourceSize.height : 1080
        
        // 设置填充模式 - 使用 Stretch 确保图像填充整个区域
        fillMode: Image.Stretch
        cache: false
        
        // 设置变换原点
        transformOrigin: Item.TopLeft
        
        // 设置透明度
        opacity: 0.8  // 设置一定的透明度，以便能看到下面的网格
        
        // 设置层级
        z: 0  // 在 StageImages 内部，图像是唯一的元素，所以 z 值可以是 0
        
        // 只有在有有效图像时才显示
        visible: {
            if (!currentStage || !currentStage.currentFrame) {
                source = ""
                return false
            }
            return !currentStage.currentFrame.isNull
        }
  
        // 监听 currentStage 的 currentFrame 变化
        Connections {
            target: root.currentStage
            enabled: root.currentStage !== null
            
            function onCurrentFrameChanged() {
                if (!root.currentStage || !root.currentStage.currentFrame || root.currentStage.currentFrame.isNull) {
                    frameImage.source = ""
                    return
                }
                frameImage.source = "image://timeline/" + Date.now()
            }
        }

        // 添加源变化处理
        onSourceChanged: {
            if (source == "") {
                visible = false
            }
        }
    }
}
