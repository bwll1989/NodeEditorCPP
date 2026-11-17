# Video 片段

视频媒体片段（时间轴片段类型，type = `"Video"`）——合并前编辑器的视频轨道片段，2026-11-12 随 MediaServer 收编、2026-11-13 磁盘插件化（`video.plugin`）。

## 功能

- **模型** `VideoClipModel`：继承 BaseClass 的 `SceneClipModel`——橙色块（#E68A33）、`RESIZEABLE=true`（拖右缘改时长）、`EMBEDWIDGET=false`（属性走双击对话框）、`SHOWBORDER=false`。
- **纯时间区间承载**：媒体引用经 `asset_id` 挂在工作场景（随场景 pb 分发），画布标注/素材收集由宿主按 kind 分派——本插件只管时间轴上的片段本体。
- **无专属编辑区**：属性面板 = 通用页（时间/片段属性/媒体文件）+ 动画页（`createPropertySection` 未实现）。

## 结构（2026-11-15 对齐节点插件同构）

| 文件 | 职责 |
|---|---|
| `VideoClipModel.hpp` | 类型专属模型（类名=文件名，对齐节点 `<Name>DataModel.hpp`） |
| `PluginDefinition.hpp/.cpp` | 插件定义（class `Plugin` : `ClipPlugInterface`，对齐节点命名） |
| `CMakeLists.txt` | 独立 target（`plugin_video` → `bin/plugins/TimeLine/video.plugin`） |

## 加载（磁盘驱动，零内置兜底）

`WorkScenePluginLoader` 扫描 `bin/plugins/TimeLine/*.plugin`——删 `video.plugin` = Video 类型不存在（Add Track 不露项、含 Video 的场景报类型缺失）。新增媒体类型 = 新增一个插件目录（父级 CMakeLists 类型清单追加）。

---

## 属性面板
三页式面板（通用/动画/专属）装配与 schema 化设计见 [`../../Common/ClipPanelBase/Doc.md`](../../Common/ClipPanelBase/Doc.md)。Video 无专属页；动画页 = 变换七属性（全类型基准暴露面）。
