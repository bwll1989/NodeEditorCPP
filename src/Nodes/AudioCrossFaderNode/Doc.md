# AudioCrossFaderNode 使用说明

## 用途
AudioCrossFaderNode 用于在两路音频 A / B 之间做平滑过渡（交叉淡化），常用于场景切换、音乐/语音切换等。

## 输入端口（AudioData）
- In0：A 路音频
- In1：B 路音频

## 输出端口（AudioData）
- Out0：混合后的音频

## 界面参数
- Mix（0=A，1=B）：手动混合比例。
- Fade Time (ms)：自动过渡时长（毫秒）。
- Start A -> B：按 Fade Time 从 A 过渡到 B。
- Start B -> A：按 Fade Time 从 B 过渡到 A。
- Reset Mix：把当前 Mix 立即应用到输出。

## 快速上手
1. 把两路音频源分别接到 In0/In1。
2. 手动模式：直接调 Mix。
3. 自动模式：设置 Fade Time，然后点 Start A -> B 或 Start B -> A。
4. 将 Out0 连接到音频输出或后续处理节点。

## 注意事项
- 只有一路输入时，会直接输出现有输入；另一侧为空不会报错。
- 如果听不到声音：确认 Out0 已连接到音频输出节点，且上游音量正常。

