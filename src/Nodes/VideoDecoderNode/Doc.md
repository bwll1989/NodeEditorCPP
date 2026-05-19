# VideoDecoderNode 使用说明

## 用途
使用 FFmpeg 7.1 解码媒体文件（视频+音频），输出视频帧（ImageData）与按声道拆分的音频流（AudioData）。

## 端口
### 输入（VariableData）
- PLAY/STOP：播放开关（bool）
- STOP：停止（bool，true 时停止）
- LOOP：循环（bool）
- GAIN：音量（double，单位 dB）

### 输出
- Image（ImageData）：解码后的视频帧
- CH 1..CH N（AudioData）：按声道拆分的音频输出（N 会随文件实际声道数动态变化）

## 参数/界面
- File：选择媒体库中的文件
- Play：播放/暂停
- Loop：循环
- Volume：音量（dB）
- 进度条/时间：显示播放进度

## 外部控制（可选）
- /fileName（string）：设置文件名（媒体库条目）
- /play（bool）：播放/停止
- /loop（bool）：循环开关
- /volume（double）：音量（dB）

## 使用步骤
1. 选择文件（File 或 /fileName）。
2. 将 Image 连接到图像处理/显示节点；将 CH 1..N 连接到音频处理/输出节点。
3. 通过 PLAY/STOP 或 /play 开始播放；需要循环则打开 LOOP。 
