# MpvControllerNode 使用说明

## 用途
MpvControllerNode 用于通过 HTTP 接口控制 MPV（暂停/全屏/播放列表切换/变速/音量等），并输出 MPV 的状态信息。

说明：节点会向 `http://<host>:8080/api/...` 发送请求，因此需要你的 MPV 控制服务在目标机器上运行并监听 8080 端口。

## 端口
- 输入（1）
  - COMMAND（端口 0）：VariableData（string）

当输入一个字符串（例如 `toggle_pause`），节点会请求：
`http://<host>:8080/api/toggle_pause`

- 输出（1）
  - STATUS（端口 0）：VariableData（状态信息，JSON）

## 节点参数
- hostAddress：控制服务地址（默认 127.0.0.1）
- volume：音量（0~100 的浮点值）

## 外部控制（可选）
属性：
- `/host`：设置 hostAddress（string）
- `/volume`：设置 volume（double）

命令（发送任意值或 true 触发一次）：
- `/play`：切换暂停/播放（toggle_pause）
- `/fullscreen`：切换全屏
- `/playlist_prev`：上一项
- `/playlist_next`：下一项
- `/speed_add`：速度 * 1.1
- `/speed_sub`：速度 * 0.9
- `/speed_reset`：速度重置

## 使用步骤
1. 设置 hostAddress 指向运行 MPV 控制服务的机器。
2. 直接点击节点里的按钮，或通过外部控制地址触发。
3. 从 STATUS 输出读取当前状态（用于显示/逻辑判断）。

## 注意事项
- hostAddress 不可达或服务未启动时，命令不会生效，STATUS 也不会更新。
