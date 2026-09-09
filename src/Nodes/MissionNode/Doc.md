# Nav Missions

面板分类：**Controls → Nav Missions**

航点任务层：根据 **Nav 的 ARRIVED** 推进步骤，向 **Nav Controller** 下发目标点；定位只进 Nav，本节点不吃 POSITION。

## 分层

```
AuroraS          感知：POSITION / ORIENTATION
    ↓（仅定位）
Nav Controller   控制：到点跟踪 → VX / STEER / ENABLE + ARRIVED
    ↑ GOAL/ACTIVE          ↓ ARRIVED
Nav Missions     任务：多点顺序、等待、到达后动作
    ↓ ACTION（仅动作步，JSON）
Ranger Mini      执行：CAN 底盘（接 Nav 的 VX/STEER/ENABLE）
```

## 推荐连线

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

## 端口

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

## 步骤与推进

| type | 输出 | 下一步条件 |
|------|------|------------|
| `navigate` | GOAL + ACTIVE=true | Nav.ARRIVED == true |
| `wait` | ACTIVE=false | 定时结束 |
| `action` | ACTION = `{action, params}` | 立即推进（下游自行消费） |

`ENABLE=false` 立即停止；再次 `true` 从步骤 0 重跑。旧工程 `type=emit` / 字段 `data` 仍可加载。

## 面板

步骤列表 + 类型（导航 / 等待 / 动作）、备注、目标 X/Y/航向、等待秒数、动作名、参数。无「到达距离」（在 Nav 面板配置）。
