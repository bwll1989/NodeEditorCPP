
# Int Source

## 1. 节点说明

整数常量源。内嵌拖拽数值框，输出当前整数值；输入口可覆盖。值写入工程。

## 2. 端口说明

| 方向 | 端口 | 类型 | 说明 |
|------|------|------|------|
| 入 | （默认） | VariableData | 覆盖当前整数；断开时置 0 |
| 出 | （默认） | VariableData | 当前整数 |

## 3. 界面说明

`IntDragValueWidget`。外部控制：`/int`。

## 4. 使用说明

拖拽或 OSC 改值；需要记忆时用 Source，仅运行时用 Int Variable。

## 5. 示例

Int Source（场景号）→ Switch 的 INDEX。

---

# Int Variable

## 1. 节点说明

与 Int Source 相同的端口与界面，**工程不持久化数值**（运行时变量）。

## 2. 端口说明

同 Int Source。

## 3. 界面说明

同 Int Source。外部控制：`/int`。

## 4. 使用说明

作中间计数、临时索引；切场不恢复时用本节点。

## 5. 示例

Count → Int Variable → 下游显示。

---

# Float Source

## 1. 节点说明

浮点常量源。内嵌浮点拖拽控件，值可持久化。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 入 | VariableData | 覆盖当前浮点 |
| 出 | VariableData | 当前浮点 |

## 3. 界面说明

浮点拖拽控件。外部控制：`/float`。

## 4. 使用说明

增益、混合比、阈值等可复现参数。

## 5. 示例

Float Source → Audio Matrix 的增益口。

---

# Float Variable

## 1. 节点说明

浮点运行时变量；界面与端口同 Float Source，**不持久化**。

## 2. 端口说明

同 Float Source。

## 3. 界面说明

外部控制：`/float`。

## 4. 使用说明

LFO / 分析节点的中间结果暂存。

## 5. 示例

Audio Analysis → Float Variable → Condition。

---

# String Source

## 1. 节点说明

字符串常量源。内嵌单行编辑框，值可持久化。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 入 | VariableData | 覆盖字符串 |
| 出 | VariableData | 当前字符串 |

## 3. 界面说明

`QLineEdit`。外部控制：`/string`。

## 4. 使用说明

文件名、OSC 地址片段、设备 ID 等。

## 5. 示例

String Source → HTTP Client 的 URL 相关口。

---

# String Variable

## 1. 节点说明

字符串运行时变量；不持久化。外部控制：`/string`。

## 2. 端口说明

同 String Source。

## 3. 界面说明

同 String Source。

## 4. 使用说明

动态文案、临时路径。

## 5. 示例

Extract 字符串字段 → String Variable → To JSON。

---

# Bool Source

## 1. 节点说明

布尔常量源。内嵌勾选按钮，值可持久化。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 入 | VariableData | 覆盖布尔 |
| 出 | VariableData | 当前布尔 |

## 3. 界面说明

可勾选按钮。外部控制：`/bool`。

## 4. 使用说明

开关类默认状态、使能标志。

## 5. 示例

Bool Source → Object Detection 的 ENABLE。

---

# Bool Variable

## 1. 节点说明

布尔运行时变量；不持久化。外部控制：`/bool`。

## 2. 端口说明

同 Bool Source。

## 3. 界面说明

同 Bool Source。

## 4. 使用说明

边沿检测、临时互锁位。

## 5. 示例

Edge Trigger → Bool Variable → Hold。

---

# Toggle Source

## 1. 节点说明

可锁定的布尔开关（Checkable 按钮）。点击切换真假，值可持久化。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 入 | VariableData | 覆盖开关状态 |
| 出 | VariableData | 当前布尔 |

## 3. 界面说明

「Toggle」按钮（可勾选）。外部控制：`/bool`。

## 4. 使用说明

需要手动拨动且记住状态的开关。

## 5. 示例

Toggle Source → Audio Device Out 的静音/使能逻辑。

---

# Toggle Variable

## 1. 节点说明

