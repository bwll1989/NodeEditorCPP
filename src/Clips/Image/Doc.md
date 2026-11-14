# Image 片段

静态图片媒体片段（时间轴片段类型，type = `"Image"`）——2026-11-12 随 MediaServer 收编、2026-11-13 磁盘插件化（`image.plugin`）。

## 功能

- **模型** `ImageClipModel`：继承 BaseClass 的 `SceneClipModel`——青色块（#33B2A6）、`RESIZEABLE=true`（图片无固有播放时长，时间轴上可自由拉伸）、`EMBEDWIDGET=false`、`SHOWBORDER=false`。
- **无专属编辑区**：属性面板 = 通用页（时间/片段属性/媒体文件）+ 动画页。

## 结构（2026-11-15 对齐节点插件同构）

| 文件 | 职责 |
|---|---|
| `ImageClipModel.hpp` | 类型专属模型（类名=文件名，对齐节点 `<Name>DataModel.hpp`） |
| `PluginDefinition.hpp/.cpp` | 插件定义（class `Plugin` : `ClipPlugInterface`，对齐节点命名） |
| `CMakeLists.txt` | 独立 target（`plugin_image` → `bin/plugins/TimeLine/image.plugin`） |

## 加载（磁盘驱动，零内置兜底）

`WorkScenePluginLoader` 扫描 `bin/plugins/TimeLine/*.plugin`——删 `image.plugin` = Image 类型不存在。构建/输出约定详见 `../Video/Doc.md`。
