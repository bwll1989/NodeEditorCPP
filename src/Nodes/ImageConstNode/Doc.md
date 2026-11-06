本插件包含三种节点：**Image Constant**（纯色图）、**Text To Image**（文字图）、**Image Layout**（多图拼接）。三者均为静态源：参数脏时在全局帧时钟节拍上重新生成并推帧，不随每帧重绘。

---

# Image Constant

## 1. 节点说明

生成指定尺寸、指定颜色的纯色图像（GPU 直绘纹理），适合占位、蒙版或测试画面。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | WIDTH | VariableData | 宽度（像素，asInt） |
| 1 | HEIGHT | VariableData | 高度（asInt） |
| 2 | RGBA | VariableData | 颜色，`[r, g, b, a]` 浮点 0～1（3 元则 a=1，超出截断；旧工程 `#hex` 字符串仍可加载） |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | Image | ImageData | 生成的纯色图像（GPU 纹理，环形缓冲） |

## 3. 界面与属性

控件位于右侧属性面板（`WidgetEmbeddable=false`，不嵌入节点体）：

- 宽度、高度数值框（默认 100×100，≤0 按 1 处理）。
- 颜色预览区与「颜色编辑器」按钮（弹出独立置顶颜色窗口，800×400）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），宽高与颜色只能通过界面或输入端口设置。

## 5. 行为细节

- **输入语义**：WIDTH/HEIGHT 取 `asInt()` 并同步回界面数值框；RGBA 经 `rgbaVectorFromVariant` 解析后应用到颜色编辑器。
- **发布机制**：订阅全局 TimestampGenerator 帧时钟（QueuedConnection）；仅当「参数脏」时才重新生成 GPU 纯色纹理并推入输出环形缓冲（时间戳去重），否则节拍空转。
- **生效顺序**：端口写入 → 界面同步 → 参数标脏 → 下一个帧节拍发布。
- **持久化**：`values.width` / `values.height` / `values.rgba`（浮点数组；旧键 `color` 兼容读取）。

## 6. 使用说明与典型接法

在界面中设置宽高和颜色，或通过输入端口动态覆盖；修改后自动重新生成图像。Variable 数据接 WIDTH/HEIGHT/RGBA 可做运行时变色底板。

## 7. 示例

Image Constant（1920×1080 黑色）→ 作为 Image Layout 的背景层；OSC 不可控，动态变色需经 RGBA 端口。

---

# Text To Image

## 1. 节点说明

将文字渲染为图像（QPainter + QTextDocument 排版），支持字体、字号、样式、颜色、对齐、边距与换行。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 9 | 否 | 9 | 9 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入（均为 VariableData，可选）

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | WIDTH | 画布宽（asInt，默认 512） |
| 1 | HEIGHT | 画布高（asInt，默认 256） |
| 2 | TEXT | 文字内容（asString，`\n` 转 `<br/>`） |
| 3 | FONT | 字体族名（asString） |
| 4 | FONTSIZE | 字号（asInt） |
| 5 | TEXT_RGBA | 文字色 `[r,g,b,a]` 浮点 0～1（3 元则 a=1） |
| 6 | BG_RGBA | 背景色（同上） |
| 7 | HALIGN | 水平对齐 0 左 / 1 中 / 2 右（越界钳制） |
| 8 | VALIGN | 垂直对齐 0 上 / 1 中 / 2 下（越界钳制） |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | Image | ImageData | 渲染结果（ARGB32 → BGRA 帧） |

## 3. 界面与属性

控件位于右侧属性面板（`WidgetEmbeddable=false`，不嵌入节点体）：

- 文本框、字体选择（QFontComboBox）、字号、粗体/斜体/下划线、水平/垂直对齐、边距、换行、抗锯齿开关。
- 文字色/背景色按钮（各自弹出独立颜色编辑器，支持 Alpha）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），文字与样式只能通过界面或输入端口设置。

## 5. 行为细节

- **输入语义**：各端口写入对应界面控件或内部参数；任一输入到达即触发重渲染（`renderImage`，主线程同步执行）。
- **渲染流程**：按参数拼 HTML（rgba() 颜色、font-weight/style/decoration、text-align、white-space pre/pre-wrap）→ QTextDocument 排版 → QPainter 绘制到画布（先填背景色，内容区裁剪 + 垂直偏移）；预览图同步缩放显示。
- **发布机制**：与 Image Constant 相同——订阅帧时钟，参数脏时把渲染好的 BGRA 帧推入输出环形缓冲。
- **持久化**：`values` 内 `width/height/text/font/fontSize/textRgba/bgRgba/hAlign/vAlign/padding/bold/italic/underline/wrap/antialia`（旧键 `textColor`/`bgColor` 兼容读取）。

## 6. 使用说明与典型接法

在节点内编辑文字样式，或用端口传入动态文案与尺寸；String 变量接 TEXT 即可做动态字幕/计分板。

## 7. 示例

String 变量 → TEXT 端口 → Text To Image → 叠加到视频画面上。

---

# Image Layout

## 1. 节点说明

将多路输入图像按指定布局拼成一张图（默认 4 路输入，端口可增删）。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 是 | 1 | 不限 | ImageData | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0～N-1 | Image1～ImageN | ImageData | 待拼接的图像；断开即从布局中移除该路 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | Image | ImageData | 拼接后的图像 |

## 3. 界面与属性

控件位于右侧属性面板（`WidgetEmbeddable=false`，不嵌入节点体）：

- 画布宽高（≤0 不更新内部值）、布局方式、行列数、间距、背景色及颜色编辑器按钮。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），布局参数只能通过界面设置。

## 5. 行为细节

- **布局计算**（computeGrid）：模式 0 = 单行 N 列；模式 1 = 单列 N 行；模式 2 = 自定义行列（自动扩行容纳输入数）。
- **输入管理**：`setInData` 收到 ImageData 存入对应槽位，非 ImageData/断开则清除该槽位并标脏；任一输入变化重置「已见时间戳」。
- **发布机制**：订阅帧时钟，参数或输入脏时重新合成并推帧（背景色填充 + 缩放贴块 + 间距）。
- **持久化**：`values.width/height/layout/rows/cols/spacing/color`（HexArgb 字符串；rows/cols 默认 2×2）。

## 6. 使用说明与典型接法

1. 按需增加输入端口并连接各图像源。
2. 选择布局（单行/单列/网格）并设置行列、间距。
3. 从 Image 输出口取合成结果。

## 7. 示例

四路摄像头 → Image Layout（2×2 网格）→ Spout Out。
