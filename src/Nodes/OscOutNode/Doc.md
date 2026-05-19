# OscOutNode 使用说明

## 用途
向指定主机/端口发送 OSC 消息（UDP）。

## 端口
### 输入（VariableData）
- HOST：目标主机（例如 127.0.0.1）
- PORT：目标端口（int）
- ADDRESS：OSC 地址（例如 /test）
- VALUE：要发送的值（字符串形式输入，按 Type 转换）
- TRIGGER：触发发送（bool，true 时触发一次）

### 输出
无

## 参数/界面
- Host：目标主机
- Port：目标端口
- Address：OSC 地址
- Type：数据类型
  - 0：Int
  - 1：Float
  - 2：String
- Value：要发送的值
- Send：按钮，点击立即发送一次

## 外部控制（可选）
- /host（string）
- /port（int）
- /address（string）
- /value（string）
- /type（int，0/1/2）
- /send（bool 或任意值）：触发发送一次

## 发送行为（重要）
- 当 ADDRESS 改变时，会立即发送一次。
- 当 VALUE 改变时，会立即发送一次。
- 当 TRIGGER 输入为 true 时，会触发发送一次。

## 使用步骤
1. 设置 HOST/PORT/ADDRESS/TYPE。
2. 通过界面修改 Value，或将 VALUE 端口连接到上游数据源。
3. 需要“按一下才发”时，用 TRIGGER 或 /send 来触发。

## 注意事项
- VALUE 以字符串输入，最终发送类型由 Type 决定；发送 Int/Float 时会做 toInt/toDouble 转换。
- 高频率改动 VALUE 会造成频繁发送；如需限速，建议在上游做节流/采样。 
