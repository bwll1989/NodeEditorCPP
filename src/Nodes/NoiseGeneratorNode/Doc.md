# Noise Generator

## 1. 节点说明

生成测试用噪声音频（单声道），可切换噪声类型并调节音量，用于线路检测或占位。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 是 | 3 | 不限 | VariableData | 仅前 3 个端口参与逻辑（GAIN/TYPE/START-STOP） |
| 输出 | 1 | 是 | 1 | 不限 | AudioData | 仅端口 0 有输出 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

- **GAIN**（VariableData）：音量（dB）。
- **TYPE**（VariableData）：噪声类型索引。
- **START/STOP**（VariableData）：开始/停止生成（布尔）。

### 输出

- **Out**（AudioData）：噪声音频流。

## 3. 界面与属性

控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 含义 |
|------|------|
| 启动/停止按钮 | 勾选态按钮，启动时文字显示「停止」 |
| 噪声类型下拉框 | 0 = 白噪声（默认）、1 = 粉噪声 |
| 音量滑块 | 输出音量（dB），默认 0 dB |

可外部控制的属性（Q_PROPERTY）：`volume`（dB）、`noiseType`（0/1）、`generating`（bool）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/volume` | 读写 | 音量（dB，绑定音量滑块） |
| `/noiseType` | 读写 | 噪声类型（0=白噪声，1=粉噪声；越界值被忽略） |
| `/generating` | 读写 | 启停生成（bool，绑定启停按钮） |

## 5. 行为细节

- **音频引擎**：基于 PortAudio 回调生成，48 kHz、单声道、32 位浮点；音量以 dB 表示（0 dB = 线性 1.0）。
- **端口语义**：GAIN 以 `asNumber()` 解析为音量；TYPE 以 `asInt()` 解析为类型索引（仅接受 0/1，越界忽略）；START/STOP 以 `asBool()` 启停。
- **输出**：outData 返回新的 AudioData，指向生成器的共享环形缓冲，下游直接读取实时噪声流。
- **界面同步**：任意路径（端口 / OSC / 界面）改值都会同步控件显示；启停时以 `blockSignals` 防止按钮信号回环。
- **持久化**：`save()` 将 `volume`、`noiseType`、`generating` 写入 `values.*`；`load()` 恢复后若 generating 为 true 会自动开始生成。
- **资源清理**：析构时若仍在生成则立即停止。

## 6. 使用说明与典型接法

点「启动」或通过端口/外部地址 `/generating` 开始；`/volume`、`/noiseType` 可远程调节。

## 7. 示例

Noise Generator → Audio Analysis → 检测声卡通道是否正常。

