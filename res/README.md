# 第三方开源项目与 SDK 声明

Flow / FlowRuntime 在开发与分发过程中使用了以下第三方开源项目及 SDK。

本文档用于开源合规与致谢；各组件的具体许可证以其官方网站、仓库或随附 LICENSE 文件为准。列表基于当前发行版本整理，仅供参考。

---

## UI 与框架

| 组件 | 版本 | 用途 |
|------|------|------|
| [Qt 6](https://www.qt.io/) | 6.5.3 | 应用程序框架（界面、网络、多媒体、OpenGL、QML 等） |
| [QtNodes](https://github.com/bwll1989/nodeeditor) | — | 可视化节点图编辑器 |
| [Qt Advanced Docking System](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System) | — | 可停靠面板布局 |
| [QWindowKit](https://github.com/stdware/qwindowkit) | — | 无边框窗口 |
| [QtTimeLine](https://github.com/bwll1989/QtTimeLine) | — | 时间轴组件 |
| [Qt Color Widgets](https://github.com/mbasaglia/Qt-Color-Widgets) | — | 颜色选择控件 |
| Qt Property Browser（源自 Qt Solutions） | — | 属性编辑控件 |
| [QScintilla](https://www.riverbankcomputing.com/software/qscintilla/) | 2.14.1 | 代码编辑器组件 |
| [QCustomPlot](https://www.qcustomplot.com/) | — | 图表绘制 |

## 网页控制台

布局与交互模式参考 [Home Assistant Frontend](https://github.com/home-assistant/frontend)，以下为主要依赖库：

| 组件 | 版本 | 用途 |
|------|------|------|
| [Lit](https://lit.dev/) | 3.3 | Web Components UI 框架 |
| [@lit/context](https://github.com/lit/lit/tree/main/packages/context) | 1.1 | 组件上下文 |
| [ECharts](https://echarts.apache.org/) / [ECharts GL](https://github.com/ecomfe/echarts-gl) | 6.1 / 2.1 | 2D / 3D 图表 |
| [SortableJS](https://sortablejs.github.io/Sortable/) | 1.15 | 拖拽排序 |
| [marked](https://marked.js.org/) | 18 | Markdown 渲染 |
| [xss](https://github.com/leizongmin/js-xss) | 1.0 | HTML 过滤 |
| [@egjs/hammerjs](https://github.com/naver/hammer.js) | 2.0 | 手势识别 |
| [@mdi/js](https://github.com/Templarian/MaterialDesign-SVG) | 7.4 | Material Design Icons |

## 网络与通信

| 组件 | 版本 | 用途 |
|------|------|------|
| [Poco C++ Libraries](https://pocoproject.org/) | 1.15.3 | HTTP 服务与网络基础库 |
| [OpenSSL](https://www.openssl.org/) | — | Poco 安全网络组件的加密依赖 |
| [QMQTT](https://github.com/emqx/qmqtt) | — | MQTT 客户端 |
| [TinyOSC](https://github.com/mhroth/tinyosc) | — | OSC 报文解析 |

## 音频

| 组件 | 版本 | 用途 |
|------|------|------|
| [PortAudio](https://www.portaudio.com/) | — | 跨平台音频 IO |
| [libltc](https://github.com/x42/libltc) | — | LTC 时间码编解码 |
| [Gist](https://github.com/adamstark/Gist)（含 [kiss_fft](https://github.com/mborgerding/kissfft)） | 1.0.7 | 音频特征分析 |
| [VST3 SDK](https://github.com/steinbergmedia/vst3sdk) | 3.7.14 | VST3 插件接口（Steinberg） |
| [FMOD](https://www.fmod.com/) | — | 音频引擎（商业 SDK） |

## 视频、图像与 GPU

| 组件 | 版本 | 用途 |
|------|------|------|
| [FFmpeg](https://ffmpeg.org/) | 8.1.1 | 音视频编解码 |
| [OpenCV](https://opencv.org/) | 4.10.0 | 计算机视觉与图像处理 |
| [Spout2](https://github.com/leadedge/Spout2) | 2.007 | GPU 纹理共享 |
| [NDI SDK](https://ndi.video/) | 6 | NDI 视频流（商业 SDK） |
| [mpv](https://mpv.io/) | — | 媒体播放器控制（由用户另行安装，发行包不附带） |
| [VVISF-GL](https://github.com/mrRay/VVISF-GL) | — | [ISF](https://github.com/mrRay/ISF_Spec) 着色器加载与渲染（含 VVGL / VVISF、[GLEW](https://glew.sourceforge.net/)） |
| [VIOSO WarpBlend API](https://github.com/vioso/VIOSO_API) | — | 投影机校准 Warp + Blend（商业 SDK） |

## AI 与推理

| 组件 | 版本 | 用途 |
|------|------|------|
| [ONNX Runtime](https://onnxruntime.ai/) | 1.23.0（GPU） | ONNX 模型推理（目标检测、姿态、人脸、风格迁移等） |

## 脚本与序列化

| 组件 | 版本 | 用途 |
|------|------|------|
| [Protocol Buffers](https://github.com/protocolbuffers/protobuf)（含 [Abseil](https://github.com/abseil/abseil-cpp) / utf8_range） | — | 结构化数据序列化 |

## 数学与基础库

| 组件 | 版本 | 用途 |
|------|------|------|
| [Eigen](https://eigen.tuxfamily.org/) | 3.4.0 | 线性代数 |
| [spdlog](https://gabime.github.io/spdlog/) | — | 日志库 |

---

## 商业 / 非开源 SDK

以下组件**不等同于开源软件**，使用与再分发须遵守其各自许可协议：

| SDK | 用途 |
|-----|------|
| [FMOD](https://www.fmod.com/) | 音频引擎 |
| [NDI SDK](https://ndi.video/) | NDI 视频输入 / 输出 |
| [VST3 SDK](https://www.steinberg.net/vst3/) | VST3 插件宿主 |
| [Aurora Remote SDK](https://www.slamtec.com/)（Slamtec） | Aurora S 设备接入 |
| [VIOSO WarpBlend API](https://github.com/vioso/VIOSO_API) | 投影校准 / Warp + Blend |

---

## 可选组件

以下组件在部分功能或历史版本中使用，**不包含在当前默认发行版中**：

| 组件 | 用途 |
|------|------|
| Python / [pybind11](https://github.com/pybind/pybind11) | Python 脚本扩展 |
| Lua / [LuaBridge](https://github.com/vinniefalco/LuaBridge) | Lua 脚本扩展 |
| [libpd](https://github.com/libpd/libpd) / Pure Data | Pure Data 音频 |
| [QHotkey](https://github.com/Skycoder42/QHotkey) | 全局热键 |
| QDetailsView | 详情视图控件 |

---

## 致谢

感谢以上开源社区与 SDK 提供者的贡献。
