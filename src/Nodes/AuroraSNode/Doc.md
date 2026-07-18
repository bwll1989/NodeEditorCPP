# Aurora S

## 1. 节点说明

连接 Aurora S 设备，连接成功后**先上传地图**（若配置了地图文件路径），再**自动重定位**（失败后每 5 秒重试），重定位成功后再以 **50 Hz** 输出 **6DoF 位姿**，拆分为：

- **ORIENTATION**：欧拉角 Roll / Pitch / Yaw
- **POSITION**：三维位置 X / Y / Z
- **LOCALIZATION**：定位状态

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| HOST | VariableData | 设备 IP 地址，如 `192.168.11.1` 或 `192.168.11.1:8080` |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| ORIENTATION | VariableData | 姿态欧拉角（弧度 + 角度） |
| POSITION | VariableData | 位置坐标（米） |
| CONNECTION | VariableData | 是否已连接 |
| LOCALIZATION | VariableData | 定位状态 |

### ORIENTATION 字段

| 字段 | 单位 | 说明 |
|------|------|------|
| roll | 弧度 | 横滚角 |
| pitch | 弧度 | 俯仰角 |
| yaw | 弧度 | 偏航角 |
| roll_deg / pitch_deg / yaw_deg | 度 | 同上，便于显示与控制 |
| timestamp_ns | 纳秒 | 采样时间戳 |

欧拉角采用 SDK 标准 **Roll-Pitch-Yaw** 顺序（与 `simple_pose` 示例一致）。

### POSITION 字段

| 字段 | 单位 | 说明 |
|------|------|------|
| x, y, z | 米 | 设备在 SLAM 地图/世界坐标系中的位置 |
| timestamp_ns | 纳秒 | 采样时间戳 |

### LOCALIZATION 字段

| 字段 | 类型 | 说明 |
|------|------|------|
| state | string | `unknown` / `relocalizing` / `localized` / `tracking_lost` / `relocalization_failed` / `reconnecting` |
| state_text | string | 中文状态描述 |
| localized | bool | 是否已成功定位 |
| tracking_lost | bool | 是否跟踪丢失 |
| relocalizing | bool | 是否正在重定位 |
| relocalization_failed | bool | 最近一次重定位是否失败 |
| reconnecting | bool | 是否正在断线重连 |

## 3. 使用说明

1. 在节点面板「设备地址」中填写设备 IP（如 `192.168.11.1`），可选端口（如 `192.168.11.1:8080`）。
2. 在「地图数据」中选择媒体库内的 `.stcm` 地图文件（默认 `auroramap.stcm`）。文件需位于 AppConfig 媒体库目录（`Documents/Flow/Medias`）下。留空则跳过上传，直接重定位。
3. 连接设备后自动执行：**上传地图 → 进入纯定位模式 → 重定位**；重定位失败则每 **5 秒** 重试，直到成功或断开连接。
4. **首次重定位成功** 后才开始 50 Hz 位姿输出。
5. 运行中若检测到跟踪丢失（`LOSTED`），会再次自动重定位（同样 5 秒重试）。
6. **网络断线或连接失败** 时，每 **1 秒** 自动重连；重连后跳过重复上传地图，直接重定位并恢复位姿输出。重连成功后状态提示自动清除。
7. 点击 **重新初始化** 可强制重新上传地图并重定位（已连接时立即执行；断线等待重连时会在下次连接时执行）。
8. 建图完成后请先用 `vslam_map_saveload` 或 SDK 将地图 **download** 到 PC，再在本节点配置该文件路径。
9. 下游若只需角度，接 ORIENTATION；只需坐标，接 POSITION；需要判断能否使用位姿，接 LOCALIZATION 并检查 `localized`。
10. 角度优先使用 `*_deg` 字段；需要弧度时用 `roll` / `pitch` / `yaw`。
