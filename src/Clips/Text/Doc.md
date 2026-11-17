# Text 片段

字幕/文本媒体片段（时间轴片段类型，type = `"Text"`）——2026-11-05 文本片段完整化，2026-11-12 随 MediaServer 收编、2026-11-13 磁盘插件化（`text.plugin`）。

## 功能

- **模型** `TextClipModel`：继承 BaseClass 的 `SceneClipModel`——黄色块（#D9C23C）、`RESIZEABLE=true`、`EMBEDWIDGET=false`、`SHOWBORDER=false`。
- **无镜像字段（单源铁律）**：文本内容/字体/字号/颜色等业务属性存于工作副本的 Text 素材（`Asset.text_*`，随场景 pb 分发），文本域唯一写者是文本区（直写素材），模型只镜像 `asset_id`（画布渲染/依赖联动）。
- **专属编辑区** `TextClipSection`：属性面板第 3 页（`ClipPlugInterface::createPropertySection` 工厂加载）——未绑定文本素材返回 nullptr（面板只含通用区）；广播灌值经 `ClipSectionInterface`。

## 结构（2026-11-15 对齐节点插件同构）

| 文件 | 职责 |
|---|---|
| `TextClipModel.hpp` | 类型专属模型（类名=文件名，对齐节点 `<Name>DataModel.hpp`） |
| `TextClipSection.hpp/.cpp` | 文本专属编辑区（属性面板第 3 页） |
| `PluginDefinition.hpp/.cpp` | 插件定义（class `Plugin` : `ClipPlugInterface`，对齐节点命名） |
| `CMakeLists.txt` | 独立 target（`plugin_text` → `bin/plugins/TimeLine/text.plugin`，另链 MediaCore/GUIElements） |

## 加载（磁盘驱动，零内置兜底）

`WorkScenePluginLoader` 扫描 `bin/plugins/TimeLine/*.plugin`——删 `text.plugin` = Text 类型不存在。构建/输出约定详见 `../Video/Doc.md`。

---

## 属性面板
三页式面板装配与 schema 化设计见 [`../../Common/ClipPanelBase/Doc.md`](../../Common/ClipPanelBase/Doc.md)。文本区经 `createPropertySection` 工厂装载（P3 迁库时协议类型化）。
