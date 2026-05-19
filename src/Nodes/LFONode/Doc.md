# LFONode 使用说明

## 用途
LFONode（Low Frequency Oscillator）用于生成周期性数值信号（如正弦/方波/三角波），可用于做动画、调制参数、周期性触发等。

## 端口
- 输入（1）
  - INPUT（端口 0）：VariableData；为 true 时启动输出（等同于把 running 设为 true）
- 输出（1）
  - OUTPUT（端口 0）：VariableData

OUTPUT 内容：
- `default`：当前输出值（double）
- `method`：波形索引（int，0-2）

## 波形（method）
- 0：Sine（正弦）
- 1：Square（方波）
- 2：Triangle（三角）

## 参数与外部控制（可选）
- `/method`：波形索引（int，0-2）
- `/frequency`：频率（double，Hz）
- `/amplitude`：幅度（double）
- `/phase`：相位（double，弧度）
- `/sampleRate`：更新频率（double，每秒更新次数，越大输出更新越快）
- `/running`：是否运行（bool）

## 使用步骤
1. 设置 frequency/amplitude/sampleRate（可通过外部控制或属性面板）。
2. 把 `/running` 设为 true 或给 INPUT 输入 true 开始输出。
3. 将 OUTPUT 连接到需要被调制的参数节点或数据处理节点。

## 注意事项
- sampleRate 不是音频采样率，它表示“每秒输出更新次数”。
- sampleRate 过小会导致输出更新很慢；过大可能增加刷新频率带来的开销。

