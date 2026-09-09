# Nav Controller
## 节点说明
与 **Ranger Mini** 打在同一个插件 `RangerMiniNode.node` 内（源码在 `RangerMiniNode/NavControllerNode/`）。

面板分类：**Devices → Nav Controller**

将 **Aurora S**（或同约定）的平面位姿转为 **Ranger Mini** 阿克曼指令：`VX`（m/s）、`STEER`（rad）、`ENABLE`（bool）。

控制默认：**Reeds–Shepp 最短路径（含倒车圆弧）+ Pure Pursuit 跟踪**。GOAL 为三维位姿 `[x,y,yaw]`，到达需位置与航向同时满足。

多点任务可配合 **Controls → Nav Missions**（见 `MissionNode/Doc.md`）：

```
Aurora.POSITION / ORIENTATION → Nav（定位只进导航）
Inject → Mission.ENABLE
Mission.GOAL / ACTIVE → Nav.GOAL / ENABLE
Nav.ARRIVED → Mission.ARRIVED
Nav.VX / STEER / ENABLE → Ranger Mini
```

Aurora 与底盘同装时，POSITION 可视为车位姿；Mission 负责任务航点与顺序，Nav 负责规划、跟踪与到点判定。

**节点间单位（SI）**：位置 **m**，姿态 **rad**，线速度 **m/s**，角速度 **rad/s**。不要用 mm / mm/s / 度接到本节点。

## 推荐连线

```
AuroraS.POSITION      → NavController.POSITION      # [x,y,z] m
AuroraS.ORIENTATION   → NavController.ORIENTATION   # map.rad = [roll,pitch,yaw]
AuroraS.LOCALIZATION  → NavController.LOCALIZATION  # bool，仅写入 STATUS，不门控
Mission.GOAL          → NavController.GOAL          # [gx,gy,yaw] m/rad
Mission.ACTIVE        → NavController.ENABLE        # 或 Inject(true)

NavController.VX      → RangerMini.VX
NavController.STEER   → RangerMini.STEER
NavController.ENABLE  → RangerMini.ENABLE
NavController.ARRIVED → Mission.ARRIVED             # 多点任务时
# Ranger SPIN 不要接 Nav 输出
```

Ranger Mini 请选 **Ackermann**（前后对向转）。  
ORIENTATION **不必**再经 Extract；本节点会读 `rad` 并取 yaw。若一定要用 Extract，写 `$input.rad[2]`（下标 0/1/2 = roll/pitch/yaw），不要写 `[3]`。

---

## 输入端口

| 索引 | 名称 | 载荷 | 解析方式 | 说明 |
|------|------|------|----------|------|
| 0 | POSITION | float 列表 | `asFloats(2)` | 当前平面位置。取前两维为 `x,y`（m）。多接 `z` 会被截断忽略。未收到过有效数据则不跟踪。 |
| 1 | ORIENTATION | map 或列表 | 优先 `asFloats(0,"rad")`，否则 `asFloats(0)` | 期望 Aurora 的 map：`rad`/`deg`/`default` 均为 `[roll,pitch,yaw]`。控制只用 **rad**；长度 ≥3 时取 **下标 2 = yaw**；若只有 1 个数则当作 yaw 标量。 |
| 2 | LOCALIZATION | bool | `asBool()` | 定位标志，**只写入 STATUS.localized**，不参与是否跟踪的门控。可不接。 |
| 3 | GOAL | `[gx,gy,yaw]` | `asFloats` | 目标位姿：x/y 为 m，yaw 为 rad。**参与 RS 规划与到达判定**。不足 3 维时保留面板原 yaw。属性/OSC `/goal`。 |
| 4 | ENABLE | bool | `asBool()` | 导航总使能。false 时不跟踪。也可 OSC `/enable` 或属性 `enable` 写入。 |

激活条件（同时满足才进入跟踪）：

1. `ENABLE == true`
2. 已收到 POSITION 与 ORIENTATION

---

## 输出端口

| 索引 | 名称 | 载荷 | 说明 |
|------|------|------|------|
| 0 | VX | 标量 m/s | 前进为正、倒车为负。由路径段档位 × Max Vx × 近距/曲率缩放得到；到达或未激活时为 0。 |
| 1 | STEER | 标量 rad | Pure Pursuit 阿克曼转角（左正），限幅到 ±Max Steer。 |
| 2 | ENABLE | bool | **跟踪中**为 true（已激活且未到达）。到达或总使能为 false 时为 false。应接到 Ranger 的 ENABLE，**不要**接到 SPIN。 |
| 3 | STATUS | map | 调试状态（见下表）。`default` 键为当前距离（m），便于监视器显示。 |
| 4 | ARRIVED | bool | **Trigger**：进入到点区时只发一次 `true`，随后为 `false`。接到 Mission.ARRIVED 推进航点。 |

### STATUS 字段

