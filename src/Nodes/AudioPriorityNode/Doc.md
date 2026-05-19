# AudioPriorityNode 使用说明

## 用途
AudioPriorityNode 用于“前景优先”：当旁链/前景音出现时，自动压低背景音，并把前景音更清晰地混合到输出。

## 输入端口（AudioData）
- In0：背景音（BGM）。
- In1：前景/旁链音（人声、提示音等）。

## 输出端口（AudioData）
- Out0：优先混合输出（背景被压低 + 前景更突出）。

## 常用参数
- Threshold：触发前景优先的阈值。
- Attack / Release：进入/恢复速度。
- Depth（或最大压低深度）：背景最多被压低多少。
- Makeup：压低后的补偿（如界面提供）。
- Sidechain Gain：旁链检测灵敏度（如界面提供）。

## 快速上手
1. 把 BGM 接到 In0，把人声/提示音接到 In1。
2. 先调 Threshold，确认前景出现时背景会下降。
3. 调 Attack/Release，让变化更自然。
4. 用 Depth/Makeup 微调整体听感。

## 注意事项
- 背景仍然太吵：加大压低深度或降低 Threshold。
- 一直在压低：提高 Threshold 或降低旁链增益。
