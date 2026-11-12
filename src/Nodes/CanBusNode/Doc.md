# CAN Bus

## 1. 节点说明

> ⚠️ **版本要求**：本节点为 **Advance（进阶版）** 功能，需有效授权后才能加载使用。

Windows 下通过 WinUSB + candle API 访问 candleLight / CANable 等 gs_usb 设备，收发原始 CAN 帧（2.0A/B）。典型用途：控制/读取 CAN 总线设备（底盘、灯控、PLC 网关等）。只控制单个 CAN 设备用专用节点（如 Ranger Mini）更方便；需要收发任意原始帧时用本节点。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 末端口为 TRIGGER（发送触发） |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| ID | VariableData | 否 | CAN ID（HEX 字符串，如 `1A`）或含 `id`/`data`/`extended` 的 map；收到即按口内容发送；断开时：不影响已填参数，仅停止自动发送 |
| DATA | VariableData | 否 | 数据 HEX 字符串（如 `01 FF 00`）；收到即发送；断开时：同上 |
| TRIGGER | VariableData | 否 | 触发一次发送（真值时发当前 ID+DATA）；断开时：仅界面 Send 按钮可发 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| FRAME | VariableData | 收到的帧 map：`id`/`id_hex`/`extended`/`rtr`/`dlc`/`data`/`timestamp_us`，事件驱动（每收到一帧更新一次） |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| deviceIndex | int | -1 | 已枚举设备索引 | gs_usb 设备索引，对应界面设备下拉框，OSC `/deviceIndex` |
| bitrate | int | 取决于下拉框 | 20k–1M（常用 500000） | 波特率，连接前设置，OSC `/bitrate` |
| canId | string | 空 | HEX 字符串 | 待发送 CAN ID，OSC `/canId` |
| data | string | 空 | HEX 字符串 | 待发送数据，OSC `/data` |
| connected | bool | false | true/false | 打开/关闭设备，OSC `/connected` |

内嵌界面控件：设备下拉框 + 刷新设备按钮、波特率下拉框、连接开关、扩展帧复选框、CAN ID / Data 输入框、Send 按钮。`canId`/`data` 保存到工程并在加载时恢复。

## 4. 外部控制（OSC 地址）

### OSC 地址

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 类型 | 范围 | 说明 |
|------|------|------|------|
| /deviceIndex | int | 枚举索引 | 选择设备；变更即重新开关设备。 |
| /bitrate | int | 20k–1M | 设置波特率；变更即重新开关设备。 |
| /canId | string | HEX | 设置待发送 CAN ID（支持 `0x` 前缀）。 |
| /data | string | HEX | 设置待发送数据。 |
| /connected | 只读 | true/false | 实际连接状态反馈（由设备就绪回调驱动）；**外部写入无效**（写方法为空实现）。 |
| /send | 触发 | — | 触发一次发送；发送时推送 `true`，结束（无论成败）推 `false`。 |

前 5 个地址注册了带属性 member 的绑定：模型就绪时推送一次初始值，此后属性变化自动反馈（`/send` 的 member 不是真实属性，仅由节点手动推送）。6 个地址全部可被外部 Command 触发，仅 `/connected` 例外。

## 5. 行为细节

- 设备开关时机：deviceIndex、bitrate、通道号、工作模式任一变化都会重新开关设备；deviceIndex 为 -1 时关闭。连接状态完全由设备就绪回调（`isReady`）驱动，面板按钮只是显示。
- 工作模式三档：Normal / Listen Only（只听不发）/ Loop Back（回环）。
- 发送规则：CAN ID 去 `0x` 前缀后按 16 进制解析，失败直接丢弃不发送；DATA 去空格后 `fromHex`，超过 8 字节截断到 8；扩展帧 / RTR 标志取自界面复选框。
- 端口注入即发：ID 口收到 map 可一次带 `id` / `data` / `extended`（字符串则只填 ID），填完立即发送；DATA 口收到即填即发；TRIGGER 口真值触发发送当前参数。输入断开不动作。
- 收帧输出事件驱动：每收到一帧 `FRAME` 口更新一次（map 含 `id`/`id_hex`/`extended`/`rtr`/`dlc`/`data`/`timestamp_us`）。
- 端口连接策略：输入、输出均为 Many；通道号与工作模式两个控件没有 OSC 地址。
- 持久化：`values` 子对象 8 键（DeviceIndex / Bitrate / Channel / Mode / CanId / Data / Extended / Rtr）；加载时先刷新设备列表再恢复并重新打开设备。

## 6. 使用说明与典型接法

1. 插入 candleLight 适配器（设备管理器显示 WinUSB）
2. 添加 **Connect → CAN Bus** 节点
3. Refresh Devices → 选择设备 → 设置 Bitrate → 确认 Connected
4. 填写 CAN ID / Data 后 Send，或从输入口注入（ID 口接 map 可一次带 id/data/extended）

