# Audio Cross Fader

## 1. 节点说明

在多路音频 A、B 之间做交叉淡入淡出（Crossfade）。用 **Channels** 配置成对路数，各路 `Ai`↔`Bi` 共用同一个 Mix。默认 **Mix=0**（直通 A，不混音）。

## 2. 端口说明

用 **Channels** 配置路数（1–64，默认 1）。音频输入 `2N` 路，另加一路控制，输出 `N` 路。

### 输入

- **A1 … AN**（AudioData）：音源 A 各通道。
- **B1 … BN**（AudioData）：音源 B 各通道。
- **SWTCH B**（VariableData）：`true` / 非 0 → 淡入到 B；`false` / 0 → 淡入到 A。

### 输出

- **Out / Out 1 … Out N**（AudioData）：与各对 A/B 一一对应的混合结果。

例如 Channels=2 时：输入 A1、A2、B1、B2、SWTCH B，输出 Out 1、Out 2。

## 3. 参数

- **Channels**（1 ~ 64）：成对通道数。
- **Mix**（0 ~ 1）：0 为全 A（默认），1 为全 B，中间为 equal-power 过渡。
- **Fade Time**（ms）：按钮触发淡入淡出时长。
- **Action**：互斥按钮 **A → B** / **B → A**（默认选中 B→A，输出 A）。

也可通过 OSC `/channels`、`/mix`、`/fade_ms`、`/action`（0=B→A，1=A→B）、`/switch_b` 控制。

## 4. 使用说明

1. 用 Channels 设好路数。
2. 各路 A/B 分别接入对应端口。
3. 用 Action 按钮、SWTCH B，或拖动 Mix 做切换。
4. Out 接到声卡或下级处理。

## 5. 示例

立体声曲目 A/B → Channels=2 → CrossFader；TimeLine 驱动 mix → 自动淡入下一曲。
