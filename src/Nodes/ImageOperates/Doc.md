# ImageOperates 节点包

## 1. 节点说明

`ImageOperates` 是一个图像操作节点集合插件，当前包含以下几个子节点：

- `Image Flip`：图像水平/垂直翻转，类似 TouchDesigner 的 `Flip TOP`
- `Image Cross`：在两路图像之间按浮点比例交叉混合
- `Image Difference`：输出两路图像的差异图像，类似 `Difference TOP`
- `Image Blur`：对图像进行高斯模糊，类似 `Blur TOP`
- `Image Crop`：按上下左右百分比裁剪图像，类似 `Crop TOP`
- `Image Chroma Key`：按色相范围生成透明度，类似 `Chroma Key TOP`
- `Image Under`：将输入2叠加到输入1上，使用输入2的 alpha 决定输入1的可见区域，类似 `Under TOP`

## 2. 子节点说明

### Image Flip

- 输入：
  - `IMAGE`
  - `H`：是否水平翻转
  - `V`：是否垂直翻转
- 输出：
  - `IMAGE`

### Image Cross

- 输入：
  - `A`
  - `B`
  - `BLEND`：`0` 输出 A，`1` 输出 B，中间值做线性混合
- 输出：
  - `IMAGE`

说明：

- `BLEND` 范围为 `0-1`
- 两路图像尺寸不一致时会先将 B 缩放到 A 的尺寸

### Image Difference

- 输入：
  - `A`
  - `B`
  - `GAIN`：差异结果增益
- 输出：
  - `IMAGE`

说明：

- 内部会将两张图像统一到相同尺寸与 3 通道格式后再做差异比较
- 输出结果为 `absdiff` 差异图像

### Image Blur

- 输入：
  - `IMAGE`
  - `RADIUS X`
  - `RADIUS Y`
- 输出：
  - `IMAGE`

说明：

- 使用高斯模糊实现
- 半径为 `0` 时直接透传输入图像
- `RADIUS X` 与 `RADIUS Y` 范围为 `0-32`

### Image Crop

- 输入：
  - `IMAGE`
  - `LEFT %`
  - `RIGHT %`
  - `TOP %`
  - `BOTTOM %`
- 输出：
  - `IMAGE`

说明：

- 四个百分比范围均为 `0-100`
- 百分比基于当前输入图像宽高分别计算
- 输出尺寸会变为裁剪后的实际尺寸
- 当四边裁剪总量导致宽或高小于等于 `0` 时，输出为空图像

### Image Chroma Key

- 输入：
  - `IMAGE`
  - `HUE MIN`
  - `HUE MAX`
  - `SOFT LOW`
  - `SOFT HIGH`
- 输出：
  - `IMAGE`

说明：

- `HUE MIN` 与 `HUE MAX` 范围为 `0-360`
- `SOFT LOW` 与 `SOFT HIGH` 范围为 `0-360`
- 输出图像保留原始颜色信息，并写入 alpha 通道作为抠像结果
- 当输入为 4 通道图像时，会保留原始 alpha 并与新的抠像 alpha 相乘
- `SOFT LOW` 作用于 `HUE MIN` 一侧的过渡，`SOFT HIGH` 作用于 `HUE MAX` 一侧的过渡

### Image Under

- 输入：
  - `A`
  - `B`
- 输出：
  - `IMAGE`

说明：

- 结果为 `B over A`：将输入 `B` 叠加到 `A` 上
- `B` 的 alpha 用于决定 `A` 在结果中的可见区域（alpha=1 时完全遮挡 A，alpha=0 时完全显示 A）
- 如果 `A/B` 尺寸不同，会将 `B` 缩放到 `A` 尺寸后再合成
- 输出为 4 通道 BGRA，并输出合成后的 alpha

## 3. 使用建议

- `Image Flip` 适合做镜像、上下翻转、纹理方向修正
- `Image Cross` 适合做 A/B 画面淡入淡出与交叉混合
- `Image Difference` 适合检测前后帧、参考图与当前图的变化区域
- `Image Blur` 适合做柔化、背景模糊、预处理
- `Image Crop` 适合做画面安全区裁剪、局部取景与边缘留白调整
- `Image Chroma Key` 适合做绿幕、蓝幕或指定色相范围的透明抠除
- `Image Under` 适合做图层合成、贴图叠加与基于 alpha 的遮罩叠放
