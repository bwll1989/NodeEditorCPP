# USR-IO808Node 使用说明

## 用途
通过 Modbus TCP 控制 USR-IO808 的 8 路数字输出（DO0~DO7），并读取 8 路数字输入（DI0~DI7）。

## 端口
### 输入（VariableData）
- DO 0 ~ DO 7：数字输出控制（bool）

### 输出（VariableData）
- DI 0 ~ DI 7：数字输入状态（bool）

## 参数/界面
- Host：设备 IP（默认 127.0.0.1）
- Port：设备端口（默认 8080）
- ServerID：UnitId（默认 1）
- Read All：手动刷新一次输入/输出状态

## 外部控制（可选）
### 属性（写入）
- /host（string）
- /port（int）
- /serverId（int）

### 输出控制（写入）
- /DO0 ~ /DO7（bool）：设置对应 DO 输出

### 反馈（只读）
- /connect：连接状态
- /DI0 ~ /DI7：输入状态（用于外部显示/读取）

## 地址映射（设备侧）
- DO：0x0000 ~ 0x0007（8 个线圈）
- DI：0x0020 ~ 0x0027（8 个离散输入）

## 使用步骤
1. 设置 Host/Port/ServerID，等待连接成功（/connect）。
2. 通过 DO 端口或 /DO* 控制输出。
3. 从 DI 输出端口读取输入状态，需要时点 Read All 刷新。 
