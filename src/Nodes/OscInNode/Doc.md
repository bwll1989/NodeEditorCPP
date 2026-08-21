# OSC Source

## 1. 节点说明

OSC 接收节点（界面标题为「OSC Source」），在指定 UDP 端口监听 Open Sound Control 消息，并将地址、参数值输出到节点图。适合接收 Resolume、QLab、TouchOSC、Max 等软件的 OSC 控制。

支持**多参数**消息：多个 OSC 参数会收成 `default` 列表（例如 `/vec ,fff` → `[x,y,z]`）；单参数仍为标量。

## 2. 端口说明

### 输入

本节点无输入端口。

### 输出

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | RESULT | 完整 OSC 消息（VariableData：`address` / `type` / `default`） |
| 1 | ADDRESS | OSC 地址路径（如 `/cue/1/go`） |
| 2 | VALUE | 参数值：单参为标量，多参为 float/int/string 列表 |

## 3. 界面说明

- **port**：本地监听端口，默认 6000。
- **address**：最近一次收到的 OSC 地址（只读显示）。
- **value**：最近一次收到的参数值（多参显示为 JSON 数组，如 `[0.1,0.2,0.3]`）。

## 4. 使用说明

1. 将 **port** 设为与发送方一致的端口（发送方目标 IP 为本机）。
2. 把 **ADDRESS** 或 **VALUE** 连到条件、映射或执行节点；向量可直接接 Scatter / OscOut 等。
3. 收到新消息时，三个输出端口与界面上的 address、value 同步更新。
4. 工程会保存监听端口。

**外部控制路径**（完整地址：`/dataflow/{父级别名}/{节点ID}{相对路径}`）：

| 相对路径 | 作用 |
|----------|------|
| `/port` | 设置监听端口（整数） |

支持 OSC / 全局事件总线；修改端口后会重新绑定接收器，并反馈当前端口状态。

## 5. 示例

- **QLab 控场景**：QLab 发 `/cue/1/start` 到本机 6000，**ADDRESS** 连 Switch 节点按路径分支。
- **多参数向量**：外部发 `/position 1.0 2.0 3.0` → **VALUE** 为 `[1,2,3]`，可接 Vec Source / Scatter / OscOut。
- **远程改端口**：演出前用 OSC 命令将 `/dataflow/Live/3/port` 设为 `7000`，避免与其他软件冲突。