与 Toggle Source 相同交互，**不持久化**。外部控制：`/bool`。

## 2. 端口说明

同 Toggle Source。

## 3. 界面说明

同 Toggle Source。

## 4. 使用说明

演出中临时拨动、不进工程快照的开关。

## 5. 示例

Keyboard In → Toggle Variable → Switch。

---

# Trigger Source

## 1. 节点说明

脉冲触发器：点击按钮或输入口收到 `true` 时，输出一次触发脉冲（读出后内部复位）。适合驱动 Inject、Internal Commands、Delay 等「一次性动作」节点。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 入 | VariableData | 为 `true` 时触发 |
| 出 | VariableData | 触发瞬间为真脉冲 |

## 3. 界面说明

`TriggerWidget`（Trigger 按钮）。外部控制：`/trigger`（`true` 时触发）。

## 4. 使用说明

1. 点击按钮或向输入送 `true`。
2. 将输出接到需要脉冲的下游。

## 5. 示例

Trigger Source → Inject 的 TRIGGER；或 → Osc Out Group 的 TRIGGER。

---

# Vec Source

## 1. 节点说明

可变长度向量源。分量是 **float 列表**，输出 **VariableData**（`default` 为列表）。维数由 Size 决定；输入端口可编辑（默认 5 路，末口为整段向量）。

## 2. 端口说明

### 输入

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 … n-2 | `0`… | 覆盖对应下标分量（超出当前 Size 则忽略） |
| n-1 | Vec | 整段向量；按当前 Size 截断或补 0 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| Vec | VariableData | `default` 为当前 float 列表 |

## 3. 界面说明

- **N**：分量个数（1–16）
- 下方按 0、1、2… 编辑各分量

## 4. 使用说明

改 Size 只影响向量长度，不会增删端口；改端口数量也不会改 Size。解析统一走 `floatVectorFromVariant`（列表 / 标量 / 旧几何类型均可）。

---

# To JSON

## 1. 节点说明

将字符串解析为 VariableData（JSON 对象）。内嵌可缩放文本框：若文本为合法 JSON **对象**，输出其键值表；否则将原文放入 `default` 字段。

## 2. 端口说明

| 方向 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 入 | STRING | VariableData | 字符串写入编辑框 |
| 出 | JSON | VariableData | 解析后的对象或 `{default: 原文}` |

## 3. 界面说明

`QPlainTextEdit`（`Resizable=true`）。编辑即刷新输出。文本写入工程。

## 4. 使用说明

1. 粘贴或输入 JSON；或由上游字符串写入。
2. 下游用 Extract / Condition 读字段。

## 5. 示例

HTTP Client 响应字符串 → To JSON → Extract `$input.status`。

---

# From JSON

## 1. 节点说明

将 VariableData 转为 JSON 文本字符串输出。输入调用 `toJsonString()` 填入编辑框；输出为编辑框纯文本。

## 2. 端口说明

| 方向 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 入 | JSON | VariableData | 对象/表 → 文本 |
| 出 | STRING | VariableData | 当前文本 |

## 3. 界面说明

`QPlainTextEdit`。文本可持久化；手动改文本也会更新输出。

## 4. 使用说明

调试查看对象结构，或把结构化数据发给只收字符串的节点。

## 5. 示例

Merge / Extract 结果 → From JSON → TCP Client 发送。

---

# Variable In

## 1. 节点说明

数据流**入口**：从 `ModelDataBridge` 按备注名订阅上游导出的 VariableData，输出到本图。无输入口，输出口数量可编辑。用于跨数据流、时间轴桥接。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 出 | VariableData | 桥接来的变量（可多口） |

## 3. 界面说明

`DataBridgeSelectorBox`：选择/填写备注名（quoted address）。外部：`/input`（备注）。

## 4. 使用说明

1. 在另一侧用 Variable Out 导出并设相同备注。
2. 本节点选择该备注，即可读到数据。

## 5. 示例

子图 Variable Out（备注 `score`）→ 主图 Variable In（`score`）→ Condition。

