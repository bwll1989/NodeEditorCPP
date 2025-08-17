import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

/**
 * @brief 简化的数据浏览器组件，以树状图形式显示QVariantMap数据
 */
Rectangle {
    id: root
    color: "#2d2d2d"
    
    // 添加展开状态属性
    property bool autoExpandAll: false
    
    signal itemClicked(var item)
    signal itemDoubleClicked(var item)
    signal itemRightClicked(var item)
    
    /**
     * @brief 设置数据模型
     * @param data QVariantMap格式的数据
     */
    function setData(data) {
        if (typeof dataModel !== 'undefined') {
            dataModel.setRootData(data)
        }
    }
    
    /**
     * @brief 增量更新数据
     * @param newData 新的数据
     */
    function updateData(newData) {
        if (typeof dataModel !== 'undefined') {
            dataModel.updateData(newData)
        }
    }
    
    /**
     * @brief 展开所有项目
     */
    function expandAll() {
        treeView.expandRecursively(-1)
    }
    
    /**
     * @brief 折叠所有项目
     */
    function collapseAll() {
        treeView.collapseRecursively(-1)
    }
    
    /**
     * @brief 根据数据类型获取对应的颜色
     * @param type 数据类型字符串
     * @return 对应的颜色值
     */
    function getTypeColor(type) {
        switch(type) {
            case "QString": return "#4CAF50"
            case "int": case "double": case "float": case "qreal": return "#2196F3"
            case "bool": return "#FF9800"
            case "QVariantMap": case "QVariantList": return "#9C27B0"
            case "QDateTime": return "#607D8B"
            case "QUrl": return "#00BCD4"
            case "QColor": return "#E91E63"
            default: return "#757575"
        }
    }
    
    /**
     * @brief 更新统计信息（空实现，保持兼容性）
     */
    function updateStatistics() {
        // 空实现，因为当前简化版本不需要统计功能
        // 这个方法存在是为了保持与C++代码的兼容性
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 简单的工具栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#1a1a1a"
            border.color: "#333333"
            border.width: 1
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8
                
                // 展开全部单选框 - 改进样式
                CheckBox {
                    id: expandAllCheckBox
                    text: "展开全部"
                    checked: root.autoExpandAll
                    Layout.preferredHeight: 25
                    
                    onCheckedChanged: {
                        root.autoExpandAll = checked
                        if (checked) {
                            root.expandAll()
                        } else {
                            root.collapseAll()
                        }
                    }
                    
                    // 自定义样式 - 更统一的风格
                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        x: expandAllCheckBox.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 3
                        border.color: {
                            if (expandAllCheckBox.checked) return "#4CAF50"
                            if (expandAllCheckBox.hovered) return "#666666"
                            return "#555555"
                        }
                        border.width: 2
                        color: {
                            if (expandAllCheckBox.checked) return "#4CAF50"
                            if (expandAllCheckBox.hovered) return "#333333"
                            return "#2d2d2d"
                        }
                        
                        // 勾选标记
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: "#ffffff"
                            font.pixelSize: 12
                            font.bold: true
                            visible: expandAllCheckBox.checked
                        }
                    }
                    
                    contentItem: Text {
                        text: expandAllCheckBox.text
                        font.pixelSize: 12
                        color: expandAllCheckBox.hovered ? "#ffffff" : "#BDBDBD"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: expandAllCheckBox.indicator.width + expandAllCheckBox.spacing + 4
                    }
                    
                    // 添加悬停效果
                    hoverEnabled: true
                }
                
                Item { Layout.fillWidth: true }
            }
        }
        
        // 树状视图 - 直接显示，去除表头
        TreeView {
            id: treeView
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            // 使用从C++传递的dataModel实例
            model: dataModel
            
            // 监听模型重置信号
            Connections {
                target: dataModel
                function onModelReset() {
                    if (autoExpandAll) {
                        // 减少延迟，使用单次 callLater
                        Qt.callLater(function() {
                            expandAll()
                        })
                    }
                }
            }
            
            // 自定义委托
            delegate: TreeViewDelegate {
                id: treeDelegate
                
                implicitWidth: treeView.width
                implicitHeight: 28
                
                Rectangle {
                    anchors.fill: parent
                    color: {
                        if (treeDelegate.current) return "#404040"
                        if (treeDelegate.hovered) return "#353535"
                        return index % 2 === 0 ? "#2d2d2d" : "#323232"
                    }
                    border.color: treeDelegate.current ? "#4CAF50" : "transparent"
                    border.width: 1
                    
                    // 计算列宽 - 比例 2:1:7
                    property int totalWidth: parent.width - 10 // 减去边距
                    property int column1Width: totalWidth * 0.2  // 40% - 比例2
                    property int column2Width: totalWidth * 0.1  // 20% - 比例1
                    property int column3Width: totalWidth * 0.7  // 40% - 比例2
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 0
                        
                        // 第一列：键名区域
                        Rectangle {
                            Layout.preferredWidth: parent.parent.column1Width
                            Layout.fillHeight: true
                            color: "transparent"
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: depth * 20
                                spacing: 5
                                
                                // 展开/折叠指示器
                                Rectangle {
                                    width: 16
                                    height: 16
                                    color: expandMouseArea.containsMouse ? "#404040" : "transparent"
                                    radius: 2
                                    visible: hasChildren > 0
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: expanded ? "▼" : "▶"
                                        color: "#ffffff"
                                        font.pixelSize: 10
                                    }
                                    
                                    MouseArea {
                                        id: expandMouseArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: treeView.toggleExpanded(row)
                                    }
                                }
                                
                                // 键名文本
                                Text {
                                    Layout.fillWidth: true
                                    text: model.key || "未知键"
                                    color: "#ffffff"
                                    font.bold: hasChildren > 0
                                    font.pixelSize: 13
                                    elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                        
                        // 分隔线
                        Rectangle {
                            Layout.preferredWidth: 1
                            Layout.fillHeight: true
                            color: "#333333"
                        }
                        
                        // 第二列：类型区域
                        Rectangle {
                            Layout.preferredWidth: parent.parent.column2Width
                            Layout.fillHeight: true
                            color: "transparent"
                            
                            Text {
                                anchors.centerIn: parent
                                text: model.valueType || ""
                                color: "#BDBDBD"
                                font.pixelSize: 11
                                visible: model.valueType && model.valueType !== ""
                            }
                        }
                        
                        // 分隔线
                        Rectangle {
                            Layout.preferredWidth: 1
                            Layout.fillHeight: true
                            color: "#333333"
                        }
                        
                        // 第三列：值区域
                        Rectangle {
                            Layout.preferredWidth: parent.parent.column3Width
                            Layout.fillHeight: true
                            color: "transparent"
                            
                            Text {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: 5
                                text: model.displayValue || ""
                                color: "#BDBDBD"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                // 移除 hasChildren 条件，让所有节点都能显示值
                                visible: model.displayValue && model.displayValue !== ""
                            }
                        }
                    }
                }
                
                // 鼠标交互
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    hoverEnabled: true
                    
                    onClicked: function(mouse) {
                        treeView.selectionModel.setCurrentIndex(treeView.index(row, 0), ItemSelectionModel.ClearAndSelect)
                        
                        if (mouse.button === Qt.LeftButton) {
                            root.itemClicked(model)
                        } else if (mouse.button === Qt.RightButton) {
                            root.itemRightClicked(model)
                        }
                    }
                    
                    onDoubleClicked: function(mouse) {
                        if (mouse.button === Qt.LeftButton) {
                            root.itemDoubleClicked(model)
                            if (hasChildren > 0) {
                                treeView.toggleExpanded(row)
                            }
                        }
                    }
                }
            }
        }
    }
}