# MPCDI

## 1. 节点说明

用 Scalable Display 的 MPCDI 参考实现读取 `.mpcdi`，对输入图像做几何变形（geometry warp）和边缘融合（alpha map）。按所选 buffer 里的 region 数自动展开多路输出，一路对应一台投影机。

依赖：`3rdParty/Scalable Display Reference Implementation`（静态编进插件，不另带 DLL）。该库实现的是 MPCDI 1.0；更高版本会关掉版本检查后尽量读。

## 2. 端口说明

### 输入

- **IMAGE**：待变形的源图像，对应 MPCDI buffer 的整幅画面。

### 输出

- **IMAGE** / 各 region id：N = 所选 buffer 的 region 数（上限 64）。

## 3. 界面说明

文件选择：媒体库 MPCDI 分类中的 `.mpcdi`。换文件会重新读校准并按 region 数重算输出口。文件里有多个 buffer 时使用已保存的 buffer id，否则用第一个。

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

## 4. 实现要点

```
IMAGE → 输入翻 V → 按 region 采样 warp UV → 可选融合 / 黑电平 → 输出翻 V → IMAGE[i]
```

- 显示管线的 V 翻转在 `runGl` 外侧，和界面上的两个翻转 Y 无关。
- 3D / Shader Lamp 里存的是屏幕曲面坐标而不是 2D UV 时，节点报错，需要 2D media 的 MPCDI。

## 5. 使用说明

1. 把 `.mpcdi` 导入媒体库（MPCDI 分类），或从媒体库拖到画布。
2. 选中文件。画面上下反了就切换「翻转 Y（内容）」；边缘发灰或融合过重时再看「Alpha 线性化」和 Blend。
3. IMAGE 接源；各输出口分别接 Display / Spout 等。
4. 校准文件更新后重新选择该文件即可重新加载。
