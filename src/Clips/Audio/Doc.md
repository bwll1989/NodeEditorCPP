# Audio 片段

音频媒体片段（时间轴片段类型，type = `"Audio"`）——2026-11-12 随 MediaServer 收编、2026-11-13 磁盘插件化（`audio.plugin`）。

## 功能

- **模型** `AudioClipModel`：继承 BaseClass 的 `SceneClipModel`——紫色块（#9B6FD1）、`RESIZEABLE=true`、`EMBEDWIDGET=false`、`SHOWBORDER=true`。
- **时间信息承载**：Player 仅消费音频的时间信息（方案 §4.4，编辑器预览不出声——预览音频走 TimelineAudioBusHost 混音通道）；画布无几何呈现，片段只承载时间区间（入出点后续扩展）。
- **无专属编辑区**：属性面板 = 通用页（时间/片段属性/媒体文件）+ 动画页。

## 结构（2026-11-15 对齐节点插件同构）

| 文件 | 职责 |
|---|---|
| `AudioClipModel.hpp` | 类型专属模型（类名=文件名，对齐节点 `<Name>DataModel.hpp`） |
| `PluginDefinition.hpp/.cpp` | 插件定义（class `Plugin` : `ClipPlugInterface`，对齐节点命名） |
| `CMakeLists.txt` | 独立 target（`plugin_audio` → `bin/plugins/TimeLine/audio.plugin`） |

## 加载（磁盘驱动，零内置兜底）

`WorkScenePluginLoader` 扫描 `bin/plugins/TimeLine/*.plugin`——删 `audio.plugin` = Audio 类型不存在。构建/输出约定详见 `../Video/Doc.md`。
