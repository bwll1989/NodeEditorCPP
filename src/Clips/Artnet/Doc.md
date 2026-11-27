# Artnet 片段

Artnet 灯光片段（时间轴片段类型，type = `"Artnet"`）——节点体系 artnetclip 的语义移植（2026-11-15，磁盘插件化）。

## 功能

- **定位**：在时间轴上回放灯光数据——片段引用一段 **DMX 视频**（宽 512 像素 × 高 = universe 数，**FFV1** 编码；灰度直读 / 旧 RGBA 取 R 通道），播放头进入片段区间（前闭后开）后逐帧解码，把每一行（universe）的 512 字节 DMX 写入 **ArtnetService 状态表**（owner = 本片段），由服务内置 **50Hz 节拍**合成广播。离开区间不清零（状态表保持最后一帧持续重发——旧口径）。
- **发送配置**：目标主机（默认广播 `192.168.1.255`，IPv4 校验）、起始域（0~65535）；subnet/net 恒 0（Timeline 简化，域地址 = `(起始域 + 序号) % 16`）。
- **模型** `ArtnetClipModel`：继承 BaseClass 的 `SceneClipModel`（μs 域）——青色块（#66CCCC）、可拖拽调长。播放驱动 = **PlaybackSendScheduler** 独立线程快照直读工作副本 `clip_data`（2026-11-22 治本；模型侧 `onPlayheadAdvanced` 推送钩子已于 2026-11-27 退役删除）；**去重键 = 视频自身帧号**（比旧版按时间轴帧去重更精确）。
- **时长自适应**：选择 DMX 视频后按视频时长自动 `setEnd`（旧 loadArtnetInfo 口径）——`lengthChanged` → 适配器几何回写链闭环写工作副本。
- **配置存储** `ms::Clip.clip_data`（pb field 13，随场景分发）：`{"file":媒体库相对路径,"targetHost":"...","startUniverse":N}`；适配器装载/面板写回两路经 `setClipPayload` 灌给模型（幂等 diff）。
- **专属编辑区** `ArtnetClipSection`：SelectorComboBox（媒体库 DMX 分类）+ 目标主机 + 起始域 + 文件探测信息（FFV1/宽高/域数/时长），属性面板第 3 页（`createPropertySection` 工厂加载）。写回 = 800ms 轮询 diff（ControlClipSection 同源策略）。

## 结构（对齐节点插件同构）

| 文件 | 职责 |
|---|---|
| `ArtnetClipModel.hpp/.cpp` | 类型专属模型：配置镜像 + 编辑期时长自适应（播放期 FFmpeg 逐帧 DMX 解码在 SendScheduler 的 DmxDecoder） |
| `ArtnetClipSection.hpp/.cpp` | DMX 配置区（属性面板第 3 页） |
| `PluginDefinition.hpp/.cpp` | 插件定义（class `Plugin` : `ClipPlugInterface`） |
| `CMakeLists.txt` | 独立 target（`plugin_artnet` → `bin/plugins/TimeLine/artnet.plugin`；另链 MediaCore/GUIElements/MediaManger/AppConfig/ArtnetService/FFmpeg 全路径 .lib） |

## 加载（磁盘驱动，零内置兜底）

`WorkScenePluginLoader` 扫描 `bin/plugins/TimeLine/*.plugin`——删 `artnet.plugin` = Artnet 类型不存在。构建/输出约定详见 `../Video/Doc.md`。

## 与节点体系（src/Nodes/TimeLineNode/artnetclipmodel）的差异

| 节点体系 | 本插件 |
|---|---|
| `currentData(currentFrame)` 拉取（节点引擎驱动） | 快照直读（PlaybackSendScheduler 独立线程；初版曾用 `onPlayheadAdvanced` 推送，已退役） |
| 时间轴帧号（制式相关）→ 秒 | μs 直算（rel_us/1e6） |
| 去重键 = 时间轴帧 | 去重键 = 视频自身帧号 |
| save()/load() 节点文档 JSON | `Clip.clip_data`（场景 pb 分发） |
| GlobalEventBus 订阅 /universe /host /file | **暂缓**（本体系属性绑定未覆盖片段业务字段） |
| "/file" "/stop" OSC 标记 | 不再产生（本体系无消费者） |

---

## 属性面板
三页式面板装配与 schema 化设计见 [`../../Common/ClipPanelBase/Doc.md`](../../Common/ClipPanelBase/Doc.md)。场景全量下发、display 端无渲染 creator（执行在 PlaybackSendScheduler 调度线程）；动画页按 schema 可见性隐藏。
