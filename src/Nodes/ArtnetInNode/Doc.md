# Artnet Source

## 1. 节点说明

从局域网接收 Art-Net / DMX 数据，并输出为可在流程中继续使用的变量数据。可指定 Universe、按通道过滤，也支持通过外部命令或输入端口动态修改这些参数。

需要把 DMX 数据发送到灯具/设备时用 **Artnet Out**；本节点只负责接收。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | —（不可新增） | 无 |
| 输出 | 1 | 否 | 1 | 1 | —（不可新增） | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| UNIVERSE | VariableData | 否 | Universe 编号（整数），写入即更新 `universe` 属性；断开时：保持当前值，仅停止动态修改 |
| CHANNELS | VariableData | 否 | 通道过滤文本，如 `1,2,5-10`（通道号 0～512，支持逗号分隔与范围）；断开时：保持当前值 |
| FILTER | VariableData | 否 | 是否启用通道过滤（布尔）；断开时：保持当前值 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| OUTPUT | VariableData | 未启用过滤时：Artnet 原始 map（含 `universe`、`host`、`default` 等），每收到一帧更新一次（事件驱动）；启用过滤且命中时：map 含 `universe`、`host` 及所配通道键值（如 `"1": 255`） |

## 3. 界面与属性

本节点无自定义界面控件，属性经属性树（PropertyTreeWidget）显示与编辑。

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| universe | int | 0 | 无限制 | 目标 Universe 编号，OSC `/universe` |
| channels | string | 空 | 通道 0～512 | 通道过滤文本（`1,2,5-10` 格式），OSC `/channels` |
| filterEnabled | bool | false | true/false | 是否启用通道过滤，OSC `/filter` |

三个属性均保存到工程（`values.Universe` / `values.Channels` / `values.Filter`）并在加载时恢复。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`（{别名} 为节点别名，{节点ID} 为节点编号）

| 地址 | 类型 | 范围 | 说明 |
|------|------|------|------|
| /universe | int | 无限制 | 设置目标 Universe，等价于 UNIVERSE 端口 |
| /channels | string | 通道 0～512 | 设置通道过滤文本，等价于 CHANNELS 端口 |
| /filter | bool | true/false | 启用/关闭通道过滤，等价于 FILTER 端口 |

以上地址通过 `registerExternalBinding` 注册并订阅全局事件总线（Command 事件），外部命令会直接更新对应属性。

## 5. 行为细节

- **状态与持久化**：`universe` / `channels` / `filterEnabled` 保存到工程，重新打开工程后恢复。
- **过滤逻辑**：`filterEnabled = true` 时只输出 universe 匹配的数据；若 `channels` 非空，输出 map 仅含所配通道键值；`channels` 为空时不输出（等待配置）。
- **时序**：Artnet 接收为事件驱动（每收到一帧更新一次输出），无轮询。
- **错误行为**：输入端口数据断开（`nullptr`）时忽略、不影响已填参数；通道文本解析失败的部分被跳过；非法通道号（<0 或 >512）被丢弃。

## 6. 使用说明与典型接法

1. 将节点放入流程，默认监听全部 Art-Net 数据（`filterEnabled = false`）。
2. 若只关心某个 Universe：向 UNIVERSE 写入编号，或将 FILTER 设为 true 并填写 CHANNELS。
3. 通道号为 0～512，与 DMX 字节下标一致；支持逗号与范围，如 `1,2,5-10`。

## 7. 示例

只取 Universe 0 的通道 1、2、10：

- UNIVERSE = `0`
- CHANNELS = `1,2,10`
- FILTER = `true`

将 OUTPUT 接到需要 DMX 数值的下游节点即可。
