# TimeLineNode 使用说明

## 用途
在节点图中嵌入时间线（Timeline）编辑与播放控制，用于统一管理片段、时间轴播放、循环与定位帧等操作。

## 端口
### 输入（VariableData）
- PLAY：播放开关（bool）
- STOP：停止（bool，true 时停止）
- PAUSE：暂停（bool，true 时暂停）
- LOOP：循环开关（bool）

### 输出
无

## 外部控制（可选）
TimeLineNode 使用内部总线地址（会自动带上节点所在的 dataflow 路径）。完整地址格式：
- `/dataflow/<ParentAlias>/<NodeID>/play`（bool 或任意值）
- `/dataflow/<ParentAlias>/<NodeID>/stop`（bool 或任意值）
- `/dataflow/<ParentAlias>/<NodeID>/pause`（bool 或任意值）
- `/dataflow/<ParentAlias>/<NodeID>/loop`（bool）
- `/dataflow/<ParentAlias>/<NodeID>/currentFrame`（int64）：跳转到指定帧

同时会发布状态（State）：
- `/dataflow/<ParentAlias>/<NodeID>/play`、`/stop`、`/loop`、`/currentFrame`

## 使用步骤
1. 在 TimeLineNode 内创建/编辑轨道与片段。
2. 通过输入端口或总线地址控制播放/停止/暂停/循环。
3. 需要定位时发送 currentFrame（帧号）。 
