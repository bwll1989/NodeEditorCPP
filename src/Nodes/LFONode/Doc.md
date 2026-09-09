**LFONode 插件文档**

本插件（`Controls` 分类）包含两个节点：**LFO** 与 **Metronome**。

---

# LFO

## 1. 节点说明

LFO（低频振荡器）按设定波形、周期和振幅**持续输出**变化的数值，常用于自动化缓变、呼吸灯、周期性调制等。通过内部定时器按采样率生成波形。

支持六种波形（对齐 TouchDesigner LFO CHOP）：Sine、Gaussian、Triangle、Ramp、Square、Pulse。输出为 `offset + amplitude × unit`；**偏置 (bias)** 用于 Triangle（峰值位置）、Square（占空比）、Gaussian（宽度）。

实现文件：`LFODataModel.hpp`、`LFOInterface.hpp`（内嵌完整操作面板）。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | PLAY | VariableData | 布尔 `true` 时开始运行 |
| 1 | STOP | VariableData | 布尔 `true` 时停止运行 |
| 2 | RESET | VariableData | 布尔 `true` 时将时间归零并刷新输出 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | VALUE | VariableData | 当前波形采样值 |
| 1 | TIME | VariableData | 当前时间（秒） |
| 2 | VECTOR | VariableData | `[时间, 值]` 二维向量，便于二维视图 |

## 3. 界面说明

节点内嵌面板（`WidgetEmbeddable = true`），顶部为**波形预览**（单周期；左侧为起点；运行时有播放头）。可在预览上横向拖动调相位、纵向拖动调偏移。控件与参数对应：

| 控件 | 含义 | 默认 |
|------|------|------|
| 波形 | Sine / Gaussian / Triangle / Ramp / Square / Pulse | Sine |
| 周期 (s) | 单个周期时长 | 1 |
| 振幅 | 摆幅缩放 | 10 |
| 偏移 | 振幅中心偏置（DC） | 0 |
| 偏置 | Triangle / Square / Gaussian 形状控制，[-1,1] | 0 |
| 相位 | 周期分数 [0,1) | 0 |
| 采样率 | 每秒采样次数 | 10 |
| 限制周期 | 关闭时无限运行；开启后跑完指定周期数（支持小数，如 0.5=半周期）自动停止 | 关 / 1 周期 |
| Play | 可切换按钮，对应属性/地址 `play` | 关 |
| Reset | 时间归零 | — |

**外部控制地址**（相对节点地址，经全局事件总线）：

- `/method`、`/period`、`/amplitude`、`/offset`、`/bias`、`/phase`、`/sampleRate` — 参数
- `/loop`、`/loopCount` — 限制周期使能与周期数（可小数）
- `/play` — 启停（布尔，已绑定 Play 按钮）
- `/reset` — 复位触发（布尔 `true` 生效）

## 4. 使用说明

1. 在面板上设置波形、周期、振幅等，或通过外部控制写入。
2. 按下 **Play**（或向 **PLAY** / `/play` 发送 `true`）开始输出；再按一次停止。
3. **STOP** 端口或取消勾选 Play 可暂停（时间不归零）；**RESET** 将时间清零。
4. **VALUE** 接调制目标；**VECTOR** 可接二维视图观察波形。

采样率过低时波形粗糙；为 0 或极小时输出恒为 0。

## 5. 示例

**呼吸灯：** 正弦波、周期 5 s、振幅 1，VALUE 接调光节点。演出开始时勾选 Play 或向 PLAY 发送 `true`。  
**遥控改速：** 将 `/period` 绑定到控台旋钮，现场调整快慢而无需改图。

---

# Metronome

## 1. 节点说明

**Metronome（节拍器）** 按固定时间间隔周期性输出字符串信号，适用于定时触发、节拍脉冲、周期性命令发送等场景。

与 LFO 不同，节拍器每次 tick 输出**同一个可配置的字符串**，而不是连续变化的波形数值。

- 数据模型：`MetronomeDataModel.hpp` — 定时逻辑、端口、属性、save/load、外部控制
- 界面组件：`MetronomeInterface.hpp` — 内嵌于节点上的操作面板

启动后立即发出第一拍，之后按 `interval` 毫秒重复输出。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | INTERVAL | VariableData | 节拍间隔（整型，单位 ms，最小 1） |
| 1 | VALUE | VariableData | 每次节拍输出的字符串 |
| 2 | Enable | VariableData | 是否启用（布尔，`true` 开始，`false` 停止） |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | OUTPUT | VariableData | 当前节拍输出的字符串（`signalValue`） |

## 3. 界面说明（MetronomeInterface）

节点内嵌面板（`WidgetEmbeddable = true`），控件与属性一一对应：

| 控件 | 类型 | 说明 | 默认值 |
|------|------|------|--------|
| 间隔 (ms) | `IntDragValueWidget` | 两次节拍之间的毫秒数 | 1000 |
| 信号值 | `QLineEdit` | 每次 tick 输出的字符串 | `tick` |
| Start | 可切换按钮 | 启停节拍器（对应 `running`） | 未启动 |

间隔范围：1 ~ 360000 ms。

**属性（Q_PROPERTY）**

| 属性 | 类型 | 说明 |
|------|------|------|
| interval | int | 间隔（ms） |
| signalValue | QString | 输出字符串 |
| running | bool | 是否运行中 |

**外部控制地址**（相对节点地址，经全局事件总线）：

- `/interval` — 间隔（整型，ms）
- `/signalValue` — 输出字符串
- `/running` — 启停（布尔）

## 4. 使用说明

1. 在节点库 **Controls** 分类中选择 **Metronome**。
2. 设置间隔与信号值，点击 **Start** 开始；或向 **Enable** 输入端口发送 `true`。
3. 将 **OUTPUT** 接到需要接收节拍字符串的下游节点（如脚本、OSC、条件判断等）。
4. 也可通过 **INTERVAL** / **VALUE** 输入端口动态覆盖面板参数。
5. 运行中修改间隔会重启定时器；停止后再启动会立即发出第一拍。

**持久化：** 保存流程图时会写入 `interval`、`signalValue`、`running`；加载后若 `running` 为 true 会自动恢复运行。

## 5. 示例

**定时触发脚本：** 间隔 500 ms，信号值 `"beat"`，OUTPUT 接脚本节点，每次收到 `"beat"` 执行一次动作。  
**外部控台启停：** 将 `/running` 绑定到控台按钮，现场开始/停止节拍而不改图。  
**动态 BPM：** INTERVAL 端口接计算节点，根据 BPM 换算毫秒后驱动节拍间隔。
