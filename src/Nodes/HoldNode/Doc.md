# Hold

## 1. 节点说明

Hold 在收到输入信号后，将数据**保持一段时间**再清空输出。用于延长脉冲、防止信号过短下游读不到，或在一段时间内锁定某状态。

支持两种重复触发策略：**忽略**（保持期间忽略新输入）或**重置**（新输入重新计时）。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 末端口为 HOLD_TIME（保持时长） |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | INPUT | VariableData | 触发保持的信号；内容会复制到输出 |
| 1 | HOLD_TIME | VariableData | 保持时长（毫秒）；可覆盖属性/外部设置的默认值 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | OUTPUT | VariableData | 保持期间为最近一次输入；到期后变为空 |

## 3. 界面与属性

节点无内嵌控件（`WidgetEmbeddable=false`），两个 Q_PROPERTY 显示在右侧属性面板：

| 属性 | 默认 | 说明 |
|------|------|------|
| holdTime | 1000 ms | 保持时间 |
| ignoreRepeat | false | true：保持期间忽略新 INPUT；false：新 INPUT 打断并重新计时 |

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/time` | 读/写 | 保持时间（毫秒，Command payload 转 int） |
| `/ignoreRepeat` | 读/写 | 重复触发策略（payload 转 bool） |

## 5. 行为细节

- **触发流程**：INPUT 到达（非空数据）→ 取当前 `m_holdTime`；≤ 0 时直接把输入赋给输出并刷新（不启动定时器）；> 0 时输出输入数据、置 `isHolding`、启动单发定时器。
- **重复输入**：保持期间新 INPUT 按 `ignoreRepeat` 处理——true 直接忽略；false 停掉当前定时器后按新数据重新保持。
- **到期**：定时器超时 → 输出替换为空 VariableData、`isHolding` 复位、emit dataUpdated（下游会收到一次「清空」刷新）。
- **HOLD_TIME 端口**：取 `asNumber()` 转整型并写入 `holdTime` 属性（等同改属性，影响后续触发）。
- **空数据**：`setInData` 收到 nullptr 直接忽略，不打断保持。
- **持久化**：`values.holdTime` / `values.ignoreRepeat`。

## 6. 使用说明与典型接法

1. 将短脉冲或事件接到 INPUT。
2. 设置保持时间（界面属性、`HOLD_TIME` 端口或 `/time`）。
3. 输出接下游；保持期间下游可读到数据，到期后输出清空并再次 `dataUpdated`。
4. 若保持时间为 0 或负数，输入会**立即**输出且不启动定时器。

**重复触发：**

- **忽略重复**（ignoreRepeat = true）：保持计时未结束时，新 INPUT 无效。
- **不忽略**（false）：新 INPUT 停止当前计时，立即按新数据重新保持。

配置保存在 `values.holdTime`、`values.ignoreRepeat`。

## 7. 示例

**按钮去抖延长：** 短按 INPUT 接 Hold，holdTime 500 ms，输出接 Switch，避免机械抖动导致多次切换。  
**HOLD_TIME 动态化：** 将控台推杆映射到 HOLD_TIME 端口，实现可变长度的「门保持」。

