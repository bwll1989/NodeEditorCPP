# Mapping 片段（mapping.plugin）

## 定位
- **播放期映射器**：片段携带一张 OSC 消息表（地址 + 类型 + 表达式），播放头在片段区间内的**每一帧**对表达式求值并逐条重发——把时间轴进度映射到任意 OSC 地址（外部状态、控件联动、下游节点等）。
- 无媒体文件、无素材绑定（`asset_id` 恒空）、无画布呈现（与 Control 同口径）；只在时间轴中存在，玫红块 `#B83B5E`，可拖拽调长。
- **移植出处**：节点体系 `src/Nodes/TimeLineNode/mappingclipmodel.hpp`（`MappingClipModel`，帧域 + currentData 拉取驱动 + 控件内 QJSEngine 求值）→ 本插件（SceneClipModel μs 域 + `onPlayheadAdvanced` 推送驱动 + 模型自带 QJSEngine）。

## 数据与持久化
- 业务数据存 **`ms::Clip.clip_data`（pb field 13）**：`{"messages":[{host,port,address,type,value},...]}`——OSCMessageListWidget save/load 兼容格式（onlyInternal 卡片 host/port 不参与发送）。
- **持久化即表达式文本**：`OSCMessageListWidget::save()` 把求值结果覆写为 `getExpression()` 原文，求值只在发送时发生——模型无控件也能完整求值（自建单份 `QJSEngine`）。
- 灌注两路：装载 `loadSceneToModel`（`setEnd` 前 push）+ 面板写回（`on_edited` 闭包 → `findClipModelByAnchor` → `setClipPayload`）；模型侧幂等（原文 diff）。
- 默认时长 = 100 帧（旧版 ctor `m_end = m_start + 100` 帧域口径，μs 化 = 100×kFrameUs = 4s）。

## 编辑（专属页）
- `MappingClipSection`（页3 类型专属）：提示标签 + OSCMessageListWidget（onlyInternal，增删/编辑/右键/拖拽全内置）；**无测试发送按钮**（节点体系同款——表达式依赖 `$percent`/`$frame`，离开播放上下文无意义）。
- 写回：800ms 轮询 diff（仅可见时）→ 直写 `clip->clip_data` + `ctx.on_edited` 落定；`refreshFromClip` 与轮询共用 diff 快照防回环；`boundClip()` 优先 `ctx.clip_lookup`（vector 搬移安全）。

## 回放语义（`onPlayheadAdvanced`）
- 区间判定 **`[start, end]` 双闭**（旧 `currentFrame >= m_start && <= m_end` 口径；Control/Artnet 为前闭后开——此处刻意保真旧语义，end 帧上 `$percent` 恰为 1.0）。
- **去重键 = 绝对帧号** `usToFrames(now)`（旧 `m_currentFrame` 判等同口径；50Hz 泵 vs 25fps 帧域，同一帧多拍只发一次）。payload 变更复位 -1（编辑后下一拍重新触发）。
- 每拍注入引擎全局：`$percent` = `(now-start)/length()`（区间内归一化进度）、`$frame` = `usToFrames(now-start)`（区间内相对帧号）；PI/E 在 ctor 注入。
- 表达式求值 = `OSCMessageItemWidget::getMessage` 逐行复刻（保真权威出处）：
  - 值以 `'['` 开头 → `type="List"`，`with(Math) { (expr) }` 求值取数组（引擎报错/非数组回退 QJsonDocument 直解，再不成 = 空列表）；
  - `type=="Int"`/`"Float"` → `with(Math) { expr }`（报错回退 0 / 0.0）；
  - 其余 → `type="String"`，表达式原文直发（不修剪）。
- 发送 = `StatusContainer::instance()->parseOSC(m)`（与旧 `trigger()` 同一入口：入状态容器 → statusUpdated → WS 8992 广播 + GlobalEventBus 控件联动）。

## 与节点体系差异对照
| 维度 | 节点体系（TimeLineNode） | 本插件（Clips/Mapping） |
|---|---|---|
| 基类 | NodeClipDelegateModel（帧域） | SceneClipModel（μs 域，BaseClass.dll） |
| 驱动 | currentData(currentFrame) 拉取 | onPlayheadAdvanced 推送（适配器两落点） |
| 求值引擎 | 每卡片一份（控件生命周期） | 模型单份（与控件解耦；求值公式逐行同源） |
| 载荷 | save()/load() QJsonObject | ms::Clip.clip_data（pb field 13） |
| 时间 | 帧域（[m_start, m_end] 双闭） | μs 域（[start, end] 双闭——保真旧口径） |
| 属性面板 | 内嵌 clipPropertyWidget | ClipPlugInterface 工厂 + ClipSectionContext |
| 时长 | m_start+100（帧） | start+100×kFrameUs |

## 遗留 / 暂缓
- **GlobalEventBus 外部控制**（旧 `afterModelReady` 订阅 `/start_frame` `/end_frame` `/position_frame`）：本体系属性绑定体系未覆盖片段业务字段——暂缓，与 Artnet 的 /universe /host /file 订阅同批处理。
- host/port 字段仅随 save() 往返持久化，发送不参与（本地状态容器口径，旧版一致）。

---

## 属性面板
三页式面板装配与 schema 化设计见 [`../../Common/ClipPanelBase/Doc.md`](../../Common/ClipPanelBase/Doc.md)。场景全量下发、display 端无渲染 creator（执行在宿主 onPlayheadAdvanced）；动画页含映射插值域（P4 schema 化）。
