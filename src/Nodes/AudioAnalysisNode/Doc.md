# Audio Analysis 节点

## 1. 节点说明

分析输入音频（如电平、频谱等），将结果以 VariableData 输出，用于可视化或自动控制。

## 2. 端口说明

### 输入

- **AUDIO**（AudioData）：待分析音频。
- **ENABLE**（VariableData）：是否启用分析（布尔）。

### 输出

- **RES**（VariableData）：分析结果数值/结构。

## 3. 界面说明

无复杂控件；通过 `/enable` 开关分析。

## 4. 使用说明

1. 连接音频源到 AUDIO。
2. ENABLE 为 true 时持续更新 RES。
3. 将 RES 接到 Data Visual、Condition 等。

## 5. 示例

背景音乐 → Audio Analysis → RES → 驱动灯光亮度随音乐变化。
