# ISF

## 1. 节点说明

使用 [VVISF-GL](https://github.com/mrRay/VVISF-GL) 加载 `.fs`（[ISF](https://github.com/mrRay/ISF_Spec)）着色器，按 **Resolume Wire** 风格把 `INPUTS` **动态映射为输入端口**。

依赖：`3rdParty/VVISF-GL`（构建 VVGL / VVISF，运行时需 `glew32.dll`）。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 0 | 否 | 0 | 0 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入（随 `.fs` 变化）

| ISF TYPE | 端口类型 | 说明 |
|----------|----------|------|
| `image` | ImageData | 图像纹理 |
| `float` / `bool` / `long` / `event` | VariableData | 标量；`event` 为单帧脉冲 |
| `color` | VariableData | `[r,g,b,a]`，优先 0–1 |
| `point2D` | VariableData | `[x,y]`；MIN/MAX 为二维 |
| `audio` | AudioData | 节点内 → 波形纹理（0.5 中心） |
| `audioFFT` | AudioData | 节点内 Gist FFT → 频谱纹理 |
| `cube` | （暂不建口） | 暂缓 |

未连线的标量口使用面板值 / 存盘值（或文件 DEFAULT）。
未连线的 audio / audioFFT 喂静音行，**不阻断**渲染。

### 输出

- **IMAGE**（ImageData）：渲染结果

## 3. 界面与属性

| 控件 | 说明 |
|------|------|
| 文件选择 | 媒体库 **ISF** 分类中的 `.fs` |
| Reload | **强制**重新编译同路径 `.fs`，并重建端口 / 面板 |
| 动态参数区 | float / bool / long / event / color / point2D |
| audio 行 | `(audio → wave)`，需接 AudioData |
| audioFFT 行 | `(audio → FFT)`，需接 AudioData |

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/shaderFile` | 读/写 | 切换 `.fs` 文件（媒体库 ISF 分类路径；变更后重编译并重建端口/面板） |
| `/reload` | 读/写 | payload 为 true 时强制重新编译当前 `.fs`（`reload` 属性恒读 false，仅作触发） |

各动态标量参数（float/bool/long/color/point2D）为面板控件值，未注册逐参数 OSC 地址，需经输入端口写入。

## 5. 行为细节

- **动态端口**：加载 `.fs` 后按 ISF `INPUTS` 重建输入端口与面板（先换端口表再改 InPortCount）；`cube` 类型暂不建口。
- **渲染管线**：

```
IMAGE(s) / AudioData(s) / Variable(s)
  → audio: PCM 滑动窗 → 波形行 [0,1]
  → audioFFT: PCM 滑动窗 → Gist → 频谱行 [0,1]
  → wrap(image flipped=true) + upload audio RGBA32F
  → setBuffer / setBufferForAudioInputKey
  → createAndRenderABuffer → blit → 输出翻 V → IMAGE
```

| 主题 | 约定 |
|------|------|
| GL 上下文 | 仅在 `ImageGpuUpload::runGl` 内触碰 VVGL / 纹理 |
| 音频 MAX | JSON `MAX` = 纹理宽度；默认 512 |
| FFT | 窗长 2048，峰值归一化；与 AudioAnalysis 同用 Gist |
| 缺音频 | 静音波形 0.5 / 零频谱，不 clearOutput |
| 缺图 | 任一必填 image 缺失则清空输出 |

- **Reload 语义**：`setReload(true)` 触发重编译；属性读取恒为 false（不保存状态）。
- **持久化**：`values.shaderFile` + `values.scalars`（各标量参数当前值），加载后按存盘值恢复面板。

## 6. 使用说明与典型接法

1. 将 `.fs` 导入媒体库（ISF 分类）。可从 [ISF-Files](https://github.com/Vidvox/ISF-Files) 获取。
2. 放置 ISF 节点，选择文件；按需连接图像与参数口。
3. 修改磁盘上的 `.fs` 后点 **Reload**。
4. 输出接 Display / SpoutOut 等。

## 7. 示例

- Video Decoder → ISF（Filter `.fs`）→ Display
- ISF（Generator `.fs`，无 image 输入）→ Display

