# Camera

## 1. 节点说明

从本地摄像头采集实时画面，经 **项目自带 FFmpeg（`avdevice` / DirectShow `dshow`）** 打开设备并解码，转为 `cv::Mat`（BGR）后在 GUI 线程上传 GPU 纹理，写入 **ImageTimestampRingQueue**，输出稳定的 **ImageData** 共享句柄。

采集在独立线程中运行，避免阻塞界面；与 VideoDecoder / RTSP / ImageLoader 一致，**不**每帧 `dataUpdated`，下游按 **TimestampGenerator tick** 调用 `getLatestFrame()`。

不再使用 Qt Multimedia / OpenCV `VideoCapture`（及其捆绑 FFmpeg）。

## 2. 数据流

```text
CameraCaptureThread
  FFmpeg dshow → avcodec → sws(BGR24) → cv::Mat
       ↓ QueuedConnection
CameraModel::onFrameAvailable（合并为最新 pending 帧）
       ↓ GUI 线程
ImageFrame::fromMat → ImageGpuUpload → pushFrame(timestamp = 当前系统帧号)
m_outImageData ──→ Image Display / ImageOperates / …
```

## 3. 端口说明

### 输入

- **输入 0**（ImageData）：保留端口，当前未使用。

### 输出

- **输出 0**（ImageData）：共享 ring buffer 句柄；有效帧在 buffer 内，非每帧新建对象。

## 4. 界面说明

- **设备下拉框**：通过 `avdevice_list_input_sources(dshow)` 列出视频输入设备，选择后自动开始采集。当前设备（`device` / `displayName`）写入节点 JSON。
- **采集状态**：勾选框反映当前是否正在捕获。

## 5. 使用说明

1. 添加节点，在属性面板中选择摄像头。未保存过设备时，使用枚举到的第一路。
2. 输出接到 Image Display、Image Scale、Image Blur 等下游节点。
3. 更换设备时重新选择下拉项并保存工程；停止时 ring buffer 会被清空，Display 下一 tick 清屏。
4. FlowRuntime 不创建属性面板，加载工程后按保存的设备名直接 `startCapture`。旧工程没有设备字段时，同样回退到第一路摄像头。
5. 运行时需能加载工程 `3rdParty/ffmpeg-*-full_build-shared/bin` 下的 `avdevice` 等 DLL（与 VideoDecoder / RTSP 相同）。

## 6. 实现要点

| 项目 | 说明 |
|------|------|
| 采集 | `CameraCaptureThread` + FFmpeg `dshow`（`video=设备名`）；`load()` 后启动，不依赖界面 |
| 持久化 | `values.device`（打开名）、`values.displayName`（友好名） |
| 枚举 | `avdevice_list_input_sources`，仅视频设备 |
| 上传 | `ImageGpuUpload::warmup()` + `ImageFrame::fromMat`（须 GUI 线程） |
| 合并 | pending 帧合并，避免 GL 上传积压 |
| 时间戳 | `TimestampGenerator::getCurrentFrameCount()` |
| CPU 缓存 | `ImageFrame::fromMat` 始终保留 CPU 副本，便于下游 OpenCV 读 `frame.image` |
| 停止 | `interrupt_callback` 打断阻塞的 `av_read_frame` |

## 7. 示例

USB 摄像头 → **Camera** → Image Scale（1280×720）→ Image Display，用于现场画面监测。
