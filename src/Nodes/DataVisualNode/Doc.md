# CurveNode 帮助文档

## 概述
CurveNode 是一个时间轴曲线驱动节点，用于按时间输出数值与状态，并与内置的 QML Timeline 编辑器联动。节点支持播放、暂停、定位以及循环播放，能够将时间轴数据与循环设置一起持久化到项目文件，在加载时自动恢复。

## 功能特性
- **时间轴编辑**：支持关键帧曲线、播放、暂停与定位（seek）
- **循环播放**：支持循环播放开关，UI 与 QML 状态双向同步
- **数据持久化**：保存与加载时间轴 JSON 数据和循环设置（`values.timeline` / `values.timelineString` / `values.loop`）
- **OSC 控制**：支持外部控制接口（如 `/start`、`/loop`）
- **独立编辑窗口**：内嵌界面可弹出为独立窗口，以便编辑
- **状态反馈**：播放状态更新、时间与数值输出皆可回传

## 输入输出端口
- **输入**
  - `TRIGGER`：布尔触发，控制播放/暂停
- **输出**
  - `TIME`：当前播放时间（秒）
  - `VALUE_X`：当前曲线 X 值
  - `VALUE_Y`：当前曲线 Y 值
  - `STATUS`：播放状态（布尔）

## 界面组件
- **Start 按钮**：控制播放/暂停，与 `STATUS` 输出联动
- **Loop 复选框**：控制是否循环播放，与 QML `TimelineEditor.isLoop` 双向同步
- **TimelineEditor**：QML 时间轴编辑器，id 为 `timelineEditor`

## 数据持久化说明
- **保存（save）**
  - 从 QML 根项 `savedData` 读取时间轴 JSON 字符串。
  - 若解析为对象成功，写入 `values.timeline`；否则写入 `values.timelineString`。
  - 将循环状态写入 `values.loop`（布尔值）。
- **加载（load）**
  - 若存在 `values.timeline` 或 `values.timelineString`，写回根项 `savedData`，并通过 `WebEngineView.runJavaScript` 导入到页面（带防护判断，页面就绪时才执行）。
  - 从 `values.loop` 恢复循环播放设置：勾选 UI 复选框，同时调用 QML `TimelineEditor.setIsLoop()` 保持逻辑一致。

### 与 QML 的同步机制
- QML 端（main.qml）
  - `CheckBox.checked` 绑定到 `timeline.isLoop`
  - 在 `onCheckedChanged` 中：
    - 写回 `timeline.isLoop = checked`
    - 调用 `CppBridge.updateLoop(checked)`，将状态同步回 C++
- C++ 端
  - `CurveDataModel::onLoopCheckBoxClicked()` 调用 QML `setIsLoop(...)`，确保 UI 操作能更新 QML
  - `CurveDataModel::updateLoop(bool)` 勾选 UI 复选框，用于接收来自 QML 的同步回写
  - `save()`/`load()` 负责 `values.loop` 的持久化与恢复，保证跨会话的一致性

## 使用说明
1. 将触发信号接入 `TRIGGER` 输入端口以控制播放/暂停。
2. 打开独立编辑窗口，编辑时间轴曲线与关键帧。
3. 勾选或取消 `Loop` 复选框以设置循环播放。
4. 保存项目文件后，时间轴与循环设置会自动持久化。
5. 重新加载项目时，时间轴数据与 `Loop` 状态会自动恢复。

## OSC 控制
- 支持注册的控制项：
  - `/start`：控制播放/暂停（布尔）
  - `/loop`：控制循环播放（布尔）
- 状态反馈：
  - 通过 `stateFeedBack` 接口回传到 `AppConstants::EXTRA_FEEDBACK_HOST` 与 `EXTRA_FEEDBACK_PORT` 的地址空间（形如 `/dataflow/<alias>/<nodeId>/...`）

## 依赖与环境
- **操作系统**：Windows（推荐 Windows 10 及以上）
- **框架**：Qt 6（Widgets + QML + WebEngine）、QtNodes
- **时间轴组件**：QtTimeLine
- **多媒体**：FFmpeg 7.1（全局依赖项）
- **编译器**：MSVC（C++17）
- **其他**：可选 OpenGL（用于渲染相关的依赖）

## 故障排除
- **Loop 不生效/不同步**
  - 确认 `CurveDataModel::onLoopCheckBoxClicked()` 能被触发，且 `timelineEditor` 能查找到。
  - 确认 QML `onCheckedChanged` 中调用了 `CppBridge.updateLoop(checked)`。
- **时间轴未导入**
  - 检查 `values.timeline` 与 `values.timelineString` 是否存在且格式正确。
  - 确认 `WebEngineView.runJavaScript` 调用加了 `typeof timeline !== 'undefined'` 防护判断。
- **状态回传异常**
  - 检查 `AppConstants::EXTRA_FEEDBACK_HOST/PORT` 与 OSC 发送队列是否正常。

## 版本信息
- **节点版本**：随构建时间注入到 `PRODUCT_VERSION`（主工程）或插件宏（若为模块化插件）
- **依赖库版本**：Qt 6、QtNodes
- **更新日期**：2025年