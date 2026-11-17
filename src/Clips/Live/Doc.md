# Live 片段

实时源媒体片段（时间轴片段类型，type = `"Live"`）——2026-11-12 随 MediaServer 收编、2026-11-13 磁盘插件化（`live.plugin`，原"仅内置注册"撤销）。

## 功能

- **模型** `LiveClipModel`：继承 BaseClass 的 `SceneClipModel`——青色块（#3CC4D9）、`RESIZEABLE=true`、`EMBEDWIDGET=false`、`SHOWBORDER=false`。
- **无镜像字段（单源铁律）**：Live 业务属性（NDI 源名/延迟）存于工作副本的 Live 素材（`Asset.live_*`，随场景 pb 分发），实况域唯一写者是实况区（直写素材），模型只镜像 `asset_id`（与 Text 同款"无文件素材"模式）。
- **专属编辑区** `LiveClipSection`：属性面板第 3 页（`ClipPlugInterface::createPropertySection` 工厂加载）——未绑定 Live 素材返回 nullptr（面板只含通用区）；广播灌值经 `ClipSectionInterface`。

## 结构（2026-11-15 对齐节点插件同构）

| 文件 | 职责 |
|---|---|
| `LiveClipModel.hpp` | 类型专属模型（类名=文件名，对齐节点 `<Name>DataModel.hpp`） |
| `LiveClipSection.hpp/.cpp` | 实况源专属编辑区（属性面板第 3 页） |
| `PluginDefinition.hpp/.cpp` | 插件定义（class `Plugin` : `ClipPlugInterface`，对齐节点命名） |
| `CMakeLists.txt` | 独立 target（`plugin_live` → `bin/plugins/TimeLine/live.plugin`，另链 MediaCore/GUIElements） |

## 加载（磁盘驱动，零内置兜底）

`WorkScenePluginLoader` 扫描 `bin/plugins/TimeLine/*.plugin`——删 `live.plugin` = Live 类型不可建（与其余类型行为一致）。构建/输出约定详见 `../Video/Doc.md`。

---

## 属性面板
三页式面板装配与 schema 化设计见 [`../../Common/ClipPanelBase/Doc.md`](../../Common/ClipPanelBase/Doc.md)。实况区经 `createPropertySection` 工厂装载（P3 迁库时协议类型化）。
