# TSETLNode 帮助文档

## 概述

TSETL节点是一个触发事件收集系统接口节点，用于接收和解析TSETL（Trigger Signal Event Transfer Layer）协议消息。该节点专门处理包含SignalID的触发事件，并将解析后的数据以VariableData的形式从输出端口输出。

**重要特性**：
- 仅响应SignalID类型的消息，忽略心跳等其他消息类型
- 实现完整的Modbus CRC16校验算法
- 提供详细的调试输出和日志记录
- 优化的端口设计，专注于信号处理

## 协议格式

TSETL采用自定义的消息帧结构：

| 包头(4bytes) | 数据段长度(2bytes) | 数据段数据 | 数据段CRC16(2bytes) |
|-------------|------------------|-----------|-------------------|
| 十六进制 | 十六进制低字节在前 | Asc码字符串 | 十六进制低字节在前 |
| 0xFBFBFBFB | Json数据包的总字节数 | Json数据包 | Json数据包的CRC校验结果 |

**注意**：
- "数据段长度"仅计算"数据段数据"的总字节数，不包含"包头"、"数据段长度"、"数据段CRC"的字节数
- CRC16校验使用Modbus算法，计算范围仅为JSON数据部分
- 字节序采用低字节在前（Little Endian）

### CRC16校验算法

节点使用标准的Modbus CRC16算法进行数据完整性校验：
- 多项式：0xA001（反向）
- 初始值：0xFFFF
- 计算范围：仅JSON数据部分
- 结果格式：低字节在前

## JSON数据格式

### 基本结构
```json
{
    "MsgID": "",
    "FromObject": "TSETL",
    "ToObject": "QSC",
    "Datas": {}
}
```

### 支持的消息类型

| MsgID | Datas示例 | 处理方式 |
|-------|-----------|----------|
| HeartBeat | `{"DateTime": "2022-10-01 00:00:00.001"}` | **忽略处理** |
| HeartBeatReply | `{"DateTime": "2022-10-01 00:00:00.001"}` | **忽略处理** |
| SignalID | `{"DateTime": "2022-10-01 00:00:00.001", "SignalID": "10001"}` | **正常处理并输出** |

**注意**：节点仅处理MsgID为"SignalID"的消息，其他类型的消息将被忽略。

## 输入端口

- **HOST**: 服务器主机地址（字符串类型）
- **PORT**: 服务器端口号（整数类型）

## 输出端口

- **SIGNAL_ID**: 提取的SignalID（字符串类型，仅SignalID消息有效）
- **JSON_DATA**: 完整的JSON数据（字符串类型，包含原始JSON内容）
- **CONNECTION**: 连接状态（布尔类型，true表示已连接）

## 界面

节点界面包含以下部分：

1. **连接设置**：
   - 主机地址：TSETL服务器的IP地址（默认：127.0.0.1）
   - 端口：TSETL服务器的TCP端口（默认：2001）

2. **连接状态**：
   - 显示当前与TSETL服务器的连接状态
   - 绿色表示已连接，红色表示未连接

3. **信号信息**：
   - 最新信号：显示最近接收到的SignalID
   - 时间：显示信号的时间戳
   - 消息计数：总接收消息数量（仅计算SignalID消息）

## 使用示例

### 示例1：基本信号接收

1. 添加TSETL节点到流程图
2. 设置正确的主机地址和端口
3. 节点会自动连接到TSETL服务器
4. 连接成功后，开始接收和解析TSETL消息
5. 从SIGNAL_ID输出端口获取触发信号ID
6. 从JSON_DATA输出端口获取完整的JSON数据

### 示例2：信号触发处理

1. 将TSETL节点的SIGNAL_ID输出连接到条件判断节点
2. 根据不同的SignalID执行不同的操作
3. 使用JSON_DATA输出获取时间戳等详细信息
4. 通过CONNECTION输出监控连接状态

### 示例3：信号过滤和处理

1. 将SIGNAL_ID输出连接到字符串比较节点
2. 过滤特定的SignalID值
3. 使用JSON_DATA进行进一步的数据解析
4. 实现基于SignalID的条件触发逻辑

## 技术细节

### 协议解析

- **消息头验证**：检查0xFBFBFBFB包头
- **长度校验**：验证数据段长度的一致性
- **CRC16校验**：使用Modbus算法验证JSON数据完整性
- **JSON解析**：解析数据段中的JSON内容
- **类型过滤**：仅处理MsgID为"SignalID"的消息

### 数据处理

- **选择性处理**：仅处理SignalID类型消息，忽略心跳等其他消息
- **实时解析**：接收到有效消息后立即解析
- **数据提取**：提取SignalID、时间戳等关键信息
- **状态管理**：维护连接状态和统计信息

### CRC16校验实现

```cpp
/**
 * @brief 计算Modbus CRC16校验码
 * @param data 待校验的数据
 * @return 计算得到的CRC16值（低字节在前）
 */
quint16 calculateModbusCRC16(const QByteArray& data);

/**
 * @brief 验证CRC16校验码
 * @param jsonData JSON数据
 * @param expectedCRC 期望的CRC16值
 * @return 校验是否通过
 */
bool validateCRC16(const QByteArray& jsonData, quint16 expectedCRC);
```

### 调试功能

节点提供详细的调试输出，包括：
- 接收到的原始数据（十六进制格式）
- JSON数据长度和内容
- CRC16校验过程和结果
- 消息解析状态
- 连接状态变化

### 错误处理

- **连接异常**：自动重连机制
- **协议错误**：跳过无效消息，继续处理
- **JSON错误**：记录错误信息，不中断处理
- **CRC错误**：记录校验失败，可选择忽略或拒绝消息
- **消息过滤**：非SignalID消息被静默忽略

## 测试数据

### 标准测试包

1. **简单信号包（SignalID: 999）**
```json
{
   "MsgID":"SignalID",
   "FromObject":"TSETL",
   "ToObject":"QSC",
   "Datas":{
      "DateTime":"2022-10-01 00:00:00.001",
      "SignalID":"10001"
      }
}
```
```
FB FB FB FB 7C 00 7B 22 4D 73 67 49 44 22 3A 22 53 69 67 6E 61 6C 49 44 22 2C 22 46 72 6F 6D 4F 62 6A 65 63 74 22 3A 22 54 53 45 54 4C 22 2C 22 54 6F 4F 62 6A 65 63 74 22 3A 22 51 53 43 22 2C 22 44 61 74 61 73 22 3A 7B 22 44 61 74 65 54 69 6D 65 22 3A 22 32 30 32 32 2D 31 30 2D 30 31 20 30 30 3A 30 30 3A 30 30 2E 30 30 31 22 2C 22 53 69 67 6E 61 6C 49 44 22 3A 22 31 30 30 30 31 22 7D 7D 29 21  
```
