# Color 插件

本插件提供颜色选取与格式转换节点：**Color**、**Color Float**、**To RGBA**、**To HSV**、**To Float RGBA**。

---

## Color

### 1. 节点说明

交互式选取颜色，并输出完整颜色或各通道分量。

### 2. 端口说明

#### 输入

- **R** / **G** / **B** / **A**（VariableData）：各通道 0～255。

#### 输出

- **COLOR**（VariableData）：QColor。
- **RED** / **GREEN** / **BLUE** / **ALPHA**：各通道整数。

### 3. 界面说明

内嵌颜色编辑器（RGB/HSV 滑块与色板）。

### 4. 使用说明

在编辑器中调色，或用 R/G/B/A 端口覆盖；外部地址 `/red`、`/green`、`/blue`、`/alpha`、`/hue`、`/saturation`、`/value`。

### 5. 示例

Color → RED/GREEN/BLUE → Image Constant 的 RED 端口，驱动纯色背景。

---

## Color Float、To RGBA、To HSV、To Float RGBA

### 1. 节点说明

在 QColor、浮点 RGBA、HSV 等表示之间转换，或输出浮点通道，供 Shader、协议等使用。

### 2. 端口说明

各子节点输入/输出均为 **VariableData**；具体端口名见节点上的标注（与 Color 类似，为颜色分量或组合值）。

### 3. 界面说明

**Color Float** 带颜色编辑器；转换类节点多为透传/计算，界面较简。

### 4. 使用说明

将上游颜色接到输入，从对应格式输出口取数。

### 5. 示例

Color → To HSV → Extract，按色相做场景切换。
