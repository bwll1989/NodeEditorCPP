 # PJLinkNode 帮助文档

## 概述

PJLink节点是一个专业的投影仪控制接口节点，用于通过PJLink协议与支持该协议的投影仪设备进行通信。该节点实现了完整的PJLink协议规范，支持密码认证、指令发送和状态监控等功能。

**重要特性**：
- 完整的PJLink协议支持（Class 1）
- MD5密码认证机制
- 自动心跳维持连接
- 标准PJLink指令支持
- 实时连接状态监控
- 自定义指令发送功能

## PJLink协议格式

PJLink协议采用基于TCP的文本协议，标准格式如下：

### 无密码认证格式
```
Trae

参考了 1 个上下文

我将为您实现 sendCustomPJLinkCommand 方法，将输入的command按空格分割后调用 sendPJLinkCommand 发送第一位和第二位参数。

```
    /**
     * @brief 发送自定义PJLink指令
     * @param command 完整的指令字符串，格式为"指令 参数"
     * 将指令按空格分割，第一部分作为指令，第二部分作为参数发送
     */
    void sendCustomPJLinkCommand(const QString &command) {
        // 按空格分割指令
        QStringList parts = command.split(' ', 
        Qt::SkipEmptyParts);
        
        if (parts.size() >= 2) {
            // 如果有至少两个部分，第一位作为指令，第二位作为参数
            QString cmd = parts[0];
            QString param = parts[1];
            sendPJLinkCommand(cmd, param);
            qDebug() << "发送自定义PJLink指令:" << cmd << "参
            数:" << param;
        } else if (parts.size() == 1) {
            // 如果只有一个部分，作为指令发送，参数为空
            QString cmd = parts[0];
            sendPJLinkCommand(cmd, "");
            qDebug() << "发送自定义PJLink指令:" << cmd << "(无
            参数)";
        } else {
            // 如果指令为空或无效
            qDebug() << "无效的PJLink指令:" << command;
        }
    }
```
功能说明：

1. 1.
   指令分割 ：使用 split(' ', Qt::SkipEmptyParts) 按空格分割指令，忽略空字符串
2. 2.
   参数处理 ：
   - 如果有2个或以上部分：第一位作为指令，第二位作为参数
   - 如果只有1个部分：作为指令，参数为空字符串
   - 如果为空：输出错误信息
3. 3.
   调用发送 ：调用现有的 sendPJLinkCommand(cmd, param) 方法
4. 4.
   调试输出 ：添加调试信息便于跟踪指令发送
使用示例：

- 输入： "POWR 1" → 发送指令： POWR ，参数： 1
- 输入： "POWR ?" → 发送指令： POWR ，参数： ?
- 输入： "INPT 11" → 发送指令： INPT ，参数： 11

bwll1989

复制

删除

Doc.md 更新文档

Trae

参考了 1 个上下文

Doc.md

我发现当前的文档内容与实际的PJLinkNode功能不符。文档中描述的是TSETL协议节点，但实际上这是一个PJLink协议节点。我将为您更新正确的PJLink节点文档。

```
# PJLinkNode 帮助文档

## 概述

PJLink节点是一个专业的投影仪控制接口节点，用于通过PJLink协议与支持该
协议的投影仪设备进行通信。该节点实现了完整的PJLink协议规范，支持密码认
证、指令发送和状态监控等功能。

**重要特性**：
- 完整的PJLink协议支持（Class 1）
- MD5密码认证机制
- 自动心跳维持连接
- 标准PJLink指令支持
- 实时连接状态监控
- 自定义指令发送功能

## PJLink协议格式

PJLink协议采用基于TCP的文本协议，标准格式如下：

### 无密码认证格式
```
%1指令 参数
```
### 有密码认证格式
**认证流程**：
1. 客户端连接到投影仪（默认端口4352）
2. 投影仪返回认证信息：`PJLINK 0` 或 `PJLINK 1 随机密钥`
3. 如需认证，客户端计算MD5(随机密钥+密码)
4. 发送带认证哈希的指令

## 支持的PJLink指令

| 指令 | 参数 | 功能 | 示例 |
|------|------|------|------|
| POWR | 0/1/? | 电源控制/查询 | `%1POWR 1` (开机) |
| INPT | 输入源/? | 输入源切换/查询 | `%1INPT 11` (RGB1) |
| AVMT | 10/11/20/21/30/31/? | 静音控制/查询 | `%1AVMT 11` (视频静音开) |
| ERST | ? | 错误状态查询 | `%1ERST ?` |
| LAMP | ? | 灯泡状态查询 | `%1LAMP ?` |
| INST | ? | 输入源列表查询 | `%1INST ?` |
| NAME | ? | 投影仪名称查询 | `%1NAME ?` |
| INF1 | ? | 制造商信息查询 | `%1INF1 ?` |
| INF2 | ? | 产品名称查询 | `%1INF2 ?` |
| INFO | ? | 其他信息查询 | `%1INFO ?` |
| CLSS | ? | 类别信息查询 | `%1CLSS ?` |

## 输入端口

- **TRIGGER**: 触发信号输入（VariableData类型）
  - 接收到信号时触发指令发送
  - 支持任何类型的触发数据

## 输出端口

- **CONNECTION**: 连接状态输出（布尔类型）
  - true: 已连接到投影仪
  - false: 未连接或连接断开

## 界面控件

### 连接设置组
1. **主机地址输入框**：
   - 投影仪的IP地址
   - 默认值：192.168.1.100
   - 支持域名和IP地址

2. **密码输入框**：
   - PJLink认证密码
   - 如果投影仪未设置密码则留空
   - 支持任意长度密码

3. **连接状态标签**：
   - 显示当前连接状态
   - 绿色"已连接"或红色"未连接"

### 控制功能
1. **电源开按钮**：发送开机指令 `POWR 1`
2. **电源关按钮**：发送关机指令 `POWR 0`
3. **静音开按钮**：发送静音指令 `AVMT 31`
4. **静音关按钮**：发送取消静音指令 `AVMT 30`
5. **自定义命令输入框**：
   - 输入自定义PJLink指令，格式为 `指令 参数`
   - 例如：`POWR 1` 或 `INPT 11`
6. **自定义命令发送按钮**：
   - 发送自定义命令输入框中的指令
   - 指令发送成功后，会在连接状态标签中显示成功信息

## 使用示例

### 示例1：基本投影仪控制

1. 添加PJLink节点到流程图
2. 设置投影仪IP地址（如：192.168.1.100）
3. 如果投影仪设置了密码，在密码框中输入
4. 节点会自动连接到投影仪
5. 使用界面按钮控制投影仪电源和静音
6. 通过CONNECTION输出端口监控连接状态

### 示例2：触发式控制

1. 将其他节点的输出连接到PJLink节点的TRIGGER输入
2. 当触发信号到达时，可以执行预设的控制操作
3. 结合条件判断节点实现智能控制逻辑

### 示例3：自定义指令发送

使用 `sendCustomPJLinkCommand` 方法发送自定义指令：

```cpp
// 发送输入源切换指令
sendCustomPJLinkCommand("INPT 11");

// 查询投影仪状态
sendCustomPJLinkCommand("POWR ?");

// 查询灯泡状态
sendCustomPJLinkCommand("LAMP ?");
```
## 参考资料

- [PJLink协议规范](https://pjlink.jbmia.or.jp/english/)
- [PJLink Class 1 指令集](https://pjlink.jbmia.or.jp/english/data/5-1_PJLink_eng_20131210.pdf)
- [投影仪厂商PJLink支持列表](https://pjlink.jbmia.or.jp/english/list.html)