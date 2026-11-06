# OSC Source

## 1. 节点说明

OSC 接收节点（界面标题为「OSC Source」），在指定 UDP 端口监听 Open Sound Control 消息，并将地址、参数值输出到节点图。适合接收 Resolume、QLab、TouchOSC、Max 等软件的 OSC 控制。

支持**多参数**消息：多个 OSC 参数会收成 `default` 列表（例如 `/vec ,fff` → `[x,y,z]`）；单参数仍为标量。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 0 | 否 | 0 | 0 | — | 无 |
| 输出 | 3 | 否 | 3 | 3 | — | 末端口为 VALUE（消息文本） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

本节点无输入端口。

### 输出

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | RESULT | 完整 OSC 消息（VariableData：`address` / `type` / `default`） |
| 1 | ADDRESS | OSC 地址路径（如 `/cue/1/go`） |
| 2 | VALUE | 参数值：单参为标量，多参为 float/int/string 列表 |

## 3. 界面与属性

控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 含义 |
|------|------|
| port | 本地监听端口（IntDragValueWidget），默认 6000 |
| address | 最近一次收到的 OSC 地址（只读显示） |
| value | 最近一次收到的参数值（多参显示为 JSON 数组，如 `[0.1,0.2,0.3]`） |

可外部控制的属性（Q_PROPERTY）：`port`。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/port` | 读写 | 设置本地监听端口（整数），修改后重新绑定接收器并回读当前值 |

## 5. 行为细节

- **消息接收**：`OSCReceiver` 在独立线程监听 UDP；收到消息后以键值对象（`address` / `type` / `default`）更新输出并刷新界面显示。
- **多参合并**：多个 OSC 参数收进 `default` 列表（如 `/vec ,fff` → `[x,y,z]`）；单参仍为标量。
- **输出语义**：RESULT 发布完整消息对象；ADDRESS 发布 `address` 字段字符串；VALUE 发布 `default`（标量或列表，原样透传）。
- **端口切换**：`setPort` 通过队列调用在接收线程重绑定端口，任意来源（界面 / OSC / `.flow`）修改都会同步控件并回读 `/port`。
- **连接策略**：三个输出端口均为 Many。
- **持久化**：监听端口保存在 `values.Port`（注意键名首字母大写）；`load()` 恢复后自动按新端口重绑。
- **资源清理**：析构时销毁界面控件（接收器随模型生命周期释放）。

## 6. 使用说明与典型接法

1. 将 **port** 设为与发送方一致的端口（发送方目标 IP 为本机）。
2. 把 **ADDRESS** 或 **VALUE** 连到条件、映射或执行节点；向量可直接接 Scatter / OscOut 等。
3. 收到新消息时，三个输出端口与界面上的 address、value 同步更新。
4. 工程会保存监听端口。

## 7. 示例

- **QLab 控场景**：QLab 发 `/cue/1/start` 到本机 6000，**ADDRESS** 连 Switch 节点按路径分支。
- **多参数向量**：外部发 `/position 1.0 2.0 3.0` → **VALUE** 为 `[1,2,3]`，可接 Vec Source / Scatter / OscOut。
- **远程改端口**：演出前用 OSC 命令将 `/dataflow/Live/3/port` 设为 `7000`，避免与其他软件冲突。

