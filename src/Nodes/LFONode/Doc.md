**LFONode 插件文档**

本插件（`Controls` 分类）包含两个节点：**LFO** 与 **Metronome**。

---

# LFO

## 1. 节点说明

LFO（低频振荡器）按设定波形、周期和振幅**持续输出**变化的数值，常用于自动化缓变、呼吸灯、周期性调制等。通过内部定时器按采样率生成波形。

支持六种波形（对齐 TouchDesigner LFO CHOP）：Sine、Gaussian、Triangle、Ramp、Square、Pulse。输出为 `offset + amplitude × unit`；**偏置 (bias)** 用于 Triangle（峰值位置）、Square（占空比）、Gaussian（宽度）。

实现文件：`LFODataModel.hpp`、`LFOInterface.hpp`（内嵌完整操作面板）。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 末端口为 RESET（时间归零） |
| 输出 | 3 | 否 | 3 | 3 | — | 末端口为 VECTOR（[时间,值] 向量） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



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

## 3. 界面与属性

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

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/method` | 读/写 | 波形类型（枚举索引） |
| `/period` | 读/写 | 周期（秒） |
| `/amplitude` | 读/写 | 振幅 |
| `/offset` | 读/写 | 偏移（DC） |
| `/bias` | 读/写 | 偏置（[-1,1]） |
| `/phase` | 读/写 | 相位（[0,1)） |
| `/sampleRate` | 读/写 | 采样率 |
| `/loop` | 读/写 | 限制周期使能 |
| `/loopCount` | 读/写 | 周期数（支持小数） |
| `/play` | 读/写 | 启停（绑定 Play 按钮） |
| `/reset` | 写 | 布尔 true 时时间归零（无属性绑定，仅命令） |

## 5. 行为细节

- **波形生成**：QTimer 按采样率触发 `generateWave`——采样当前波形值 → 更新三个输出 → 时间累加 1/sampleRate。采样率为 0 或极小时输出恒 0。
- **输出语义**：VALUE = `offset + amplitude × unit`；TIME = 当前时间（秒）；VECTOR = `[时间, 值]` 列表（每次输出新建 VariableData）。
- **限制周期**：`loop` 开启且时间到达 `period × loopCount` 时——输出定格终值后自动 `setPlay(false)` 停止；重新播放若已超限会先停。
- **输入语义**：PLAY/STOP/RESET 端口均 `asBool()==true` 生效（false 忽略）；STOP 只暂停不归零，RESET 归零并立即按 t=0 采样输出。
- **预览**：面板波形预览随参数刷新，运行时显示播放头；预览上横向拖动调相位、纵向拖动调偏移。
- **持久化**：`values` 内 `method/period/amplitude/offset/bias/phase/sampleRate/loop/loopCount/play` 全量保存恢复。

## 6. 使用说明与典型接法

1. 在面板上设置波形、周期、振幅等，或通过外部控制写入。
2. 按下 **Play**（或向 **PLAY** / `/play` 发送 `true`）开始输出；再按一次停止。
3. **STOP** 端口或取消勾选 Play 可暂停（时间不归零）；**RESET** 将时间清零。
4. **VALUE** 接调制目标；**VECTOR** 可接二维视图观察波形。

采样率过低时波形粗糙；为 0 或极小时输出恒为 0。

## 7. 示例

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

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 末端口为 Enable（启停） |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



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

## 3. 界面与属性

### MetronomeInterface

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

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/interval` | 读/写 | 间隔（整型，ms） |
| `/signalValue` | 读/写 | 输出字符串 |
| `/running` | 读/写 | 启停（布尔，绑定 Start 按钮） |

## 5. 行为细节

- **节拍时机**：启动后**立即发出第一拍**，之后按 interval 毫秒重复；运行中修改间隔会重启定时器；停止后再启动重新立即出第一拍。
- **输出语义**：每次 tick 输出 `signalValue` 字符串（新建 VariableData），不做递增或脉冲附带字段。
- **输入语义**：INTERVAL 取 `asInt()`（最小 1 ms）、VALUE 取 `asString()`、Enable 取 `asBool()`（true 启动 / false 停止），均可动态覆盖面板值。
- **间隔范围**：1 ～ 360000 ms。
- **持久化**：`values.interval` / `values.signalValue` / `values.running`；加载后若 `running` 为 true 自动恢复运行。

## 6. 使用说明与典型接法

1. 在节点库 **Controls** 分类中选择 **Metronome**。
2. 设置间隔与信号值，点击 **Start** 开始；或向 **Enable** 输入端口发送 `true`。
3. 将 **OUTPUT** 接到需要接收节拍字符串的下游节点（如脚本、OSC、条件判断等）。
4. 也可通过 **INTERVAL** / **VALUE** 输入端口动态覆盖面板参数。
5. 运行中修改间隔会重启定时器；停止后再启动会立即发出第一拍。

**持久化：** 保存流程图时会写入 `interval`、`signalValue`、`running`；加载后若 `running` 为 true 会自动恢复运行。

## 7. 示例

**定时触发脚本：** 间隔 500 ms，信号值 `"beat"`，OUTPUT 接脚本节点，每次收到 `"beat"` 执行一次动作。  
**外部控台启停：** 将 `/running` 绑定到控台按钮，现场开始/停止节拍而不改图。  
**动态 BPM：** INTERVAL 端口接计算节点，根据 BPM 换算毫秒后驱动节拍间隔。

