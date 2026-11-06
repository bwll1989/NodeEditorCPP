# PosiStageNet Client

## 1. 节点说明

PosiStageNet Client 是 PSN（PosiStageNet）舞台定位协议接收节点：加入 UDP 组播组持续接收 PSN 数据包，解码后输出在线追踪器数量与全部追踪器的位置/速度/朝向等信息。

典型使用场景：接收舞台跟踪系统（如 viGema、kaposzta PSN Server、Navgi 等支持 PSN 输出的设备）的演员/道具坐标，用于投影映射实时校正、灯光跟随、音响对象追踪等。

需要发送 PSN 数据时用 PosiStageNet 服务器端软件；本节点只负责接收解码。若只需要单个数值（如某个轴的 X 坐标），将 TRACKERS 输出接到 Extract 节点按 key 取值。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 否 | 1 | 1 | — | 末端口为 ENABLE（启停） |
| 输出 | 3 | 否 | 3 | 3 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| ENABLE | VariableData | 否 | 接收开关（真值兼容 bool/非 0 int/字符串 "true"/"1"）；断开时：停止接收并断开连接；非法值时：视为 false 停止接收 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| CONNECTED | VariableData | bool。是否正在收到有效 PSN 数据（事件驱动：收到首包置 true；看门狗检测到 2 秒无数据置 false） |
| TRACKER_COUNT | VariableData | int。当前帧追踪器数量（事件驱动：PSN frame_id 变化时更新） |
| TRACKERS | VariableData | QVariantMap。完整追踪器数据（事件驱动，结构见下） |

TRACKERS 输出的 Map 结构：

```text
{
  "system_name":     "..."        // 发送端系统名
  "frame_id":        N            // PSN 帧号
  "timestamp_usec":  N            // 帧时间戳（微秒）
  "trackers": [
    { "id": 1, "name": "T1",
      "pos":  [x, y, z],          // 位置，仅发送端设置时存在
      "speed": [x, y, z],         // 速度（可选）
      "ori":   [x, y, z],         // 朝向（可选）
      "accel": [x, y, z],         // 加速度（可选）
      "target_pos": [x, y, z],    // 目标位置（可选）
      "status": 0, "timestamp": 0 }
  ]
}
```

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| enabled | bool | false | true/false | 启用接收；true 时绑定端口并加入组播组 |
| connected | bool | false | 只读 | 当前连接状态（界面状态灯同步显示） |
| multicastAddress | string | 236.10.10.10 | 合法 IPv4 组播地址 | PSN 组播地址；修改后正在接收时自动重启接收器 |
| port | int | 56565 | 1–65535 | 组播端口；修改后正在接收时自动重启接收器 |

内嵌界面控件：启用复选框、组播地址输入框、端口数值拖拽框、连接状态标签、系统名与帧号/追踪器数显示、各追踪器名称与实时坐标列表。

## 4. 外部控制（OSC 地址）

### OSC 地址

地址前缀：`/dataflow/{别名}/{节点ID}/…`（{别名} 为节点别名，{节点ID} 为节点编号）

| 地址 | 类型 | 范围 | 说明 |
|------|------|------|------|
| /enabled | bool | true/false | 对应属性 enabled，启用/停止接收 |
| /multicastAddress | string | IPv4 组播地址 | 对应属性 multicastAddress |
| /port | int | 1–65535 | 对应属性 port |
| /connected | bool | 只读 | 连接状态反馈（状态树回读） |

## 5. 行为细节

- **状态与持久化**：工程保存 enabled、multicastAddress、port；重新打开工程后按保存值恢复，若 enabled 为 true 自动开始接收。connected 为运行时状态，不持久化。
- **时序/性能**：5 ms 定时轮询 socket 缓冲，逐包送 psn_decoder；仅在 frame_id 变化（新帧）时刷新输出与界面列表，不重复推送。
- **错误行为**：组播地址非法或 bind/joinMulticastGroup 失败时保持 connected=false 且不启动轮询；接收端停止发送时，看门狗在 2 秒后将 connected 置 false（TRACKER_COUNT/TRACKERS 保持最后值）；UDP 包解码失败时静默丢弃；追踪器无名字时回退用发送端 info 中的名字表。

## 6. 使用说明与典型接法

1. 舞台定位数据接入：PosiStageNet Client（enabled=true，地址/端口与发送端一致）→ TRACKERS → Extract（按 key 取 trackers）→ 下游使用；或 TRACKER_COUNT 直接用于人数统计。
2. 单坐标跟随：TRACKERS → Extract 提取某追踪器 pos → Math/ValueLookup 换算 → 投影/灯光控制节点。
3. 断链保护：ENABLE 接总控布尔（如 Toggle Variable），演出前统一切换；CONNECTED 接 Condition 做信号丢失告警。

