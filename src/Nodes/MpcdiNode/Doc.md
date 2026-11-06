# MPCDI

## 1. 节点说明

用 Scalable Display 的 MPCDI 参考实现读取 `.mpcdi`，对输入图像做几何变形（geometry warp）和边缘融合（alpha map）。按所选 buffer 里的 region 数自动展开多路输出，一路对应一台投影机。

依赖：`3rdParty/Scalable Display Reference Implementation`（静态编进插件，不另带 DLL）。该库实现的是 MPCDI 1.0；更高版本会关掉版本检查后尽量读。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 否 | 1 | 1 | — | 无 |
| 输出 | 1 | 否 | 1 | 随文件（region 数） | IMAGE | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。
>
> 输出口数不能手动增删：初始为 1，选定 `.mpcdi`（或换文件 / 换 buffer）后按该 buffer 的 region 数自动增删端口；文件为空或加载失败时回落 1 路。

### 输入

- **IMAGE**：待变形的源图像，对应 MPCDI buffer 的整幅画面。

### 输出

- **IMAGE** / 各 region id：N = 所选 buffer 的 region 数（源码无上限，完全由文件决定）。仅 1 个 region 时端口名为 IMAGE，多个 region 时端口名依次为各 region 的 id（见 `portCaption`）。

## 3. 界面与属性

内嵌控件面板（`WidgetEmbeddable=true`，可嵌入节点体显示，也可收入右侧属性面板）。文件选择：媒体库 MPCDI 分类中的 `.mpcdi`。换文件会重新读校准并按 region 数重算输出口。文件里有多个 buffer 时使用已保存的 buffer id，否则用第一个。

| 控件 | 默认 | 作用 |
|------|------|------|
| 坐标 | 自动 | 解释 warp 图 R/G 的单位。见下表。 |
| 翻转 Y（内容） | 开 | 采样源图前把内容 V 反过来。画面上下颠倒时切换。 |
| 翻转 Y（变形图） | 关 | 读 warp 图时把查找行上下对调。变形形状本身镜像时打开。alpha / beta 仍按屏幕坐标采样。 |
| Region 局部 | 关 | 把整幅 buffer 的 UV 收进当前 region 的矩形，让这一路铺满输出。多 region 共用一幅内容、且 warp 存的是整幅坐标时使用。 |
| Blend | 开 | 用 alpha 图做边缘融合，权重乘到 RGB 和 A。关闭后只做几何变形。 |
| Alpha 线性化 | 开 | 按该 region 的 `gammaEmbedded` 把 alpha 还原成线性权重后再相乘。关掉则按 1.0，不再做 gamma。 |
| 黑电平 | 开 | 文件里有 beta 图时，在线性空间按 `颜色 = alpha×颜色×(1−beta)+beta` 抬黑。没有 beta 图时无效果。 |
| 区域边界 | 关 | 调试用。输出边缘画绿线，warp 未映射的像素涂成琥珀色。 |

坐标三种模式：

| 模式 | 作用 |
|------|------|
| 自动 | 超过一半的有效采样 `max(\|R\|,\|G\|) > 2.5` 时当作像素，先除以 buffer 分辨率；否则当作 0–1 的归一化 UV。 |
| 归一化 [0,1] | R/G 直接当 UV，不再除分辨率。 |
| 像素 | R/G 一律先除以 buffer 分辨率。 |

颜色的 decode / encode gamma 没有单独开关，始终按文件里的 `<color>` 执行：先 `pow(rgb, decodeGamma)`，融合和黑电平之后再 `pow(rgb, 1/encodeGamma)`。缺省或 `type="none"` 时 gamma 为 1，等于不处理。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/configFile` | 读写 | 校准文件路径（媒体库相对路径或绝对路径）。写入后重新加载并按 region 数重算输出口。 |
| `/bufferId` | 写 | 选择文件内某个 buffer。写入后重新加载；**未注册绑定**，不推送初始值/变更反馈。 |
| `/coordMode` | 读写 | 坐标解释模式，整数 0=自动 / 1=归一化 / 2=像素，越界回落 0。 |
| `/flipY` | 读写 | 翻转 Y（内容），布尔。 |
| `/flipWarpY` | 读写 | 翻转 Y（变形图），布尔。 |
| `/regionLocal` | 读写 | Region 局部，布尔。 |
| `/blend` | 读写 | Blend 融合开关，布尔。 |
| `/linearizeAlpha` | 读写 | Alpha 线性化开关，布尔。 |
| `/blackLevel` | 读写 | 黑电平开关，布尔。 |
| `/bounds` | 读写 | 区域边界调试显示，布尔。 |

除 `/bufferId` 外的 9 个地址均注册了外部绑定（member 指向带 NOTIFY 信号的真实 Q_PROPERTY）：模型就绪时推送一次当前值，此后属性每次变化（无论来自界面操作还是外部写入）都会自动推送状态反馈；外部写入经 `onGlobalEvent` 分发到对应 setter，即时生效并同步回界面控件。`/bufferId` 无绑定注册，只能写、无任何反馈。

## 5. 行为细节

```
IMAGE → 输入翻 V → 按 region 采样 warp UV → 可选融合 / 黑电平 → 输出翻 V → IMAGE[i]
```

- 显示管线的 V 翻转在 `runGl` 外侧（进 GPU 前把输入翻 V，出 GPU 后把结果翻回来），和界面上的两个翻转 Y 开关无关。
- 3D / Advanced 3D / Shader Lamp 文件只有在 warp 数据本身不像 2D UV 时才报错（提示导出 2D media）；数据实际是 2D 坐标时可以正常读。
- 帧驱动：挂在全局帧计数上，每帧排队触发一次处理；同一帧且参数未变、无需重初始化时直接跳过。
- 输入帧查找：按目标时间戳在输入的共享环形队列中向前回溯最多 64 帧找 GPU 纹理，找不到时回落最新一帧。
- 输入去重：输入帧时间戳与上次相同且参数、文件均未变化时跳过 GPU 重复计算；参数变更只置脏，下一帧生效。
- 输出时间戳为当前系统帧计数（不沿用输入时间戳），各 region 结果分别推入各自的输出环形队列。
- 无输入、输入断开或未选文件时清空全部输出队列，下游收不到帧。
- 文件 / buffer 变更时重建 warp 数据并按 region 数自动增删输出端口；加载失败节点进入错误状态并把输出口回落为 1 路；渲染失败同样报错并清空输出。
- 持久化：`values` 子对象 10 键（configFile / bufferId / coordMode / 7 个布尔开关）。加载时 coordMode 钳制到 0–2，布尔缺省按界面默认值（flipY、Blend、Alpha 线性化、黑电平 = 开，其余 = 关）；加载后强制重新初始化。

## 6. 使用说明与典型接法

1. 把 `.mpcdi` 导入媒体库（MPCDI 分类），或从媒体库拖到画布。
2. 选中文件。画面上下反了就切换「翻转 Y（内容）」；边缘发灰或融合过重时再看「Alpha 线性化」和 Blend。
3. IMAGE 接源；各输出口分别接 Display / Spout 等。
4. 校准文件更新后重新选择该文件即可重新加载。

