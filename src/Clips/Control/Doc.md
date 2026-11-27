# Control 片段

Control 指令片段（时间轴片段类型，type = `"Control"`）——合并前 Trigger 片段的语义后继（2026-11-13 用户定案新增）。

## 功能

- **定位**：只在时间轴中存在——无媒体文件、无画布呈现（`asset_id` 恒空）；播放头进入片段区间（**上升沿**，检测挂宿主适配器播放头推进链）时，结合 StatusContainer 把片段携带的指令逐条发送（经 parseOSC 入状态广播链，WS 8992 出站即达外部/控件联动）。
- **模型** `ControlClipModel`：继承 BaseClass 的 `SceneClipModel`——橙色块（#F08A5D，原 Trigger 类型色）、`RESIZEABLE=false`（定长：触发窗口语义）、`EMBEDWIDGET=false`、`SHOWBORDER=false`。`paint` 逐行复刻旧 `TriggerClipModel::paint`——窄块呈「一条橙线 + 带色底文字」贴在触发帧旁（左侧 4px 金色竖线 = `ClipBorderColour`）。
- **消息列表**：存 `ms::Clip.control_messages`（随场景 pb 分发，field 10，OSCMessageListWidget save/load 兼容格式）——不再是节点委托模型的 save()/load() 载荷。
- **专属编辑区** `ControlClipSection`：OSCMessageListWidget + 测试发送按钮，属性面板第 3 页（`createPropertySection` 工厂加载，只校验片段上下文存在）。

## 结构（2026-11-15 对齐节点插件同构）

| 文件 | 职责 |
|---|---|
| `ControlClipModel.hpp` | 类型专属模型 + paint 复刻（类名=文件名，对齐节点 `<Name>DataModel.hpp`） |
| `ControlClipSection.hpp/.cpp` | 指令编辑区（属性面板第 3 页） |
| `PluginDefinition.hpp/.cpp` | 插件定义（class `Plugin` : `ClipPlugInterface`，对齐节点命名） |
| `CMakeLists.txt` | 独立 target（`plugin_control` → `bin/plugins/TimeLine/control.plugin`，另链 MediaCore/GUIElements/OscListWidget/StatusContainer/Qml） |

## 加载（磁盘驱动，零内置兜底）

`WorkScenePluginLoader` 扫描 `bin/plugins/TimeLine/*.plugin`——删 `control.plugin` = Control 类型不存在。构建/输出约定详见 `../Video/Doc.md`。

---

## 属性面板
三页式面板装配与 schema 化设计见 [`../../Common/ClipPanelBase/Doc.md`](../../Common/ClipPanelBase/Doc.md)。场景全量下发、display 端无渲染 creator（执行在宿主适配器 updateControlTriggers 播放头两落点）；动画页按 schema 可见性隐藏。
