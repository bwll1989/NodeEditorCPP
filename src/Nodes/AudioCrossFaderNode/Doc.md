# Audio Cross Fader

## 1. 节点说明

在多路音频 A、B 之间做交叉淡入淡出（Crossfade）。用 **Channels** 配置成对路数，各路 `Ai`↔`Bi` 共用同一个 Mix。默认 **Mix=0**（直通 A，不混音）。混音与淡入淡出在独立工作线程执行，不影响节点图响应。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 否 | 4 | 130 | —（由 Channels 属性决定，非手动加减） | 末两端口为 SWTCH B（淡入 B，VariableData）、MIX（直接设混音比，VariableData） |
| 输出 | 1 | 否 | 1 | 64 | —（随 Channels 变化） | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

端口数由 **Channels** 属性决定（1～64，默认 1）：音频输入 `2N` 路，另加两路控制，输出 `N` 路。
Channels=2 时：输入 A1、A2、B1、B2、SWTCH B、MIX，输出 Out 1、Out 2。

### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| A1 … AN | AudioData | 否 | 音源 A 各通道；断开时：该路按静音参与混音 |
| B1 … BN | AudioData | 否 | 音源 B 各通道；断开时：该路按静音参与混音 |
| SWTCH B | VariableData | 否 | `true`/非 0/`"true"`/`"1"` → 淡入到 B；`false`/0 → 淡入到 A；仅在状态变化时触发淡入淡出，重复同值不重启；断开时：保持当前 A/B 选择 |
| MIX | VariableData | 否 | 直接设置混音比 0～1（0=全 A，1=全 B），可用 TimeLine/旋钮驱动；非法文本忽略；断开时：保持当前 mix 值 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| Out / Out 1 … Out N | AudioData | 各对 Ai/Bi 的混合结果（共享时间戳环形缓冲，连续输出） |

## 3. 界面与属性

内嵌界面控件显示在右侧属性面板：Channels 拖动框、Mix 拖动条、Fade Time 拖动框、Action 互斥按钮（**B → A** / **A → B**）。

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| channels | int | 1 | 1～64 | 成对通道数，改变即重建端口与缓冲 |
| mix | double | 0.0 | 0～1 | 0=全 A，1=全 B，中间为等功率过渡 |
| fadeMs | double | 2000 | 10～600000 ms | 按钮 / SWTCH B / `/action` 触发淡入淡出的时长 |
| controlAction | int | 0 | 0=B→A（输出 A）、1=A→B（输出 B） | 目标侧动作，触发时按 fadeMs 淡入淡出并把 mix 对齐到 0/1 |

保存到工程：`values.channels`、`values.mix`、`values.fade_ms`、`values.action`，加载时恢复（action 优先，mix 对齐 action）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`（{别名} 为节点别名，{节点ID} 为节点编号）

| 地址 | 类型 | 范围 | 说明 |
|------|------|------|------|
| /channels | int | 1～64 | 设置成对通道数（重建端口） |
| /mix | float | 0～1 | 直接设置混音比 |
| /fade_ms | float | 10～600000 ms | 设置淡入淡出时长 |
| /action | int | 0/1 | 0=B→A、1=A→B，触发淡入淡出 |
| /switch_b | bool/数值 | 真值→B | 等价于 SWTCH B 控制口 |

以上地址均通过 `registerExternalBinding` 注册（与内嵌控件双向绑定），并订阅全局事件总线 Command 事件。

## 5. 行为细节

- **状态与持久化**：`channels` / `mix` / `fade_ms` / `action` 保存到工程并恢复；加载后 SWTCH B / MIX 控制口的首次输入只作基线，不会覆盖已恢复的状态。
- **时序/性能**：混音与淡入淡出在独立 QThread 工作线程执行（参数经队列调用推送）；音频走共享时间戳环形缓冲直通工作线程。
- **错误行为**：输入断开时该路按静音处理；MIX 端口收到无法解析的值时忽略；`channels` 超范围自动钳位到 1～64。
- **防抖**：SWTCH B 同值重复输入不重复触发淡入淡出；`setControlAction` 与当前动作相同也只同步按钮。

## 6. 使用说明与典型接法

1. 用 Channels 设好路数。
2. 各路 A/B 分别接入对应端口。
3. 用 Action 按钮、SWTCH B 做淡入切换；或把 TimeLine / 数值接到 **MIX** 口连续控制混音比。
4. Out 接到声卡或下级处理。

## 7. 示例

立体声曲目 A/B → Channels=2 → CrossFader；TimeLine 驱动 MIX → 自动淡入下一曲。
