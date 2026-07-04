# 第三方开源项目声明

Flow / FlowRuntime 在开发与分发过程中使用了以下第三方开源项目及 SDK。  
本文档用于满足开源合规与致谢要求；各组件的具体许可证以其官方仓库或随附 LICENSE 文件为准。

---

## UI 与框架

- [Qt 6](https://www.qt.io/) — 应用程序框架（GUI、网络、多媒体、QML 等）
- [QtNodes](https://github.com/bwll1989/nodeeditor) — 可视化节点图编辑器
- [Qt Advanced Docking System](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System) — 可停靠面板布局
- [QWindowKit](https://github.com/stdware/qwindowkit) — 无边框窗口
- [QtTimeLine](https://github.com/bwll1989/QtTimeLine) — 时间轴组件
- [Qt Color Widgets](https://github.com/mbasaglia/Qt-Color-Widgets) — 颜色选择控件
- [Qt Property Browser](https://github.com/qtpropertybrowser/qtpropertybrowser) — 属性树编辑器
- [QScintilla](https://www.riverbankcomputing.com/software/qscintilla/) — 代码编辑器组件
- [QHotkey](https://github.com/Skycoder42/QHotkey) — 全局热键

## 网络与通信

- [Poco C++ Libraries](https://pocoproject.org/) — HTTP 服务与网络基础库
- [QMQTT](https://github.com/emqx/qmqtt) — MQTT 客户端
- [TinyOSC](https://github.com/mhroth/tinyosc) — OSC 报文解析

## 音频

- [PortAudio](https://www.portaudio.com/) — 跨平台音频 IO
- [libltc](https://github.com/x42/libltc) — LTC 时间码编解码
- [VST3 SDK](https://github.com/steinbergmedia/vst3sdk) — VST3 插件接口（Steinberg）
- [FMOD](https://www.fmod.com/) — 音频引擎（商业 SDK，非开源）
- [Gist](https://github.com/adamstark/Gist) — 音频特征分析

## 视频与图像

- [FFmpeg](https://ffmpeg.org/) — 音视频编解码
- [OpenCV](https://opencv.org/) — 计算机视觉与图像处理
- [Spout2](https://github.com/leadedge/Spout2) — GPU 纹理共享
- [NDI SDK](https://ndi.video/) — NDI 视频流（NewTek / Vizrt）
- [mpv](https://mpv.io/) — 媒体播放器

## AI 与推理

- [ONNX Runtime](https://onnxruntime.ai/) — ONNX 模型推理
- [Google MediaPipe](https://github.com/google/mediapipe) — 人体姿态、手势等视觉能力

## 脚本与扩展

- [Python](https://www.python.org/) — 脚本运行时
- [pybind11](https://github.com/pybind/pybind11) — C++/Python 绑定
- [Lua](https://www.lua.org/) — 脚本语言
- [LuaBridge](https://github.com/vinniefalco/LuaBridge) — C++/Lua 绑定
- [Protocol Buffers](https://github.com/protocolbuffers/protobuf) — 结构化数据序列化

## 数学与基础库

- [Eigen](https://eigen.tuxfamily.org/) — 线性代数
- [spdlog](https://github.com/gabime/spdlog) — 日志库

## 其他

- [libpd](https://github.com/libpd/libpd) / [Pure Data](https://puredata.info/) — 音频补丁集成
- [QCustomPlot](https://www.qcustomplot.com/) — 图表控件

---

## 说明

- 部分条目为**商业 SDK**（如 FMOD、NDI、VST3），使用与再分发须遵守其各自许可协议，**不等同于开源软件**。
- 节点插件可能仅引用上述部分库；未列出的依赖以对应源码目录及官方文档为准。

## 致谢

感谢以上开源社区与 SDK 提供者的贡献。
