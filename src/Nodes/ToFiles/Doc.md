# To Files 节点

## 1. 节点说明

将流程中的图像或音频数据保存为媒体库文件，编码基于 FFmpeg。

包含四个节点：

- **To Image File**：触发时将当前图像保存为 PNG/JPG/BMP。
- **To Video File**：录制时将连续图像帧编码为 MP4。
- **To Audio File**：录制时将音频流保存为 WAV/MP3/AAC。
- **To Text File**：录制时将 VariableData 保存为 JSON 文本文件。

输出目录：`Documents/Flow/Medias/`（媒体库）。

## 2. 端口说明

### To Image File

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 待保存的图像 |
| START | VariableData | 为 `true` 时开始录制（持续覆盖保存当前帧） |
| STOP | VariableData | 为 `true` 时停止录制 |

### To Video File

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 连续视频帧 |
| START | VariableData | 为 `true` 时开始录制 |
| STOP | VariableData | 为 `true` 时停止录制并写入文件 |

### To Audio File

| 端口 | 类型 | 说明 |
|------|------|------|
| AUDIO 0 … AUDIO N | AudioData | 可编辑的音频输入端口；端口索引对应输出文件的声道索引 |
| START | VariableData | 为 `true` 时开始录制 |
| STOP | VariableData | 为 `true` 时停止录制并写入文件 |

端口可编辑：在节点属性面板中通过 **+ / -** 增减 `AUDIO` 端口。`START` 与 `STOP` 始终位于最后两个输入端口。录制开始时，输出文件的声道数等于当前 `AUDIO` 端口数量；每个端口输入的音频写入对应声道（多声道输入仅取第一声道）。

### To Text File

| 端口 | 类型 | 说明 |
|------|------|------|
| DATA | VariableData | 待保存的变量数据 |
| START | VariableData | 为 `true` 时开始录制 |
| STOP | VariableData | 为 `true` 时停止录制 |

录制期间，每次 `DATA` 更新都会将当前 VariableData 以 JSON 格式覆盖写入同一文件（使用 `toJsonString()`，UTF-8 编码）。单值写入 `{"default": ...}`，多键值写入完整 JSON 对象。

## 3. 属性说明

| 属性 / 界面 | 说明 |
|------|------|
| file | 输出文件名，如 `shot.png`、`clip.mp4`、`take.wav` |
| outputDir | 输出文件夹；留空则使用默认媒体库目录 `Documents/Flow/Medias/` |
| fps | 仅 To Video File：输出帧率，默认 25 |
| 状态标签 | 显示 **录制中**（红色）或 **未录制**（灰色） |
| 开始 / 停止按钮 | 界面手动控制录制 |

界面中文件名输入框右侧 **...** 按钮可打开文件夹选择对话框。鼠标悬停可查看当前输出目录。

外部控制地址：`/file`、`/outputDir`、`/fps`、`/start`、`/stop`、`/recording`（状态反馈）。

## 4. 使用说明

1. 设置 `file` 属性（含扩展名，扩展名决定编码格式）。
2. **图像**：图像源 → IMAGE，START 开始持续保存当前帧到同一文件，STOP 结束。
3. **视频**：图像源 → IMAGE，START 开始录制，STOP 结束并写入文件。
4. **音频**：一个或多个音频源 → 对应 `AUDIO` 端口（端口 0 → 左声道，端口 1 → 右声道，以此类推），START 开始录制，STOP 结束并写入文件。
5. **文本**：变量源 → DATA，START 开始录制并持续覆盖保存当前数据，STOP 结束。

## 5. 示例

NDI In → To Video File（record=true）→ 录制 10 秒后 record=false → 在媒体库查看 `output.mp4`。

Audio Decoder → To Audio File → 导出 `track.wav`。

Variable Out → To Text File → 导出 `state.json`。