| 键 | 类型 | 说明 |
|----|------|------|
| `active` | bool | 是否满足跟踪前置条件 |
| `arrived` | bool | 当前是否在到点区内（电平，给 STATUS） |
| `arrived_pulse` | bool | 本帧是否发出 ARRIVED trigger |
| `tracking` | bool | 同输出 ENABLE |
| `reversing` | bool | 当前是否倒车（VX 为负） |
| `mode` | string | `空闲` / `RS前进跟踪` / `RS倒车跟踪` / `已到达` / `规划失败` / `横向偏差·重规划` / `航向偏差·重规划` / `位姿偏差·重规划` |
| `plan_ok` | bool | 最近一次 RS 是否成功 |
| `path_length_m` | double | RS 路径总弧长 |
| `path_types` | string | 路径段类型串，如 `L+S+R-` |
| `sample_count` | int | 路径采样点数 |
| `cross_track_m` | double | 到路径最近点的距离 |
| `path_align_yaw_rad` | double | 相对路径最近点的航向误差（重规划用） |
| `r_min_m` | double | 规划用最小转弯半径 |
| `l_eff_m` | double | 有效轴距 = L/2 |
| `localized` | bool | 最近一次 LOCALIZATION 输入 |
| `distance_m` | double | 到目标水平距离 |
| `yaw_error_rad` | double | 跟踪时为预瞄点航向误差；到点判定用目标航向误差 |
| `vx_m_s` / `steer_rad` | double | 当前输出 |
| `pos_x` / `pos_y` | double | 当前位姿 |
| `goal_x` / `goal_y` / `goal_yaw` | double | 当前目标 |
| `default` | double | 同 `distance_m` |

面板底部状态行示例：`d=… yawErr=… vx=… RS前进跟踪|RS倒车跟踪|已到达|空闲`。

---

## 面板参数

| 控件 | 默认 | 范围 | 单位 | 作用 |
|------|------|------|------|------|
| **Goal X / Y / 航向** | 0 | — | m / m / rad | 目标位姿；属性与 OSC 为 `goal`=`[x,y,yaw]`。yaw **参与 RS 与到达**。 |
| **航向容差** | 0.15 | 0.02～1 | rad | 到点还需 `|yaw_err| ≤ 此值`。 |
| **预瞄距离** | 0.45 | 0.15～3 | m | Pure Pursuit 沿路径向前取点的弧长。偏小更跟线、偏大更稳。 |
| **Max Vx** | 0.3 | 0～2.0 | m/s | 最大线速度上限。初次实车建议 ≤0.3。**不要**填 2000（旧 mm/s）。OSC `/maxVx`。 |
| **Max Steer** | 0.5 | 0.05～1.571 | rad | 转角限幅，并决定 `R_min = L_eff/tan(δ_max)`。Ranger 阿克曼约 ±0.698（40°）。OSC `/maxSteer`。 |
| **Arrive Dist** | 0.15 | 0.02～5 | m | 进入到点半径。OSC `/arriveDist`。 |
| **到点滞回** | 0.05 | 0～1 | m | 离开需 `距离 > Arrive+滞回`（航向也会用 1.5×容差）才退出到点。 |
| **Slow Dist** | 0.8 | 0.05～10 | m | 近目标线性降速距离。 |
| **Wheelbase L** | 0.494 | 0.05～2 | m | **物理轴距**（Ranger Mini 3.0 = 494 mm）。控制/规划内自动用 `L_eff = L/2`。 |
| **Kp YawRate** | 1.2 | 0.1～5 | 1/s | 旧 LOS 增益，保留存盘兼容；**当前 RS+PP 控制律未使用**。 |
| **镜像坐标系** | 默认开 | — | — | `(y,yaw)→(-y,-yaw)`：Aurora +Y右 ↔ Ranger +Y左 / STEER 左正。整车左右反了则关掉。 |

另：内部属性 **enable**（OSC `/enable`）与输入口 ENABLE 等价，无单独面板开关。  
已移除「允许倒车」：RS 路径段自带前进/倒车，跟踪时按采样点 `gear` 输出正/负 VX。

---

## 控制律（SI）

### 1) Reeds–Shepp 规划

换目标、改 MaxSteer/轴距/镜像、重新使能，或相对**路径最近点**出现过大偏差时重规划：

- 横向：`cross_track > max(0.6 m, 3×ArriveDist)`
- 航向：`|path_align_yaw| > π/2`（约 90°；相对最近点 yaw，不是目标 yaw）

两者任一满足，且距上次规划 ≥ **1 s**（防抖）。  
**不做定时重规划**（避免 SLAM 静止抖动导致路径不停刷新）。

```
L_eff = Wheelbase / 2
R_min = L_eff / tan(MaxSteer)
path  = RS(start_pose, goal_pose, R_min)   # 含 CSC/CCC/… 与倒车段
```

实现见 `RsPath.hpp`。无障碍假设下给出有界曲率最短路径。

