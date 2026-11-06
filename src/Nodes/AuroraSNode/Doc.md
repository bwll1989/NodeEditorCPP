# Aurora S

## 1. 节点说明

连接 Aurora S 设备，连接成功后**先上传地图**（若配置了地图文件路径），再**自动重定位**（失败后每 5 秒重试），重定位成功后再以 **50 Hz** 输出 **6DoF 位姿**，拆分为：

- **ORIENTATION**：map，含 `rad` / `deg` 两个 `[roll, pitch, yaw]` 向量
- **POSITION**：`[x, y, z]`（m）
- **LOCALIZATION**：`bool`（是否已定位）

**节点间单位约定**：位置 **m**，姿态控制用 **rad**（`ORIENTATION.rad` / `default`）；`deg` 仅便于显示。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 否 | 1 | 1 | — | 无 |
| 输出 | 4 | 否 | 4 | 4 | — | 末端口为 LOCALIZATION（定位布尔） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| HOST | VariableData | 设备 IP 地址，如 `192.168.11.1` 或 `192.168.11.1:8080` |

### 输出

| 端口 | 载荷 | 说明 |
|------|------|------|
| ORIENTATION | map | `rad` / `deg` / `default` 均为 `[roll,pitch,yaw]`；控制取 `rad`（或 `default`） |
| POSITION | `[x, y, z]` | 位置，米 |
| CONNECTION | `bool` | 是否已连接 |
| LOCALIZATION | `bool` | 是否已成功定位（`localized`） |

欧拉角采用 SDK 标准 **Roll-Pitch-Yaw** 顺序。面板显示使用 `deg` 向量。

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| host | string | 空 | IPv4 或 `IP:端口` | 设备地址，对应面板「设备地址」，OSC `/host` |
| mapFilePath | string | auroramap.stcm | 媒体库内 `.stcm` 路径 | 地图文件（`Documents/Flow/Medias` 下），对应面板「地图数据」，OSC `/mapFilePath` |
| connected | bool | false | 只读 | 连接状态，OSC `/connected`（状态反馈） |
| reinitialize | bool | false | true 触发 | 置 true 强制重新上传地图并重定位，对应面板「重新初始化」，OSC `/reinitialize` |

内嵌界面控件：设备地址输入框、地图数据选择、连接/定位状态显示、位姿数值显示（deg）、重新初始化按钮。`host`、`mapFilePath` 保存到工程并随工程恢复。

## 4. 外部控制（OSC 地址）

### OSC 地址

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 类型 | 范围 | 说明 |
|------|------|------|------|
| /host | string | IPv4 或 IP:端口 | 对应属性 host |
| /mapFilePath | string | 媒体库路径 | 对应属性 mapFilePath |
| /connected | bool | 只读 | 连接状态反馈 |
| /reinitialize | bool | true 触发 | 对应属性 reinitialize |

## 5. 行为细节

- 会话启动时机：构造和工程 load 阶段都不连设备；节点就绪后才用当前 host / 地图启动会话——保证工程恢复的地图文件优先生效，不会被默认文件名抢先。
- 参数变更即重启：`host` 或地图路径变化时，若会话活跃先请求停止，会话结束后用最新参数重新启动；连续修改只保留最终值（排队去重，启动时再读 pending）。
- 重新初始化是脉冲语义：置 `true` 触发一次「重新上传地图 + 重定位」，属性随即自动复位为 `false`（反馈会先推 true 再推 false）。
- 输出节流：位姿端口（ORIENTATION / POSITION）每样本都更新；面板数值显示限流 100ms 刷新一次。
- LOCALIZATION 端口只输出 `localized` 布尔；面板显示完整的定位状态明细。
- `/connected` 绑定无界面控件，仅作状态反馈（连接状态变化时自动推送）。
- worker 运行在独立线程，全部经队列信号桥接回 UI；析构不阻塞 UI 线程等待 SDK 清理（拆离线程后异步回收）。
- 端口连接策略：输入、输出均为 Many。
- 持久化：`values` 子对象两键 `Host` / `MapFilePath`（键名大写开头）；连接状态与定位状态不持久化。

## 6. 使用说明与典型接法

1. 在节点面板「设备地址」中填写设备 IP（如 `192.168.11.1`），可选端口（如 `192.168.11.1:8080`）。
2. 在「地图数据」中选择媒体库内的 `.stcm` 地图文件（默认 `auroramap.stcm`）。文件需位于 AppConfig 媒体库目录（`Documents/Flow/Medias`）下。留空则跳过上传，直接重定位。工程保存的 `MapFilePath` 会在**再次打开工程并完成节点就绪后**自动用于上传（不会先用默认文件名起会话）。
3. 连接设备后自动执行：**上传地图 → 进入纯定位模式 → 重定位**；重定位失败则每 **5 秒** 重试，直到成功或断开连接。
4. **首次重定位成功** 后才开始 50 Hz 位姿输出。
5. 运行中若检测到跟踪丢失（`LOSTED`），会再次自动重定位（同样 5 秒重试）。
6. **网络断线或连接失败** 时，每 **1 秒** 自动重连；重连后跳过重复上传地图，直接重定位并恢复位姿输出。
7. 点击 **重新初始化** 可强制重新上传地图并重定位。
8. 下游用 `asFloats` / `asBool` 按需截取：平面导航取 POSITION 前 2 维、ORIENTATION 的 `rad`（或 `default`）中 yaw（第 3 元）。

