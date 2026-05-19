# OscInNode 使用说明

## 用途
监听指定 UDP 端口，接收 OSC 消息，并把“地址”和“参数值”输出给下游节点。

## 端口
### 输入
无

### 输出（VariableData）
- RESULT：原始结果（键值表）
- ADDRESS：OSC 地址（字符串）
- VALUE：OSC 参数值（当前实现输出为字符串）

## 参数/界面
- port：监听端口（默认 6000）
- address：显示最近一次收到的 OSC 地址
- value：显示最近一次收到的参数值

## 外部控制（可选）
- /port（int）：设置监听端口

## 输出字段（RESULT）
RESULT 为一个键值表，常用字段：
- address：OSC 地址（例如 /led/color）
- default：参数值（当前实现以字符串形式保存/显示）

## 使用步骤
1. 把 OscInNode 放入流程，设置 port。
2. 用任意 OSC 客户端向该端口发送消息（例如 /foo 123）。
3. 需要做路由/过滤时，使用 ADDRESS 输出做匹配；需要取参数值时使用 VALUE 输出或 RESULT.default。

## 注意事项
- 端口被占用时将无法正常接收；请更换端口或关闭占用程序。
- 当前 VALUE 输出为字符串形式；若需要数值运算，可在下游做转换。 
