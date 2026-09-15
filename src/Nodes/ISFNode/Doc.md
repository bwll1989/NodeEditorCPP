# ISF

## 1. 节点说明

使用 [VVISF-GL](https://github.com/mrRay/VVISF-GL) 加载 `.fs`（[ISF](https://github.com/mrRay/ISF_Spec)）着色器，按 **Resolume Wire** 风格把 `INPUTS` **动态映射为输入端口**。

依赖：`3rdParty/VVISF-GL`（构建 VVGL / VVISF，运行时需 `glew32.dll`）。

## 2. 端口说明

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

## 3. 界面说明

| 控件 | 说明 |
|------|------|
| 文件选择 | 媒体库 **ISF** 分类中的 `.fs` |
| Reload | **强制**重新编译同路径 `.fs`，并重建端口 / 面板 |
| 动态参数区 | float / bool / long / event / color / point2D |
| audio 行 | `(audio → wave)`，需接 AudioData |
| audioFFT 行 | `(audio → FFT)`，需接 AudioData |

## 4. 实现要点

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

## 5. 使用说明

1. 将 `.fs` 导入媒体库（ISF 分类）。可从 [ISF-Files](https://github.com/Vidvox/ISF-Files) 获取。
2. 放置 ISF 节点，选择文件；按需连接图像与参数口。
3. 修改磁盘上的 `.fs` 后点 **Reload**。
4. 输出接 Display / SpoutOut 等。

## 6. 示例

- Video Decoder → ISF（Filter `.fs`）→ Display
- ISF（Generator `.fs`，无 image 输入）→ Display
