# Nav Missions

## 1. 节点说明

面板分类：**Controls → Nav Missions**

航点任务层：根据 **Nav 的 ARRIVED** 推进步骤，向 **Nav Controller** 下发目标点；定位只进 Nav，本节点不吃 POSITION。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 无 |
| 输出 | 6 | 否 | 6 | 6 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 索引 | 名称 | 载荷 | 说明 |
|------|------|------|------|
| 0 | ENABLE | bool | `true`：从步骤 0 开始；`false`：停止并复位 |
| 1 | ARRIVED | bool | 接 Nav.ARRIVED（到达时单次 true）。导航步收到 true 即推进 |

### 输出

| 索引 | 名称 | 载荷 | 说明 |
|------|------|------|------|
| 0 | GOAL | `[x,y,yaw]` | 当前航点位姿 → Nav.GOAL（yaw 预留） |
| 1 | ACTIVE | bool | 导航步为 true → Nav.ENABLE；等待/动作时 false |
| 2 | ACTION | map/JSON | **仅动作步**输出；导航/等待/完成时为空。见下 |
| 3 | STEP | int | 当前步骤索引 |
| 4 | FINISHED | bool | 全部完成 |
| 5 | STATUS | map | enable / running / step / type 等 |

### ACTION 载荷（动作步）

```json
{ "action": "open_gripper", "params": { "force": 0.5 } }
```

- `action`：面板「动作」名（必填才有输出）
- `params`：面板「参数」；可解析为 JSON 对象/数组，否则按标量/字符串写入；留空则省略该键

**不会**输出 `navigate` / `wait` / `finished` 等步骤类型字符串。

## 3. 界面与属性

控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）为**步骤列表编辑器**：每个步骤可设置类型（导航 / 等待 / 动作）、备注、目标 X/Y/航向、等待秒数、动作名、参数。无「到达距离」配置（在 Nav Controller 面板配置）。

本节点无可配置 Q_PROPERTY 属性；步骤列表随工程保存并恢复（旧工程 `type=emit` / 字段 `data` 仍可加载）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），不能通过 OSC/网页等方式远程控制；任务启停经 ENABLE 输入端口，步骤编辑只能在本节点界面操作。

## 5. 行为细节

- **默认演示步骤**：构造时预置「导航(1,0) → 等待 2s → 导航回原点」三步，首次拖入即可看到输出效果。
- **启停语义**：ENABLE 由 false→true（值变化才触发）从步骤 0 重新开始；true→false 立即停表并复位全部输出（GOAL=[0,0,0]、ACTIVE=false、ACTION 空、STEP=0、FINISHED=false）。步骤列表为空时使能直接进入完成态。
- **步骤推进**：导航步等待 ARRIVED 收到单次 `true` 才推进（false 不触发）；等待步用单次 QTimer（`seconds` 默认 1，负值取 0，毫秒四舍五入），期间 ENABLE=false 会中断；动作步输出 ACTION 后经 `QTimer::singleShot(0)` 立即推进下一步（下游当拍仍能取到 ACTION）。
- **ACTION 打包**：动作名为空时回落旧字段 `data`；`params` 文本尝试解析为 JSON 对象/数组，失败则按显示值转换（数字/布尔/字符串）；留空则省略 `params` 键。导航/等待步 ACTION 恒为空。
- **完成态**：步骤索引越过末尾 → FINISHED=true、ACTIVE=false、ACTION 清空；FINISHED 后需 ENABLE 重新拉起才会重跑。
- **步骤编辑热更新**：面板修改步骤立即生效；运行中若当前索引越界（删步骤）则直接完成，否则仅刷新输出、不打断当前步。
- **STATUS 载荷**：`enable/running/finished/step/step_count/type/active`，动作进行时附 `action`，步骤有效时附 `label/goal_x/goal_y/goal_yaw`，`default` = 当前步骤索引；面板同步显示状态文本与当前步高亮。
- **连接策略**：输入/输出端口均为 Many（可一对多）。
- **持久化**：`values.steps` 数组（type/x/y/yaw/seconds/action/params/label）；**加载仅认 `values` 子对象**（缺失时不回落顶层键）；加载后强制未使能并复位；旧 `type=emit` 映射为 `action`。

## 6. 使用说明与典型接法

```
AuroraS          感知：POSITION / ORIENTATION
    ↓（仅定位）
Nav Controller   控制：到点跟踪 → VX / STEER / ENABLE + ARRIVED
    ↑ GOAL/ACTIVE          ↓ ARRIVED
Nav Missions     任务：多点顺序、等待、到达后动作
    ↓ ACTION（仅动作步，JSON）
Ranger Mini      执行：CAN 底盘（接 Nav 的 VX/STEER/ENABLE）
```

### 推荐连线

```
AuroraS.POSITION      → Nav.POSITION
AuroraS.ORIENTATION   → Nav.ORIENTATION

Inject(true/false)    → NavMissions.ENABLE
NavMissions.GOAL      → Nav.GOAL
NavMissions.ACTIVE    → Nav.ENABLE
Nav.ARRIVED           → NavMissions.ARRIVED

Nav.VX / STEER / ENABLE → RangerMini

# 可选：到达后自定义动作
NavMissions.ACTION    → 下游执行器 / Distribute
```

### 步骤与推进

| type | 输出 | 下一步条件 |
|------|------|------------|
| `navigate` | GOAL + ACTIVE=true | Nav.ARRIVED == true |
| `wait` | ACTIVE=false | 定时结束 |
| `action` | ACTION = `{action, params}` | 立即推进（下游自行消费） |

`ENABLE=false` 立即停止；再次 `true` 从步骤 0 重跑。旧工程 `type=emit` / 字段 `data` 仍可加载。

