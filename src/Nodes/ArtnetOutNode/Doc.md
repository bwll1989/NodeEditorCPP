# Artnet Out

## 1. 节点说明

将上游 **DMX Universe** 节点输出的 Art-Net 数据包，通过 UDP 发送到指定目标主机。支持多个 Universe 输入（端口可增删），每次收到数据会自动发送；并输出最近一次发送的成功或失败状态。

只负责接收 Art-Net 数据时用 **Artnet Source**。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 是 | 1 | 不限 | VariableData | 无 |
| 输出 | 1 | 否 | 1 | 1 | —（不可新增） | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| UNIVERSE DATA ×N | VariableData | 否 | 每口对应一个 Universe：接收 DMX Universe 节点输出的 Art-Net 数据包（`protocol == "Art-Net"` 的 map，含 `fullUniverse`、`dmxData`），收到即发送；断开时：仅移除该口缓存，不再发送该口数据 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| STATUS | VariableData | 最近一次发送结果 map：成功 `{status:"success", universe, host, bytesWritten, timestamp}`；失败 `{status:"failed", error, universe, host, timestamp}`；事件驱动（每帧发送后更新） |

## 3. 界面与属性

本节点无自定义界面控件，属性经属性树（PropertyTreeWidget）显示与编辑。

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| targetHost | string | 192.168.0.255 | IPv4 地址 | 目标主机地址（默认为广播地址），OSC `/targetHost` |
| isReady | bool | false | 只读 | 发送器就绪状态：发送成功置 true，失败/未初始化置 false，OSC `/isReady` |

`targetHost` 保存到工程（`values.TargetHost`）并在加载时恢复。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`（{别名} 为节点别名，{节点ID} 为节点编号）

| 地址 | 类型 | 范围 | 说明 |
|------|------|------|------|
| /targetHost | string | IPv4 地址 | 设置目标主机，外部命令经全局事件总线（Command）更新属性 |
| /isReady | bool | 只读 | 就绪状态反馈（stateFeedBack 输出，不可写入） |

## 5. 行为细节

- **状态与持久化**：仅 `targetHost` 保存到工程；Universe 缓存为运行时状态，重开工程后等输入重新到达。
- **发送机制**：收到有效 Art-Net 包即构建 ArtnetFrame（universe 取包内 `fullUniverse`，DMX 数据补齐/截断为 512 字节、每字节钳位 0～255）入发送队列。
- **多 Universe**：每个输入口独立缓存对应 Universe，互不影响；端口可增删以适配任意数量 Universe。
- **错误行为**：非 Art-Net 协议包、空包、类型不匹配均直接忽略；发送失败时 STATUS 输出失败详情且 `isReady` 置 false。

## 6. 使用说明与典型接法

1. 典型链路：**DMX Universe → Artnet Out → （STATUS → Log Out / Data Info 观察发送状态）**。
2. 多路输出：增加输入端口（+ 号），每口接一个 DMX Universe，一次发送多个 Universe。
3. 目标地址默认 `192.168.0.255`（局域网广播）；需要定点发送时改 `targetHost`（界面属性树或 OSC `/targetHost`）。

## 7. 示例

播放器驱动灯光：

DMX Universe（Universe 0）→ Artnet Out（targetHost = 192.168.0.255）→ STATUS → Data Info，在控制台观察每帧发送状态。
