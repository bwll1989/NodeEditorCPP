# AudioDuckingNode 使用说明

## 用途
AudioDuckingNode 用于“旁链压低”：当旁链/前景音（如人声、解说）出现时，自动降低背景音乐音量。

## 输入端口（AudioData）
- In0：背景音（BGM）。
- In1：旁链/前景音（人声/提示音）。

## 输出端口（AudioData）
- Out0：压低后的混合输出。

## 常用参数
- Threshold：触发压低的阈值。
- Attack / Release：压低进入/恢复速度。
- Ratio / Depth：压低强度。
- Mix：整体混合比例（如果界面提供）。

## 快速上手
1. 把 BGM 接到 In0，把人声/提示音接到 In1。
2. 先把 Threshold 调低一些，确认能触发压低。
3. 调 Attack/Release，让压低更自然。
4. 根据需要调强度（Ratio/Depth）。

## 注意事项
- 压得太狠：降低强度或提高 Threshold。
- 压不下去：提高旁链电平或降低 Threshold。