### 2) Pure Pursuit 跟踪

```
nearest = 路径上距车最近的采样点
gear    = nearest.gear          # +1 前进 / -1 倒车
target  = 沿路径向前累加弧长 ≈ LookAhead 的点
α       = atan2(localY, localX) # 预瞄点在车体系
STEER   = clamp( atan2(2·L_eff·sin(α), Ld), ±MaxSteer )
VX      = gear × MaxVx × speedScale × turnScale
```

近目标 `speedScale = clamp(dist_to_goal / SlowDist, 0.15, 1)`；`|α|` 大时再降 `turnScale`。

### 3) 到达判定

```
pos_ok = distance ≤ ArriveDist
yaw_ok = |wrap(goal_yaw - yaw)| ≤ YawTol
进入到点：pos_ok && yaw_ok
离开：distance > Arrive+滞回 或 |yaw_err| > 1.5·YawTol
```

ARRIVED 口对本目标只脉冲一次 `true`。

前后对向（`δ_f ≈ δ`，`δ_r ≈ -δ`）时：

\[
\dot\theta \approx \frac{2 v \tan\delta}{L}
\quad\Rightarrow\quad
L_{\mathrm{eff}} = L/2
\]

仅前轮转向的车才应使用 `L_eff = L`（本节点按 Ranger Mini 固定用 L/2）。

---

## 路径预览

面板按钮 **「路径预览…」** 打开独立工具窗（非模态）：

| 图例 | 含义 |
|------|------|
| 蓝实线 | 前进段 |
| 橙虚线 | 倒车段 |
| 红点+红箭头 | 当前位姿与航向 |
| 绿方块+绿箭头 | 目标位姿与航向 |

弹窗为**独立顶层窗口**（无父 `QDialog` + `Qt::Window`），不会嵌进节点图画布；非模态，可拖到屏幕任意位置。

---

## 无真机验证

目录下 `rs_offline_verify.cpp`：用同一套 `RsPath` + Pure Pursuit + 自行车模型积分，不连 Aurora/Ranger。

```bat
cd src\Nodes\RangerMiniNode\NavControllerNode
g++ -std=c++17 -O2 -o rs_offline_verify.exe rs_offline_verify.cpp
rs_offline_verify.exe
rs_offline_verify.exe --csv traj.csv
```

全部 `[PASS]` 说明规划与闭环在理想模型下闭环到点；FAIL 优先查算法/参数。通过 ≠ 实车一定稳（无滑移、无定位噪声）。

---

## 使用步骤

1. Aurora / Ranger 已连接；Ranger 模式 Ackermann。
2. 接好上表推荐连线；Goal 用 Mission / 面板给出 `[x,y,yaw]`。
3. Max Vx 先 ≤0.3，Wheelbase 保持 0.494；预瞄约 0.45 m。
4. Inject `true` → ENABLE；观察 STATUS.`mode` / `path_types` 与 Ranger 运动（可能含倒车圆弧）。
5. 位置+航向都到位后自动停车；`ARRIVED=true` 可推进 Mission。若整车左右反了：关掉 **镜像坐标系**。
6. 跟线抖/切弯慢：调 **预瞄距离**；到点航向松/紧：调 **航向容差**；打满转角仍规划不出：略增 Max Steer（勿超底盘 ±0.698）。

## 坐标系与偏航符号

| 对象 | X | Y | 左转 / 正转角 |
|------|---|---|---------------|
| Aurora 设备/地图 | 前 | **右** | 朝 +Y 转为正 yaw（与 `atan2(y,x)` 同侧） |
| Ranger 车体约定 | 前 | **左** | STEER / 自旋 **左转为正** |
| 本节点控制律（默认镜像） | 前 | 左 | 在控制系内规划与跟踪 |

Aurora 与车**朝向安装一致**时仍建议开 **镜像坐标系**（`(y,ψ)→(-y,-ψ)`）。  
单位上 yaw 已是 rad；CAN 侧 `×1000→0.001 rad` 由 Ranger 节点完成。

## 常见错误

| 现象 | 可能原因 |
|------|----------|
| 一直空闲 | ENABLE 未接 true；POSITION/ORIENTATION 未接到或接错口 |
| 规划失败 | Max Steer 过小导致 R_min 极大；目标/位姿异常 |
| 不走或乱走 | POSITION 接到了 LOCALIZATION 口等错线；镜像开关与安装不符 |
| yaw 恒为 0 | Extract 写成 `$input.rad[3]`（越界），应直接连 ORIENTATION 或用 `rad[2]` |
| 到点不触发 | 只到位置未到航向：检查 GOAL.yaw 与「航向容差」 |
| 速度离谱 | Max Vx 误存成 2000（旧 mm/s），应改为 m/s |
| STEER 接到 SPIN | 口序：Nav 出 0/1/2 = VX/STEER/**ENABLE**，不是 SPIN |
