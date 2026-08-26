# 第三方开源项目声明

Flow / FlowRuntime 在开发与分发过程中使用了以下第三方开源项目及 SDK。  
本文档用于满足开源合规与致谢要求；各组件的具体许可证以其官方仓库或随附 LICENSE 文件为准。

预编译依赖通常位于仓库根目录的 `3rdParty/`；网页控制台前端依赖见 `WebInterface/package.json`。

---

## UI 与框架（C++ / Qt）

- [Qt 6](https://www.qt.io/) — 应用程序框架（GUI、网络、多媒体、QML 等）
- [QtNodes](https://github.com/bwll1989/nodeeditor) — 可视化节点图编辑器
- [Qt Advanced Docking System](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System) — 可停靠面板布局
- [QWindowKit](https://github.com/stdware/qwindowkit) — 无边框窗口
- [QtTimeLine](https://github.com/bwll1989/QtTimeLine) — 时间轴组件
- [Qt Color Widgets](https://github.com/mbasaglia/Qt-Color-Widgets) — 颜色选择控件
- [Qt Property Browser](https://github.com/qtpropertybrowser/qtpropertybrowser) — 属性树编辑器
- [QScintilla](https://www.riverbankcomputing.com/software/qscintilla/) — JavaScript 节点代码编辑器

## 网页控制台（WebInterface）

主网页控制台位于 [`WebInterface/`](../WebInterface/)，采用 Lit Web Components 重构。主要 npm 依赖：

- [Lit](https://lit.dev/) — Web Components UI 框架
- [@lit/context](https://github.com/lit/lit/tree/main/packages/context) — 组件上下文
- [Vite](https://vitejs.dev/) — 前端构建工具
- [TypeScript](https://www.typescriptlang.org/) — 类型化 JavaScript
- [ECharts](https://echarts.apache.org/) / [ECharts GL](https://github.com/ecomfe/echarts-gl) — 2D / 3D 图表
- [SortableJS](https://sortablejs.github.io/Sortable/) — 拖拽排序
- [marked](https://marked.js.org/) — Markdown 渲染
- [xss](https://github.com/leizongmin/js-xss) — HTML 过滤
- [@egjs/hammerjs](https://github.com/naver/hammer.js) — 手势识别
- [@mdi/js](https://github.com/Templarian/MaterialDesign-SVG) — Material Design Icons

布局与交互模式参考 [Home Assistant Frontend](https://github.com/home-assistant/frontend)（Sections / Lovelace 风格）；**并非直接打包 HA 前端 npm 包**，而是在 Flow 中独立实现同类架构（`FlowStore`、`registerCard`、`createCardElement` 等）。

> 仓库中仍保留早期 Vue 仪表盘控件源码（`src/Widget/ExternalControl/dashboard/`），供部分内置页面与 `ep-widgets` 兼容；新功能以 WebInterface 为主。

## 网络与通信

- [Poco C++ Libraries](https://pocoproject.org/) — HTTP 服务与网络基础库
- [QMQTT](https://github.com/emqx/qmqtt) — MQTT 客户端
- [TinyOSC](https://github.com/mhroth/tinyosc) — OSC 报文解析

## 音频

- [PortAudio](https://www.portaudio.com/) — 跨平台音频 IO
- [libltc](https://github.com/x42/libltc) — LTC 时间码编解码
- [Gist](https://github.com/adamstark/Gist) — 音频特征分析（含 [kiss_fft](https://github.com/mborgerding/kissfft)）
- [VST3 SDK](https://github.com/steinbergmedia/vst3sdk) — VST3 插件接口（Steinberg）
- [FMOD](https://www.fmod.com/) — 音频引擎（**商业 SDK**，非开源）

## 视频与图像

- [FFmpeg](https://ffmpeg.org/) — 音视频编解码（Clip 插件、VideoDecoder 节点等）
- [OpenCV](https://opencv.org/) — 计算机视觉与图像处理
- [Spout2](https://github.com/leadedge/Spout2) — GPU 纹理共享
- [NDI SDK](https://ndi.video/) — NDI 视频流（**商业 SDK**，NewTek / Vizrt）
- [mpv](https://mpv.io/) — 媒体播放器（Mpv Controller 节点通过 IPC 控制）

## AI 与推理

- [ONNX Runtime](https://onnxruntime.ai/) — ONNX 模型推理（YOLO v11n 检测 / 姿态 / 人脸、风格迁移等）
- **MediaPipe 风格视觉模型** — 手部 / 姿态 / 人体分割节点使用 OpenCV DNN 加载兼容 ONNX 权重，**未链接 Google MediaPipe C++ SDK**

## 脚本与序列化

- [Protocol Buffers](https://github.com/protocolbuffers/protobuf) — DAW Controller 等模块的结构化数据序列化

## 数学与基础库

- [Eigen](https://eigen.tuxfamily.org/) — 线性代数
- [spdlog](https://github.com/gabime/spdlog) — 日志库

---

## 商业 / 非开源 SDK

以下组件**不等同于开源软件**，使用与再分发须遵守其各自许可协议：

| SDK | 用途 |
|-----|------|
| [FMOD](https://www.fmod.com/) | Fmod 音频节点 |
| [NDI SDK](https://ndi.video/) | NDI In / Out 节点 |
| [VST3 SDK](https://www.steinberg.net/vst3/) | VST3 插件宿主 |
| [Aurora Remote SDK](https://www.slamtec.com/)（Slamtec） | Aurora S 设备节点 |

---

## 可选 / 未纳入默认构建

以下依赖在仓库历史中出现过，或源码目录存在但**当前默认构建未启用**，不在此逐一列出版本：

- Python / pybind11（PythonScriptNode）
- Lua / LuaBridge（LuaScriptNode）
- libpd / Pure Data
- QHotkey、QCustomPlot

节点插件可能仅引用上文部分库；未列出的依赖以对应 `CMakeLists.txt` 及官方文档为准。

---

## 致谢

感谢以上开源社区与 SDK 提供者的贡献。
