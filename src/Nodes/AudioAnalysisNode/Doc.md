# Audio Analysis 节点

## 1. 节点说明

分析输入音频，输出 5 路可直接用于现场控制的标量信号：三段频段能量、实时 RMS 电平、节拍脉冲。

节点内嵌参数面板，可配置频段划分与分析检测周期。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | AUDIO | AudioData | 待分析音频 |
| 1 | ENABLE | VariableData | 是否启用分析（布尔） |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | LOW | VariableData | 低频能量占比，范围 0~1 |
| 1 | MID | VariableData | 中频能量占比，范围 0~1 |
| 2 | HIGH | VariableData | 高频能量占比，范围 0~1 |
| 3 | LEVEL | VariableData | 实时 RMS 电平，范围 0~1 |
| 4 | BEAT | VariableData | 节拍脉冲，检测到重拍时为 `true` |

## 3. 界面说明

- **Enable** 按钮：开关分析输出，与 `enabled` 属性、`/enable` OSC 地址及 **ENABLE** 输入端口同步。

### 频段 (Hz)

| 参数 | 默认值 | 说明 |
|------|--------|------|
| LOW 最小/最大 | 20 / 250 | 低频段范围 |
| MID 最小/最大 | 250 / 4000 | 中频段范围 |
| HIGH 最小/最大 | 4000 / 20000 | 高频段范围 |

### 检测参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| 分析帧长 | 2048 samples | FFT 分析窗口长度，越大频率分辨率越高、响应越慢 |
| 节拍间隔 | 120 ms | 两次 BEAT 脉冲之间的最小间隔 |
| 频段 Attack | 20 ms | 频段输出上升平滑时间 |
| 频段 Release | 120 ms | 频段输出下降平滑时间 |

所有参数支持 OSC 远程控制，地址格式：`/lowMinHz`、`/frameSize`、`/beatIntervalMs` 等。

## 4. 使用说明

1. 将音频源连接到 **AUDIO**。
2. 在节点面板中按现场音乐特性调整频段与检测参数。
3. 典型接法：
   - **LEVEL** → Range Map → Art-Net（主亮度）
   - **LOW** → Art-Net（低频频闪）
   - **BEAT** → Edge Trigger → Inject（鼓点切场景）

## 5. 示例

```
背景音乐 → Audio Analysis
  ├─ LEVEL → Range Map → Art-Net Out（主亮度）
  ├─ LOW   → Art-Net Out（低频频闪）
  └─ BEAT  → Edge Trigger → Inject（切场景）
```

配置保存在 `values` 字段中，工程重开后自动恢复。
