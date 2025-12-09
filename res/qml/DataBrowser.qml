import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQml.Models

/**
 * @brief 简化的数据浏览器组件，以树状图形式显示QVariantMap数据
 */
Rectangle {
    id: root
    color: "#2d2d2d"
    
    // 默认直接展开全部
    property bool autoExpandAll: true
    
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
        
        // 移除顶部工具栏（展开全部），保持布局占位为 0 高度
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 0
        }
        
        // 树状视图 - 直接显示，去除表头
        TreeView {
            id: treeView
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            // 使用从C++传递的dataModel实例
            model: dataModel
            
            /**
             * @brief 选择模型：用于管理当前项和选中项
             * 绑定到 TreeView 的 model，避免 selectionModel 为空导致操作失败
             */
            selectionModel: ItemSelectionModel {
                model: treeView.model
            }
            
            // 监听模型重置信号
            Connections {
                target: dataModel
                function onModelReset() {
                    if (autoExpandAll) {
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
                    property int column1Width: totalWidth * 0.4  // 40% - 比例2
                    property int column2Width: totalWidth * 0.2  // 20% - 比例1
                    property int column3Width: totalWidth * 0.4  // 40% - 比例2
                    
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
                    
                    /**
                     * @brief 单击选择当前行，并转发点击事件
                     * 增加判空保护，避免 selectionModel 为空时报错
                     */
                    onClicked: function(mouse) {
                        if (treeView.selectionModel) {
                            treeView.selectionModel.setCurrentIndex(
                                treeView.index(row, 0),
                                ItemSelectionModel.ClearAndSelect
                            )
                        }
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