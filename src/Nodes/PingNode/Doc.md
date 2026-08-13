# Ping

## 1. 节点说明

用系统 `ping` 检测主机是否可达。适用于没有状态回传的设备，作为可访问性指示。

## 2. 端口

**输入（1）：** ENABLE  
**输出（1）：** CONNECTED

| 端口 | 行为 |
|------|------|
| ENABLE | `true`：立即 ping，之后固定每 **10 秒**；`false`：停止，CONNECTED 置为不可达 |
| CONNECTED | 最近一次 ping 是否成功 |

## 3. 界面

| 控件 | 说明 |
|------|------|
| Host | 目标 IP / 主机名（默认 `127.0.0.1`） |
| 启用 | 与 ENABLE 端口相同：开则每 10s ping，关则停止 |
| 状态 | 可达（绿）/ 不可达（红） |

## 4. 行为说明

- Windows：`ping -n 1 -w 3000 <host>`
- Linux/macOS：`ping -c 1 -W 3 <host>`
- 以进程退出码 `0` 判定可达
- 同一时刻只运行一个 ping；上一次未结束则跳过本次
- 工程保存 `host`、`enabled`
