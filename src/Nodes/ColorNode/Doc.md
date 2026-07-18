# Color

## 1. 节点说明

交互式颜色源：在节点内嵌色板中选取颜色，或通过 R/G/B/A 输入、外部 OSC 覆盖各分量。输出完整 **QColor** 以及整数通道，便于驱动 Image Constant、协议参数或其它需要 0～255 颜色的节点。

默认颜色为不透明黑色 `(0, 0, 0, 255)`。颜色以 `#AARRGGBB` 形式写入工程。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| R | VariableData | 红色 0～255 |
| G | VariableData | 绿色 0～255 |
| B | VariableData | 蓝色 0～255 |
| A | VariableData | 透明度 0～255 |

任一输入更新后会同步到色板，并刷新全部输出。

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| COLOR | VariableData | 当前 `QColor` |
| RED | VariableData | 红色整数 |
| GREEN | VariableData | 绿色整数 |
| BLUE | VariableData | 蓝色整数 |
| ALPHA | VariableData | 透明度整数 |

## 3. 界面说明

内嵌 **ColorEditorWidget**：

- RGB / HSV 数值框与滑块
- 色板 / 预览
- 与属性、外部控制双向同步

外部控制（读写，变更会反馈状态）：

| 地址 | 说明 |
|------|------|
| `/red` `/green` `/blue` `/alpha` | RGBA 整数 |
| `/hue` `/saturation` `/value` | HSV 整数 |

## 4. 使用说明

1. 在色板中调色，或用 R/G/B/A 端口、OSC 写入分量。
2. 需要完整颜色时接 **COLOR**；需要单通道时接 RED/GREEN/BLUE/ALPHA。
3. 工程保存/加载会恢复色板颜色。

## 5. 示例

Color → RED/GREEN/BLUE → Image Constant 的颜色输入，生成纯色背景。

Color → COLOR → To HSV → 按色相做条件分支。

---

# Color Float

## 1. 节点说明

与 **Color** 相同的选色界面与外部控制，但：

- **输入** R/G/B/A 按**浮点 0～1**（`setRedF` 等）解析；
- **输出** RED/GREEN/BLUE/ALPHA 为浮点（`redF` / `greenF` / `blueF` / `alphaF`）；
- **COLOR** 仍为 `QColor`。

适合 Shader、归一化协议或脚本中需要 0～1 颜色通道的场景。默认仍为不透明黑色；工程同样以 `#AARRGGBB` 持久化。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| R | VariableData | 红色 0～1（浮点） |
| G | VariableData | 绿色 0～1 |
| B | VariableData | 蓝色 0～1 |
| A | VariableData | 透明度 0～1 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| COLOR | VariableData | 当前 `QColor` |
| RED | VariableData | 红色浮点 |
| GREEN | VariableData | 绿色浮点 |
| BLUE | VariableData | 蓝色浮点 |
| ALPHA | VariableData | 透明度浮点 |

## 3. 界面说明

与 Color 相同的内嵌色板。外部控制地址亦相同：`/red`、`/green`、`/blue`、`/alpha`、`/hue`、`/saturation`、`/value`（OSC 侧按整数分量写入属性，再反映到色板）。

## 4. 使用说明

1. 用色板调色，或向 R/G/B/A 送入 0～1 浮点。
2. 下游需要归一化通道时接 RED～ALPHA；需要 QColor 时接 COLOR。
3. 勿与 Color 的 0～255 整数混接，除非中间做缩放。

## 5. 示例

Color Float → RED/GREEN/BLUE/ALPHA → 自定义脚本或 QML，作为 shader uniform。

---

# To RGBA

## 1. 节点说明

将输入颜色拆成**整数** RGBA 四路输出。无内嵌界面，纯转换节点。

输入可为 `QColor`，或可被 `QColor(字符串)` 解析的颜色字符串（如 `#RRGGBB` / `#AARRGGBB`）。非法颜色则忽略。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| COLOR | VariableData | `QColor` 或颜色字符串 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| R | VariableData | 红色 0～255 |
| G | VariableData | 绿色 0～255 |
| B | VariableData | 蓝色 0～255 |
| A | VariableData | 透明度 0～255 |

## 3. 界面说明

无嵌入控件。

外部控制：`/color`（写入颜色，效果同 COLOR 输入）。

## 4. 使用说明

1. 将 Color / Color Float 的 COLOR，或其它颜色源接到 COLOR。
2. 从 R/G/B/A 取整数分量接下游。

## 5. 示例

Color → COLOR → To RGBA → 分别驱动四路参数或协议字段。

---

# To HSV

## 1. 节点说明

将输入颜色拆成 **HSV** 三路整数输出（Qt：`hsvHue` / `hsvSaturation` / `value`）。无内嵌界面。

输入规则与 To RGBA 相同：`QColor` 或合法颜色字符串。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| COLOR | VariableData | `QColor` 或颜色字符串 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| H | VariableData | 色相（Qt HSV hue） |
| S | VariableData | 饱和度 |
| V | VariableData | 明度 |

## 3. 界面说明

无嵌入控件。外部控制：`/color`。

## 4. 使用说明

1. 接入颜色源。
2. 用 H/S/V 做阈值、条件或映射（如按色相切换场景）。

## 5. 示例

Color → COLOR → To HSV → Condition（`$input.default` 对 H 比较）→ 触发不同场景。

---

# To Float RGBA

## 1. 节点说明

将输入颜色拆成**浮点** RGBA（0～1）四路输出。无内嵌界面。输入规则与 To RGBA 相同。

与 **Color Float** 的通道输出口径一致，适合已有 QColor、只需归一化分量的链路。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| COLOR | VariableData | `QColor` 或颜色字符串 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| R | VariableData | 红色浮点 0～1 |
| G | VariableData | 绿色浮点 0～1 |
| B | VariableData | 蓝色浮点 0～1 |
| A | VariableData | 透明度浮点 0～1 |

## 3. 界面说明

无嵌入控件。外部控制：`/color`。

## 4. 使用说明

1. 将 Color 的 COLOR 或其它颜色源接入。
2. 从 R/G/B/A 取浮点分量接脚本、Shader 或外部协议。

## 5. 示例

Color → COLOR → To Float RGBA → 自定义脚本打包为 `{r,g,b,a}` 发出。
