# FT-OMVController

## 1. 节点说明

通过全局 TCP Server（默认端口 `9090`）控制 **OMV 安卓播放器**：登记设备 ID、发送播放/停止/自定义 JSON 指令，并根据播放器回包更新播放状态。

底层由 `FTOMVController` 单例负责监听、解帧、心跳与按设备转发。创建本节点后会自动启动监听；播放状态以播放器返回数据为准（不乐观更新）。

详细帧格式与测试十六进制见：`src/Common/Devices/FTOMVController/安卓播放器通信协议.md`。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| ID | VariableData | 目标设备 ID（整数，与播放器握手登记的 ID 一致） |
| play | VariableData | `true` 时向当前 ID 发送播放指令 |
| stop | VariableData | `true` 时向当前 ID 发送停止指令 |
| send | VariableData | `true` 时发送界面中的自定义 JSON 指令 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| PLAYING | VariableData | 当前设备是否播放中（布尔）；主值为 `true`/`false`，并带 `deviceId` 字段 |

说明：

- `play` / `stop` / `send` 为脉冲触发（收到真值执行一次）。
- `PLAYING` 仅在收到匹配当前设备 ID 的状态回包后更新；断连或切换 ID 时会置为 `false`。

## 3. 界面说明

| 控件 | 说明 |
|------|------|
| 设备 ID | 当前控制的播放器 ID（0～999，默认 1） |
| 状态 | TCP Server 是否在监听（绿=监听中 / 红=未监听） |
| 播放 | 当前播放状态指示（绿=播放中 / 灰=停止） |
| JSON 编辑框 | 自定义指令内容，点 Send 或触发 `send` 端口时发送 |
| Send | 发送编辑框中的 JSON |
| Play / Stop | 发送播放 / 停止（与输入端口等效） |

外部控制（OSC / GlobalEventBus）：

| 地址 | 读写 | 说明 |
|------|------|------|
| `/deviceId` | 写 | 设置设备 ID |
| `/command` | 写 | 设置自定义 JSON 字符串 |
| `/send` | 写 | `true` 时发送当前 command |
| `/play` | 写 | `true` 时播放 |
| `/stop` | 写 | `true` 时停止 |
| `/playing` | 读 | 是否播放中 |
| `/listening` | 读 | Server 是否监听中 |

工程保存会持久化 **设备 ID** 与 **JSON 编辑框内容**。

## 4. 使用说明

1. 将节点拖入画布（注册名：`FT-OMVController`），确认本机 `9090` 端口未被占用；界面「状态」应变为「监听中」。
2. 播放器（或模拟客户端）以 TCP 客户端连接本机 `9090`，并完成握手（上报设备 ID）。
3. 将节点的设备 ID 设为与播放器一致。
4. 使用 Play/Stop 按钮，或向 `play`/`stop` 端口送入 `true` 控制播放。
5. 也可在编辑框填写 JSON，点 Send 或触发 `send` 端口发送自定义指令。
6. 将 `PLAYING` 接到指示灯、条件或互锁逻辑。

### 内置指令（Play / Stop）

| 操作 | JSON |
|------|------|
| 播放 | `{"tp":"1","com":"10","pm":"10"}` |
| 停止 | `{"tp":"1","com":"10","pm":"17"}` |
| 心跳（底层自动） | `{"tp":"1","com":"9","pm":""}` |

### 播放状态判定（与 tset.lua 一致）

对回包整帧做十六进制后（下标从 1）：

| 条件 | 含义 |
|------|------|
| `hex[23:24]=="32"` 且 `hex[67:68]=="30"` | 播放中 → `PLAYING=true` |
| `hex[23:24]=="32"` 且 `hex[67:68]=="32"` | 已停止 → `PLAYING=false` |

仅处理设备 ID 与当前节点匹配的回包。

## 5. 联调与模拟客户端

仓库提供 Python 模拟播放器：

```text
src/Common/Devices/FTOMVController/omv_client_sim.py
```

用法示例：

```bash
# 连接本机 9090，设备 ID=1（默认按 tset.lua 的 Length 少 1 规则组包）
python omv_client_sim.py

# 指定主机与设备 ID
python omv_client_sim.py -H 127.0.0.1 -p 9090 -i 12

# 对接严格按 Length 解帧的对端时，使用真实 Length
python omv_client_sim.py --correct-len
```

行为：

- 连接后只发一次握手；
- 收到 play → 回「正在播放」状态帧；
- 收到 stop → 回「已停止」状态帧；
- 交互命令：`play` / `stop` / `hs` / `status` / `quit`。

## 6. 示例

**场景触发播放**

1. 时间线或按钮 → `play` 端口送入 `true`。
2. `PLAYING` → 指示灯或 Hold，显示「播放中」。
3. 结束条件 → `stop` 端口送入 `true`。

**多设备**

1. 画布上放多个 `FT-OMVController`（共用同一 `FTOMVController` 监听）。
2. 各节点设置不同设备 ID（如 1、2、3）。
3. 分别接各自的 play/stop 与 PLAYING。

**自定义指令**

在 JSON 编辑框写入业务 JSON，例如：

```json
{"tp":"1","com":"10","pm":"10"}
```

触发 `send` 或点 Send；需播放器侧已实现对应 `com`/`pm`。

## 7. 注意事项

1. 播放器必须先握手登记，否则按 ID 发送会失败。
2. 节点间共享同一 TCP Server（单例）；勿与其它程序争用 `9090`。
3. `PLAYING` 依赖播放器状态回包；若设备不回报状态，指示不会变。
4. 兼容 Q-Sys 脚本 `tset.lua` 时，注意其实机帧的 Length 字段可能比实际 Payload 少 1；C++ 解码按标准 Length，Python 模拟器可用默认 lua 组包或 `--correct-len` 切换。
