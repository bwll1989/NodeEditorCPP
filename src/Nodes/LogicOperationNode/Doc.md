# Logic Operation 节点

## 1. 节点说明

Logic Operation 插件提供一组逻辑/比较节点，对 VariableData 输入做运算后，从单一输出端口给出**布尔**结果。无内嵌界面。

| 变体名称 | 运算说明 | 输入端口 |
|----------|----------|----------|
| Logic And | 布尔与（`toBool`） | 2 |
| Logic Or | 布尔或 | 2 |
| Logic Xor | 布尔异或 | 2 |
| Logic Nand | 布尔与非 | 2 |
| Logic Nor | 布尔或非 | 2 |
| Logic Not | 布尔取反 | 1 |
| Logic Equal | 字符串相等 | 2 |
| Logic NotEqual | 字符串不等 | 2 |
| Logic EqualNum | 数值近似相等（`qFuzzyCompare`） | 2 |
| Logic IsEmpty | 输入为空/无效 | 1 |
| Logic Less | float 比较：输入 0 < 输入 1 | 2 |
| Logic LessEqual | float 比较：输入 0 ≤ 输入 1 | 2 |
| Logic Greater | float 比较：输入 0 > 输入 1 | 2 |
| Logic GreaterEqual | float 比较：输入 0 ≥ 输入 1 | 2 |

已移除 Logic Max / Logic Min（请使用 Math Operation 插件中的 Math Max / Math Min）。

任一输入更新后，输出会立即刷新。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0…n | INPUT / INPUT n | VariableData | 操作数；一元运算仅 1 个端口 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | OUTPUT 0 | VariableData | 布尔运算结果 |

## 3. 界面说明

各变体无内嵌面板，无额外控件。

## 4. 使用说明

1. 从节点库选择需要的变体（如 Logic Greater）。
2. 将 VariableData 接到输入端口。
3. 将输出接到 Condition、Switch、Inject 等下游节点。

**注意：** 旧版 Logic And 实际做的是字符串相等比较；现已改为真正的布尔与。若需字符串相等，请使用 **Logic Equal**。

## 5. 示例

**阈值判断：** Logic Greater 比较传感器数值与阈值，输出接 Switch 的 INDEX。  
**开关组合：** Logic And / Or / Xor 组合多路布尔信号。  
**空值检测：** Logic IsEmpty 判断字符串或无效输入。
