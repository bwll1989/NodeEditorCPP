# CAN Bus (candleLight / gs_usb)

Windows 下通过 WinUSB + candle API 访问 candleLight / CANable 等 gs_usb 设备。

## 端口

| 方向 | 端口 | 说明 |
|------|------|------|
| In | ID | CAN ID（HEX 字符串或含 `id`/`data` 的 map） |
| In | DATA | 数据 HEX |
| In | TRIGGER | 触发发送 |
| Out | FRAME | 收到的帧 map：`id`/`id_hex`/`extended`/`rtr`/`dlc`/`data`/`timestamp_us` |

## 使用

1. 插入 candleLight 适配器（设备管理器显示 WinUSB）
2. 添加 **Connect → CAN Bus** 节点
3. Refresh Devices → 选择设备 → 设置 Bitrate → 确认 Connected
4. 填写 CAN ID / Data 后 Send，或从输入口注入