---

# Variable Out

## 1. 节点说明

数据流**出口**：将本图 VariableData 注册到 `ModelDataBridge`，供 Variable In 或其它桥接消费者读取。无输出端口；输入口可编辑，允许多连。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 入 | VariableData | 待导出数据（可多口） |

## 3. 界面说明

无内嵌控件；通过节点备注（remarks）标识导出名。

## 4. 使用说明

设置备注后，对端 Variable In 选择同名即可接收。

## 5. 示例

计算链路末级 → Variable Out（`dmx_level`）→ 另一数据流 Variable In。

---

# Image In

## 1. 节点说明

图像桥接入口：从 ModelDataBridge 按备注拉取 **ImageData**。用法同 Variable In，数据类型为图像。外部：`/input`。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 出 | ImageData | 桥接图像（可多口） |

## 3. 界面说明

`DataBridgeSelectorBox`。

## 4. 使用说明

对端使用 Image Out 并匹配备注。

## 5. 示例

渲染数据流 Image Out → 主流程 Image In → Image Display。

---

# Image Out

## 1. 节点说明

图像桥接出口：向 ModelDataBridge 导出 ImageData。无输出端口。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 入 | ImageData | 待导出图像 |

## 3. 界面说明

无内嵌控件；靠备注标识。

## 4. 使用说明

与 Image In 成对使用。

## 5. 示例

Video Decoder → Image Out（`cam_a`）。

---

# Audio In

## 1. 节点说明

音频桥接入口：从 ModelDataBridge 按备注拉取 **AudioData**。选择备注时会 `requestDataManual` 主动拉一次。外部：`/input`。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 出 | AudioData | 桥接音频 |

## 3. 界面说明

`DataBridgeSelectorBox`。

## 4. 使用说明

对端 Audio Out 匹配备注。

## 5. 示例

Audio Out（`music`）→ Audio In → Audio Device Out。

---

# Audio Out

## 1. 节点说明

音频桥接出口：导出 AudioData 到 ModelDataBridge。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 入 | AudioData | 待导出音频 |

## 3. 界面说明

无内嵌控件；靠备注标识。

## 4. 使用说明

与 Audio In 成对使用。

## 5. 示例

Audio Decoder → Audio Out（`bed`）。

---

# Image Display

## 1. 节点说明

节点内嵌 **GPU 纹理预览**。订阅系统 tick 与 ring buffer 新帧，零拷贝绑定纹理显示；输出与输入为同一 `ImageData` 句柄（透传，不复制像素）。

## 2. 端口说明

| 方向 | 类型 | 说明 |
|------|------|------|
| 入 | ImageData | 待预览图像 |
| 出 | ImageData | 透传同一句柄 |

## 3. 界面说明

`ImageTextureViewHost` 嵌入节点面板。

## 4. 使用说明

接在任意 Image 链路中查看当前帧；可继续往后连算子或 Display。

## 5. 示例

Camera → Image Display → Spout Out。

---

# Window Display

## 1. 节点说明

在独立 OpenGL 窗口中全屏/开窗显示图像。刷新策略与 Image Display 同源（tick + ring buffer）。可选择目标显示器，并用布尔口控制窗口显隐。

## 2. 端口说明

| 方向 | 端口 | 类型 | 说明 |
|------|------|------|------|
| 入 | 0 | ImageData | 显示图像 |
| 入 | 1 | VariableData | `true`/`false` 打开/关闭窗口 |
| 出 | （默认） | ImageData | 透传输入图像 |

## 3. 界面说明

- **显示器**下拉框：选择目标屏幕  
- **打开窗口**按钮：切换窗口显隐  
工程保存屏幕索引与窗口状态。

## 4. 使用说明

1. 连接图像源到端口 0。  
2. 选择显示器，点「打开窗口」或向端口 1 送布尔。  
3. 退出应用时会关闭显示窗口。

## 5. 示例

Video Decoder → Window Display（副屏演出预览）。
