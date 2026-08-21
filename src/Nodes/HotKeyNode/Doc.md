# Keyboard In

## 1. 节点说明

Keyboard In 以列表监视应用程序内按键状态（按下 = `1`，松开 = `0`），行为对齐 [TouchDesigner Keyboard In CHOP](https://docs.derivative.ca/index.php?title=Keyboard_In_CHOP)。

列表行数与输出端口数量**相互独立**，均由用户决定（用法同 Delay：列表里填 Index，节点上编辑端口数）。

本节点不会注册系统全局热键，不会抢走按键。

## 2. 端口说明

### 输入

无。

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 … N | （无端口名） | VariableData | 映射到该 Index 的按键状态 `0/1` |

输出端口数量通过节点端口编辑自行增减。

## 3. 界面说明

- 列表（右键：Add / Delete / Clear）：每行 `Index`（输出端口）+ `Key`（按键）+ 状态指示
- **Active**：总开关
- **Modifiers**：修饰键过滤

## 4. 使用说明

1. 在节点上设置需要的输出端口数。
2. 右键列表添加按键行，填写 Index 与 Key（Index 应对应已有输出端口）。
3. 边沿触发可接 Edge Trigger；按住电平直接使用 `0/1`。

# Mouse In

## 1. 节点说明

Mouse In 监视鼠标位置与左/右键。

与 Keyboard In 同属 Controls 插件；不拦截鼠标事件，其它界面可正常操作。

## 2. 端口说明

### 输入

无。

### 输出

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | tx | X 坐标（像素或归一化） |
| 1 | ty | Y 坐标（像素或归一化） |
| 2 | left | 左键 0/1 |
| 3 | right | 右键 0/1 |
| 4 | pos | VariableData，`default` 为 `[tx, ty]` |

## 3. 界面说明

- **Active**：总开关
- **Coord**：`Absolute`（屏幕像素）/ `Normalized`（当前屏幕 0–1）
- 通道预览（FloatDrag，只读，与端口同名）：`tx` / `ty` / `left` / `right`

## 4. 使用说明

1. 勾选 Active，选择坐标模式。
2. 将 `tx`/`ty` 接到下游做跟随；按键通道做点击触发。

**注意：** 位置与按键通过轮询全局状态。
