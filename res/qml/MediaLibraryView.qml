import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 媒体库视图（QML）
// - 顶部工具栏：导入文件/导入文件夹/删除选中/清空
// - 主体：按类别分组显示，每组可折叠，子项列表支持双击打开、右键菜单、批量删除
// - 依赖 MediaLibraryModel 的角色：display、path、category、ordinal（已在 C++ roleNames 中暴露）

Item {
    id: root
    width: 800
    height: 600

    // 对外转发文件激活信号（C++ MediaLibraryWidget 已连接）
    signal fileActivated(string path)

    // 收集当前选择的路径，供“删除选中”调用
    property var selectedPaths: []

    // 函数级注释：添加选中路径（去重）
    function addSelectedPath(p) {
        if (!p || p.length === 0) return;
        if (selectedPaths.indexOf(p) < 0) selectedPaths.push(p);
    }
    // 函数级注释：移除选中路径
    function removeSelectedPath(p) {
        if (!p || p.length === 0) return;
        var i = selectedPaths.indexOf(p);
        if (i >= 0) selectedPaths.splice(i, 1);
    }
    // 函数级注释：清空选中路径集合
    function clearSelectionPaths() { selectedPaths = []; }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 顶部工具栏
        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                spacing: 6
                anchors.fill: parent

                ToolButton {
                    text: qsTr("导入文件")
                    onClicked: libraryBridge.importFiles()
                }
                ToolButton {
                    text: qsTr("导入文件夹")
                    onClicked: libraryBridge.importFolder()
                }
                ToolButton {
                    text: qsTr("删除选中")
                    onClicked: {
                        if (root.selectedPaths.length > 0)
                            libraryBridge.deleteItems(root.selectedPaths)
                    }
                }
                ToolButton {
                    text: qsTr("清空")
                    onClicked: {
                        libraryBridge.clearLibrary()
                        root.clearSelectionPaths()
                    }
                }
                Item { Layout.fillWidth: true } // 占位拉伸
            }
        }

        // 主体：分组 + 子项列表（可折叠）
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Column {
                id: groupColumn
                width: parent.width
                spacing: 0

                Repeater {
                    id: groupRepeater
                    model: mediaLibraryModel

                    delegate: Item {
                        id: section
                        width: groupColumn.width
                        // 折叠状态
                        property bool expanded: true
                        // 分组的顶层行号
                        property int groupRow: index

                        Column {
                            width: parent.width
                            spacing: 0

                            // 头部：类别名 + 子项数量；点击切换折叠
                            Rectangle {
                                id: header
                                height: 30
                                width: parent.width
                                color: "#303030"
                                border.color: "#262626"

                                // 修复：Row -> RowLayout，子项不再使用 anchors 来水平定位
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    spacing: 8

                                    // 类别名（display 角色）
                                    Text {
                                        text: display
                                        color: "#e0e0e0"
                                        font.bold: true
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                    // 子项数量（使用 childRepeater.count，确保准确）
                                    Text {
                                        text: "(" + childRepeater.count + ")"
                                        color: "#b0b0b0"
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                    // 伸缩占位：把右侧箭头推到最右边
                                    Item { Layout.fillWidth: true }

                                    // 折叠指示
                                    Text {
                                        text: section.expanded ? "▾" : "▸"
                                        color: "#a0a0a0"
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: section.expanded = !section.expanded
                                }
                            }

                            // 子项模型：遍历当前分组的子项（层级模型）
                            DelegateModel {
                                id: childDM
                                model: mediaLibraryModel
                                // 函数级注释：将根索引设置为当前分组，委托仅遍历该分组的子项
                                rootIndex: mediaLibraryModel.index(section.groupRow, 0)

                                // 子项委托
                                delegate: Item {
                                    width: groupColumn.width
                                    height: 28

                                    // 使用模型角色（来自 MediaLibraryModel::roleNames）
                                    property int ord: ordinal || 0
                                    property string name: display || ""
                                    property string pth: path || ""
                                    property int cat: category || 0

                                    // 选中状态：默认单选高亮（多选仅由 Ctrl 或 CheckBox 触发）
                                    property bool selected: root.selectedIndex === index
                                    property bool checked: root.selectedPaths.indexOf(pth) >= 0

                                    // 背景
                                    Rectangle {
                                        anchors.fill: parent
                                        color: selected ? "#3b3b3b" : "#2e2e2e"
                                    }

                                    // 内容：Row -> RowLayout
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        spacing: 8

                                        // 序号列
                                        Text {
                                            text: ord > 0 ? ord.toString() : ""
                                            color: "#a0a0a0"
                                            width: 24
                                            horizontalAlignment: Text.AlignRight
                                            verticalAlignment: Text.AlignVCenter
                                            Layout.alignment: Qt.AlignVCenter
                                        }

                                        // 复选框（仅用于多选）
                                        CheckBox {
                                            checked: checked
                                            Layout.alignment: Qt.AlignVCenter
                                            onToggled: {
                                                // 函数级注释：复选框切换多选集合，不改变单选高亮
                                                if (checked) root.addSelectedPath(pth)
                                                else root.removeSelectedPath(pth)
                                            }
                                        }

                                        // 文件名
                                        Text {
                                            text: name
                                            color: "#e0e0e0"
                                            elide: Text.ElideRight
                                            verticalAlignment: Text.AlignVCenter
                                            Layout.alignment: Qt.AlignVCenter
                                            Layout.fillWidth: true
                                        }
                                    }

                                    // 鼠标交互（左键默认单选；Ctrl+左键或勾选复选框才多选；右键弹菜单；双击打开）
                                    MouseArea {
                                        anchors.fill: parent
                                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                                        onClicked: function(mouse) {
                                            if (mouse.button === Qt.LeftButton) {
                                                if (mouse.modifiers & Qt.ControlModifier) {
                                                    // 多选：Ctrl+Left 切换多选集合
                                                    if (checked) root.removeSelectedPath(pth)
                                                    else root.addSelectedPath(pth)
                                                } else {
                                                    // 单选：Left 设置唯一选中并高亮
                                                    root.setSingleSelection(index, pth)
                                                }
                                            } else if (mouse.button === Qt.RightButton) {
                                                ctxMenu.popup(mouse.screenX, mouse.screenY)
                                            }
                                        }
                                        // 函数级注释：双击激活文件
                                        onDoubleClicked: {
                                            if (pth && pth.length > 0) {
                                                root.fileActivated(pth)
                                                libraryBridge.openFile(pth)
                                            }
                                        }
                                    }

                                    // 右键菜单（打开/定位/删除）
                                    Menu {
                                        id: ctxMenu
                                        MenuItem {
                                            text: qsTr("打开")
                                            onTriggered: libraryBridge.openFile(pth)
                                        }
                                        MenuItem {
                                            text: qsTr("在资源管理器中显示")
                                            onTriggered: libraryBridge.revealInExplorer(pth)
                                        }
                                        MenuItem {
                                            text: qsTr("删除")
                                            onTriggered: {
                                                libraryBridge.deleteItems([pth])
                                                root.removeSelectedPath(pth)
                                            }
                                        }
                                    }
                                }
                            }

                            // 子项容器：使用 Column + Repeater 渲染
                            Item {
                                id: content
                                width: parent.width
                                visible: section.expanded

                                Column {
                                    id: childrenColumn
                                    width: parent.width
                                    spacing: 0

                                    // 关键：用 childDM 生成的委托，确保 childRepeater.count 即为该组真实子项数量
                                    Repeater {
                                        id: childRepeater
                                        model: childDM
                                        delegate: model.delegate // 复用 DelegateModel 中的委托
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}