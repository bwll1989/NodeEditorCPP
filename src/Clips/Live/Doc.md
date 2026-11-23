# Live 片段

实时源媒体片段（时间轴片段类型，type = `"Live"`）——2026-11-12 随 MediaServer 收编、2026-11-13 磁盘插件化（`live.plugin`，原"仅内置注册"撤销）。

## 功能

- **模型** `LiveClipModel`：继承 BaseClass 的 `SceneClipModel`——青色块（#3CC4D9）、`RESIZEABLE=true`、`EMBEDWIDGET=false`、`SHOWBORDER=false`。
- **无镜像字段（单源铁律）**：Live 业务属性（NDI 源名/延迟）存于工作副本的 Live 素材（`Asset.live_*`，随场景 pb 分发），实况域唯一写者是实况区（直写素材），模型只镜像 `asset_id`（与 Text 同款"无文件素材"模式）。
- **专属编辑区** `LiveClipSection`：属性面板第 3 页（`ClipPlugInterface::createPropertySection` 工厂加载）——未绑定 Live 素材返回 nullptr（面板只含通用区）；广播灌值经 `ClipSectionInterface`。
- **启用闸（2026-11-23 用户定案）**：「启用/停止」按钮（`liveEnable`）= 捕获与下发的唯一运行开关，状态存 `Asset.live_enabled`（proto `LivePayload.enabled`，随场景 pb 分发）。**启用** → editor Hub 开始采集（Ingest）+ 组播下发（MpegTsSender），display 收流显示，同时锁定接入方式/源/刷新/帧同步四控件 + `onEdited`/`onRefreshSources` 写回静默忽略（运行中切源 = 调度器指纹失效 → 连接中的源实例析构重建，接收/渲染线程竞态偶发崩溃——护栏与闸合一）；**停止** → Hub 拆净收端+发送器、MediaSource 不装配收流（占位黑屏），四控件解锁可自由切源。全链路生效点：`LiveHub::ensureAsset`（采集+转发）、`LiveMediaSource::prepare`（收流装配）、`MediaScheduler::liveFingerprint`（启停即失效重建）。存量工程/新建片段缺省 = 停止（需显式启用才有画面——新语义预期行为）。

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
