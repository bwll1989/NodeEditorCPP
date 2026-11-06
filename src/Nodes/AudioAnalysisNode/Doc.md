# Audio Analysis

## 1. 节点说明

分析输入音频，输出 5 路可直接用于现场控制的标量信号：三段频段能量（LOW/MID/HIGH）、实时 RMS 电平（LEVEL）、节拍脉冲（BEAT）。

节点内嵌参数面板，可配置频段划分与分析检测参数；分析在独立工作线程中执行（FFT），不阻塞节点图。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | —（不可新增） | 末端口为 ENABLE（启停控制） |
| 输出 | 5 | 否 | 5 | 5 | —（不可新增） | 末端口为 BEAT（节拍脉冲 bool） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| AUDIO | AudioData | 是 | 待分析音频流；断开时：各输出保持最后值，不再更新 |
| ENABLE | VariableData | 否 | 启停控制（布尔），与 `enabled` 属性同步；断开时：保持当前值 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| LOW | VariableData | 低频段（默认 20～250 Hz）归一化能量，0～1，随 Attack/Release 平滑 |
| MID | VariableData | 中频段（默认 250～4000 Hz）归一化能量，0～1 |
| HIGH | VariableData | 高频段（默认 4000～20000 Hz）归一化能量，0～1 |
| LEVEL | VariableData | 实时 RMS 电平，0～1 |
| BEAT | VariableData | 节拍脉冲：检测到重拍时为 `true`，两次脉冲间隔不小于 `beatIntervalMs` |

## 3. 界面与属性

内嵌参数面板（`WidgetEmbeddable = true`）：Enable 开关 + 三段频段范围拖动条 + 检测参数拖动条，控件与同名属性双向同步。

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| enabled | bool | false | true/false | 启停分析，对应 Enable 按钮 |
| lowMinHz / lowMaxHz | double | 20 / 250 | Hz | 低频段范围 |
| midMinHz / midMaxHz | double | 250 / 4000 | Hz | 中频段范围 |
| highMinHz / highMaxHz | double | 4000 / 20000 | Hz | 高频段范围 |
| frameSize | int | 2048 | samples | FFT 窗长，越大频率分辨率越高、响应越慢 |
| beatIntervalMs | int | 120 | ms | 两次 BEAT 脉冲最小间隔 |
| bandAttackMs | double | 20 | ms | 频段输出上升平滑时间 |
| bandReleaseMs | double | 120 | ms | 频段输出下降平滑时间 |

全部参数保存到工程（`values.enable`、`values.lowMinHz`…）并在加载时恢复。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`（{别名} 为节点别名，{节点ID} 为节点编号）

| 地址 | 类型 | 范围 | 说明 |
|------|------|------|------|
| /enable | bool | true/false | 启停分析，等价于 ENABLE 端口 |
| /lowMinHz | float | Hz | 低频段下限 |
| /lowMaxHz | float | Hz | 低频段上限 |
| /midMinHz | float | Hz | 中频段下限 |
| /midMaxHz | float | Hz | 中频段上限 |
| /highMinHz | float | Hz | 高频段下限 |
| /highMaxHz | float | Hz | 高频段上限 |
| /frameSize | int | samples | FFT 窗长 |
| /beatIntervalMs | int | ms | 节拍最小间隔 |
| /bandAttackMs | float | ms | 频段 Attack |
| /bandReleaseMs | float | ms | 频段 Release |

以上地址均通过 `registerExternalBinding` 注册（与内嵌控件双向绑定），并订阅全局事件总线 Command 事件，外部命令直接更新属性并同步到工作线程。

## 5. 行为细节

- **状态与持久化**：11 个参数全部保存到工程，重开工程后自动恢复；输出为实时计算值，不持久化。
- **时序/性能**：FFT 分析在独立 QThread 工作线程执行；参数变更即时推送到工作线程（`pushConfigToWorker`）；退出时最多等待线程 3 秒。
- **错误行为**：AUDIO 断开时输出保持最后值；`enabled = false` 时停止分析输出；频段范围倒挂（min > max）时由频段计算按实际数据退化（建议保持 min < max）。

## 6. 使用说明与典型接法

1. 将音频源连接到 **AUDIO**。
2. 在节点面板中按现场音乐特性调整频段与检测参数。
3. 典型接法：
   - **LEVEL** → Range Map → Art-Net（主亮度）
   - **LOW** → Art-Net（低频频闪）
   - **BEAT** → Edge Trigger → Inject（鼓点切场景）

## 7. 示例

```
背景音乐 → Audio Analysis
  ├─ LEVEL → Range Map → Artnet Out（主亮度）
  ├─ LOW   → Artnet Out（低频频闪）
  └─ BEAT  → Edge Trigger → Inject（切场景）
```

配置保存在 `values` 字段中，工程重开后自动恢复。
