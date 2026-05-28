# Internal Commands 节点

## 1. 节点说明

Internal Commands 插件用于**批量发送 OSC 类命令**，包含两种节点：

| 变体 | 节点库名称 | 行为 |
|------|------------|------|
| Internal Commands | Internal Commands | 将列表中的消息交给**内部状态容器**解析（`StatusContainer`），用于应用内命令 |
| Osc Out Group | Osc Out Group | 通过 **OSCSender** 向外部发送 OSC 消息（标签在库中为 Connect） |

二者界面相同：OSC 消息列表 + 发送测试按钮；触发后按列表顺序执行全部消息。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | TRIGGER | VariableData | 值为 true 时触发发送 |

### 输出

无（Internal Commands 的 outData 返回空 VariableData；Osc Out Group 同样无业务输出）。

## 3. 界面说明

- **消息列表**（OSCMessageListWidget）：编辑多条 OSC 消息。Internal Commands 变体通常仅允许**内部**地址；Osc Out Group 可配置对外发送。
- **发送按钮**（图标）：手动触发一次，等效于 TRIGGER 为 true。

**外部控制地址：**

- `/trigger` — 布尔 true 时执行列表（与点击发送按钮相同）；触发后会反馈 `/trigger` 状态。

列表内容保存在 `values` 中。

## 4. 使用说明

1. 在列表中添加目标 OSC 消息（地址与参数）。
2. 将 TRIGGER 接布尔脉冲，或点击发送按钮测试。
3. 需要由其它图节点远程触发时，向 `/trigger` 发送 true。

Internal Commands 适合控制本应用内路由；Osc Out Group 适合发到外部设备或其它软件。

## 5. 示例

**场景宏：** 列表内写多条 `/cue/1`、`/light/master` 等，Show 开始时 TRIGGER 一次全部下发。  
**与 Switch 联动：** Switch 输出接不同 Internal Commands 节点的 TRIGGER，实现多套路由。
