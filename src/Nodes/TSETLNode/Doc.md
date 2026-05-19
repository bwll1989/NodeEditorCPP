# TSETLNode 使用说明

## 用途
连接到 TSETL 服务端（TCP），接收并解析消息，输出 SignalID 与原始 JSON 数据。

说明：
- 节点仅处理 `MsgID=SignalID` 的消息，其它消息会忽略。
- 连接成功后会自动发送心跳维持连接（约每 5 秒一次）。

## 端口
### 输入（VariableData）
- HOST：服务器地址（string）
- PORT：服务器端口（int）

### 输出（VariableData）
- SIGNAL_ID：提取到的 SignalID（string）
- JSON_DATA：原始 JSON 数据（键值表或字符串，取决于消息内容）
- CONNECTION：连接状态（bool）

## 参数/界面
- Host / Port：连接设置
- Connection：连接状态显示

## 外部控制（可选）
### 属性（写入）
- /host（string）
- /port（int）

### 反馈（只读）
- /connect：连接状态

## 使用步骤
1. 设置 HOST/PORT，节点会自动连接。
2. 从 SIGNAL_ID 获取触发信号编号；需要更多字段时读取 JSON_DATA。
3. 用 CONNECTION 或 /connect 监控在线状态。 
