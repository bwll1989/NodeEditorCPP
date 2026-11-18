# ClipPanelBase — 片段属性面板体系设计与迁移路线

> 定稿：2026-11-17（与用户逐轮收敛后的绿地方案 + 绞杀者迁移路线）。
> 本文是全项目片段属性面板的**设计权威文档**：现状装配链、问题诊断、目标架构、
> 各片段类型影响面、分阶段迁移路线。新增片段插件时，"专属页怎么写" 以此为依据。

---

## 1. 现状装配链（P3 完成后）

属性面板 = `ClipPropertyPanel`（直接继承 QTabWidget，**嵌套 QTabWidget 会导致
qobject_cast 判型失败，此为硬约束**）。三页式：

```
QtTimelineModelAdapter::openClipPropertyPanel(clip_id)
 │
 ├─ 页1「通用」 ClipPropertyPanel 自身——【P3 装配收拢】构造自治装配三区：
 │      时间属性（ClipTimePanel，OSC 外控登记随其构造自治）
 │      → 片段属性（ClipAppearancePanel）→ 媒体文件区
 │      （构造参数 owner_model 注入，装配点与页结构同库——原 P2 时代的
 │        ClipPageHost 接口随迁库退役，BaseClass 回归零 UI 纯模型/接口库，
 │        initPropertyWidget 瘦身为「壳 + 工厂 + 嵌入」）
 ├─ 页2「动画」 TweenSection（全类型恒有；七属性唯一编辑区）
 └─ 页3「<插件声明 clipDisplayName>」 插件工厂 createPropertySection(ctx)
      （宿主零类型知识：按片段类型查 WorkScenePluginLoader 插件表；
        工厂返回 nullptr = 页不加，用户定案「没有的话就不嵌入」；
        页签名/媒体区显隐/绑定行文案 = 插件元数据，P2-B 下放；
        【P3-B】ctx 协议类型化 = ms::ClipSectionContext*（BaseClass 层
        前置声明 + 插件/宿主侧 include 完整定义，static_cast 还原退役））
```

三个注册点的区块全部登记进 adapter 的**统一广播注册表** `panel_refs_`
（P1-A1 三表合一：原 `clip_panels_` / `tween_panels_` / `clip_sections_`）：

```cpp
struct PanelRef {
    enum class Kind { Generic, Tween, Section };
    QPointer<QWidget> w;   // 惰性清理：区块随宿主面板关闭销毁
    Kind              kind;
};
std::unordered_map<std::string, std::vector<PanelRef>> panel_refs_;
```

**写回编排**（P1-A2 收口）：所有编辑落定路径统一走
`notifyClipEdited(clip_id, mirror_payload)`：

```
applyClipPanelMerge（通用页写回）        → notifyClipEdited(id)          // 补齐原缺的广播腿
applyClipTweenEdit（动画页写回）         → notifyClipEdited(id)
applyTweenKeyRemove / KeyFrameMove      → notifyClipEdited(id)
专属区 ctx.on_edited（片段插件落定）     → notifyClipEdited(id, true)    // + 模型镜像回推
```

`notifyClipEdited` 三腿编排：`mirror_payload`（可选：工作副本 clip_data →
模型 setClipPayload 镜像回推）→ `emit sceneEdited()`（未下发提示 + 自动下发链）
→ `refreshClipPanels(clip_id)`（统一注册表单次遍历 + Kind 分派广播灌值）。

## 2. 问题诊断（为什么改）

| # | 问题 | 根因 | 违反的原则 |
|---|------|------|-----------|
| ① | 所有权倒置 | BaseClass 建对话框壳、adapter 建页容器，壳结构靠 `qobject_cast` 双重鸭子类型识别（原 AbstractClipModel.cpp:82-90），任一失配**静默降级** | 单一所有者；显式协议 |（✅ P2-A 接口化终结 → **P3 装配同库收拢**（接口亦退役——装配点与页结构同库，无需协议））
| ② | 宿主类型知识 | adapter/面板层散布 5 张类型映射表 + if 链（类型中文名、图标、特判等） | 轨道去类型化定案（11-15 commit 6266c2de）：类型跟片段走，容器零类型知识 |（面板相关三表已下放 P2-B；轨道名/图标仍留宿主，见 §7 遗留）
| ③ | 写回编排不对称 | 三张同构注册表 + 四条写回链两长两短（applyClipPanelMerge 尾缺广播腿，靠 refreshSceneViews 间接链兜底） | 机制性同腿；单一事实 |
| ④ | 死重 | ClipPropertyPanel::snapshot_ 构造后零读取（P1-A3 已删） | — |

## 3. 目标架构（数据驱动界面，P4 终态）

核心思想：**schema 驱动 UI** —— 片段声明"我有哪些字段"（数据），框架负责
"怎么摆、怎么编、何时刷新"（呈现）。

```
┌─────────────────────────────────────────────────────┐
│ Inspector（通用属性检查器，一次性实现）                  │
│  输入 = FieldDef 列表 + ClipStore 指针                 │
│  行为 = 摆控件 / 灌值 / 焦点保护 / 脏标记 / 写回            │
├─────────────────────────────────────────────────────┤
│ ClipSchema（字段描述符）                                │
│  FieldDef{ key, 类型(int/double/string/enum/bool/vec3),│
│            标签, 单位, 范围, 默认值, 页归属, 可见性分级 }   │
├─────────────────────────────────────────────────────┤
│ ClipStore（读写唯一通道）                               │
│  get(path, t_us)  → 播放头处插值终值（WATCHOUT 语义）      │
│  set(path, value) → 写入 + auto-key 判定 + dataChanged  │
├─────────────────────────────────────────────────────┤
│ 数据事实：Clip（clip_data JSON 自描述 + tweens/transform）│
└─────────────────────────────────────────────────────┘
```

- **片段插件作者**只写 FieldDef 表（声明式），不再手写 Section UI；
  真正领域专属的控件（如 Live 预览、Text 富编辑）仍可走自定义 field editor
  注册进 Inspector。
- 可见性分级在 schema 层表达：例如 Audio 片段动画页只暴露「音频增益」一条
  tween 通道，其余变换字段 `visible=false` 直接不生成控件（用户确认的诉求：
  不是隐藏而是**按类型声明暴露面**）。
- 现成雏形：TweenSection.cpp 的 `kPropOrder[13]` PropDef 表
  （key/标签/单位/步进/通道映射）已经是 schema 原型，P4 即把它泛化。

## 4. 界面形态与可见性分级（设计定稿）

- **形态不变**：QTabWidget 三页式（通用 / 动画 / 专属）。页序与页名维持现状，
  用户习惯零迁移成本。
- **页2 动画页**：全类型按插件声明的暴露面 schema（【P4-A 已实施 2026-11-17】
  `ClipPlugInterface::tweenExposure()` → `ClipTweenExposure{page_visible,
  exposed_keys}`，字符串 key 白名单对齐 P4 终态 FieldDef.key）：
  - Video / Image / Text / Mapping：空白名单 = 全暴露 13 行（现状零改动；
    类型特有通道如 Mapping 的映射插值域待后续切片）；
  - Audio：仅「音频增益」（`{"audio_gain"}`）；变换区**不生成**（非置灰，
    Row 骨架保留定长契约、控件 nullptr，锚点/空组随之隐藏）；
  - Control / Artnet：`page_visible=false` → **整页隐藏**（实现期二选一
    已定案：默认隐藏；"无可用动画通道"占位说明退役为备选）。
- **页3 专属页**：schema 未覆盖的领域字段继续走插件工厂
  （【P3 已实施】协议已类型化：`createPropertySection(ms::ClipSectionContext* ctx, QWidget*)`）。

## 5. 类型影响面（哪些片段要动）

| 类型 | 属性面板改动 | 下发链路 | 说明 |
|------|------------|---------|------|
| Video / Image / Audio | **零改动**（P1-P4 全程） | 已全量下发，display 有渲染 creator | 三页 = 通用 + 动画 +（Audio 无专属页） |
| Text | 专属区迁库（P3） | 已全量下发，有 creator | 文本区随 text.plugin |
| Live | 专属区迁库（P3） | 已全量下发，有 creator | 实况区随 live.plugin |
| Control / Artnet | 专属区迁库（P3）+ 动画页按可见性隐藏 | **已全量下发但 display 端忽略**（无 render creator） | 执行在宿主侧 onPlayheadAdvanced，行为正确 |
| Mapping | 同上 | 同上 | 同上 |

> 已核实（11-17）：下发链路 `publishScene → EditorRuntime::cmdPublishScene →
> SceneLoader::serialize 全量序列化 → 广播`，所有类型片段无差别下发；
> display 渲染工厂（RenderThread.cpp:111-135）只注册 Video/Image/Text/Live
> creator，Control/Artnet/Mapping 到达后仅被忽略——**不是链路缺陷**，
> 是"执行在宿主、display 不渲染"的设计结果。

## 6. 迁移路线（绞杀者模式，每期独立可交付）

| 阶段 | 内容 | ABI 影响 | 状态 |
|------|------|---------|------|
| **P1 收编** | A1 三张注册表合一 `panel_refs_`；A2 写回编排收口 `notifyClipEdited`（补齐 applyClipPanelMerge 缺失的广播腿）；A3 死重清理（snapshot_） | **零**（不动 BaseClass 头，只重编 ClipPanelBase + 主程序适配器所在 target） | ✅ 2026-11-17 已实施 |
| **P2 装配收拢** | A: 壳协议接口化——BaseClass 新增 `ClipPageHost` 接口（insertHostWidgets），ClipPropertyPanel 实现之（页 1 自治装配），initPropertyWidget 双 duck-typing 退役为单次接口识别（退化平铺保留）；B: 类型元数据下放——ClipPlugInterface 追加 clipDisplayName/hasMediaBinding/bindingHint 三个可选虚，8 插件声明，宿主 kindPageName 漂移表（Artnet/Mapping 缺失落兜底，实证）与媒体区 if 链删除 | BaseClass 头变更 → **Clips 全量重编**（构建铁律，已执行） | ✅ 2026-11-17 已实施 |
| **P3 面板迁库** | ClipPropertyPanel / TweenSection 收拢为唯一装配中心；BaseClass 内 UI 类（ClipTimePanel/ClipAppearancePanel）迁入 ClipPanelBase，BaseClass 回归零 UI 纯模型/接口库；`void* ctx` 换类型化协议（BaseClass 层前置声明 + MediaCore 侧实现） | 接口协议变更，插件 createPropertySection/refreshFromClip 签名升级 | ✅ 2026-11-17 已实施（见 §9） |
| **P4 schema 化** | ClipStore / ClipSchema / Inspector 三件套落地；TweenSection kPropOrder 泛化；各片段专属区逐步换 FieldDef 声明（可按片段逐个迁移，长期共存）。**P4-A 动画页暴露面 schema 化已实施**（见 §10）：ClipTweenExposure 声明 + TweenSection 白名单过滤。**P4-B schema 事实源已落地**（BaseClass `ClipSchema.h`，见 §11）；**P4-D TweenSection 泛化已实施**（kPropOrder 瘦身为通道映射+测试锚，声明性数据改读 schema，见 §11）；**P4-C ClipStore 已落地**（key 寻址 get/set WATCHOUT 双模直写，见 §11）；**P4-E ClipInspector 已落地**（FieldDef 列表+ClipStore → 声明式行渲染，见 §11） | P4-A：BaseClass 头 vtable 尾部追加 → Clips 全量重编（已执行）；P4-B：纯新增头零 ABI 影响；P4-C/E：ClipPanelBase 新增文件零既有 ABI 影响 | P4-A ✅ 2026-11-17；P4-B/C/D/E ✅ 2026-11-18（见 §11）；片段专属区逐个迁移待办 |

### 负范围（明确不做）

- **下发投影不在本期范围**（用户定案 2026-11-17）：display 端投影渲染不实现。
  留档理由：下发链路已全量广播场景数据（见 §5），未来 display 端做投影时
  只需在渲染工厂注册对应 creator，编辑器侧零改动；当前投影相关需求无排期。

## 7. P1 实施记录（2026-11-17）

- `QtTimelineModelAdapter.hpp/.cpp`：`PanelRef{Kind, QPointer}` + `panel_refs_`
  统一注册表；`notifyClipEdited(clip_id, mirror_payload)` 收口四条写回链；
  `refreshClipPanels` / `refreshTweenPanelsPlayhead` 改为单表遍历 + Kind 分派。
- `ClipPropertyPanel.hpp/.cpp`：删除 `snapshot_` 死重成员。
- 行为变化：仅 applyClipPanelMerge 尾部**新增**了此前缺失的面板广播腿
  （补齐不对称，语义为修复）；其余路径行为逐条等价。

## 8. P2 实施记录（2026-11-17，全链重编验证通过）

**P2-A 壳协议接口化（消灭双 duck-typing）**
- 新增 `src/Common/BaseClass/ClipPageHost.h`：页容器装配接口
  `insertHostWidgets(const QList<QWidget*>&)` + Q_DECLARE_INTERFACE
  （接口在下游 BaseClass，实现方 ClipPanelBase 链接本库，无循环依赖；
  经 Qt 元对象系统跨 DLL 识别）。
- `ClipPropertyPanel` 多继承实现（Q_INTERFACES(ClipPageHost)）：页 1 开头
  按序插入宿主通用区（时间属性 → 片段属性 → 媒体文件区，插入位与原版
  一致）。
- `AbstractClipModel::initPropertyWidget`：`qobject_cast<ClipPageHost*>`
  单次识别替代 `qobject_cast<QTabWidget*>` + `qobject_cast<QBoxLayout*>`
  双重探测；未实现接口（旧式非 Tab 工厂产物）= 平铺进对话框，行为不变。

**P2-B 类型元数据下放**
- `ClipPlugInterface` 尾部追加三个可选虚（默认实现保底，vtable 追加即
  全量重编原因）：
  - `clipDisplayName()`——页 3 页签名（默认 = clipType() 英文兜底）；
  - `hasMediaBinding()`——页 1 媒体区显隐（默认 true）；
  - `bindingHint()`——无媒体类型的绑定行说明（默认空 = 不覆盖）。
- 8 插件声明：Video"视频"/Image"图片"/Audio"音频"/Text"文本"/Live"实况"/
  Control"指令"/Artnet"灯光"/Mapping"映射"；Text/Live/Control/Artnet/
  Mapping 声明 hasMediaBinding=false；Control/Artnet/Mapping 迁移原宿主
  硬编码文案。
- 宿主删除：`kindPageName` 漂移表（**Artnet/Mapping 缺失落兜底「属性」
  实证**——类型显示知识留宿主必漂移）；媒体区 AssetKind if 链；makeClipPropertyPanel
  插件查询前置后页 3 复用同一 factory。
- 保留宿主：AssetKind 映射（媒体库域知识）、Live 动态实况源名（读
  Asset.live_source_name）、轨道名 kindDisplayName（视图域，TimeLineBase
  菜单侧未动——遗留到 P3/视图改造）。

**验证**：BaseClass → 8 插件（plugin_*）→ ClipPanelBase → Flow 全链
重编零错误（Text 插件曾因编辑误删 createModel 声明报 C2509，补回后过）；
用户手动编译运行确认。BaseClass 头变更铁律已履行。

> 【P3 后记】P2-A 诞生的 ClipPageHost 接口在 P3 装配同库收拢后退役删除
> （见 §9）——接口协议是跨库装配的过渡方案，同库后无需协商。

## 9. P3 实施记录（2026-11-17，全链重编验证通过）

**P3-B ctx 协议类型化（先做，最小面）**
- `AbstractClipInterface.h`：头部前置声明 `namespace ms { struct
  ClipSectionContext; struct Clip; }`（BaseClass 不依赖 MediaCore 铁律保持
  ——签名用指针，不完整类型合法；class-key 与 MediaCore 侧 struct 定义
  一致）。两签名升级：
  - `createPropertySection(void* ctx, QWidget*)` →
    `createPropertySection(ms::ClipSectionContext* ctx, QWidget*)`；
  - `ClipSectionInterface::refreshFromClip(const void*)` →
    `(const ms::Clip*)`。
- 5 个有专属区的插件（Text/Live/Control/Artnet/Mapping）签名同步升级 +
  实现/工厂 `static_cast` 还原全部删除（ctx 直用成员）；
  适配器调用点零改动（传的本来就是类型化指针）。
- ABI 注意：指针类型变更即 vtable 槽签名变更（名字修饰不同）——旧插件
  与新接口混装 = 工厂解析失败，全量重编已履行。

**P3-A 面板迁库（BaseClass → ClipPanelBase）**
- `git mv` 四文件（ClipTimePanel/.hpp+.cpp、ClipAppearancePanel/.hpp+.cpp）
  → `src/Common/ClipPanelBase/`；导出宏 BASECLASS_EXPORT →
  CLIPPANELBASE_EXPORT。
- **OSC 外控登记随面板自治**：原 AbstractClipModel::initPropertyWidget
  尾部 6× registerExternalControl + pushOscInitialState 块收编入
  ClipTimePanel 构造尾（面板在哪装配，登记就在哪——独立打开路径
  showClipOwnPanel 不再漏登记）；`pushOscInitialState` 升 public
  （原 protected——面板库需跨 DLL 调用；幂等，setId 侧触发点不变）。
- **装配中心收拢**：`ClipPropertyPanel` 构造追加 `AbstractClipModel*
  owner_model = nullptr` 参数，非空即在页 1 顶部自治装配通用区
  （时间属性 → 片段属性 → 媒体文件区，落位与 P2 时代接口插入一致）；
  适配器 makeClipPropertyPanel 传 `findClipModelByAnchor(clip_id)`。
  **ClipPageHost 接口删除**（P2 诞生、P3 退役——装配点与页结构同库后
  无需跨库协商协议）。
- `AbstractClipModel::initPropertyWidget` 瘦身为「壳 + 工厂 + 嵌入」：
  FramelessDialog 壳（置顶语义保留）→ 工厂闭包 → 布局嵌入；删除
  contentWidget 中转层、6 控件 OSC 注册块、timeGroupBox/m_layout 成员
  与面板前置声明——BaseClass 回归零 UI 纯模型/接口库。
- CMakeLists：BaseClass 源列表删 4 面板文件 + ClipPageHost.h；
  ClipPanelBase 收编 4 面板文件（依赖已齐：GUIElements 传播
  IntDragValueWidget、BaseClass 传播 src 根/TimeLineDefines/AppConfig
  include——零新增链接）。

**行为变化（设计接受，均已留痕代码注释）**
1. 独立打开路径（showClipOwnPanel，测试/程序化入口）的面板现在**含**
   时间属性区（原仅媒体区）——与组合面板同构化，语义为补齐；
2. 退化路径（工厂未返回产物：hooks 未注入/工作副本缺片段）原平铺库
   通用面板兜底——BaseClass 零 UI 后无兜底，对话框空壳 + qWarning 留痕；
3. OSC 外控登记时机从「面板构造后由基类注册」变为「ClipTimePanel 构造
   内自治注册」——StatusContainer 登记/推送内容与顺序不变（pushOscInitialState
   幂等）。

**验证**：BaseClass → 8 插件 → ClipPanelBase → Flow 全链重编零错误
（84 步 + 18 步全绿）；节点链 TimelineEngine.* 的同名冻结副本
（成员名 m_startFrameSpinBox 等）与本次迁移零关联，未触碰。

## 10. P4-A 实施记录：动画页暴露面 schema 化（2026-11-17，全链重编验证通过）

> P4 绞杀路线第一刀：先让「动画页暴露什么」由插件声明驱动（schema 的
> 露出端），ClipStore/Inspector 三件套（schema 的存储端）待后续切片。
> key 词汇从第一天起对齐 P4 终态 FieldDef.key——后续迁移无词汇转换。

**接口（BaseClass，vtable 尾部追加）**
- `AbstractClipInterface.h`：新增 `struct ClipTweenExposure{ bool
  page_visible = true; QStringList exposed_keys; }` + 可选虚
  `tweenExposure()`（默认 `{}` = 全暴露 13 行，未声明插件的保守兜底）。
  **字符串 key 白名单而非 ms::TweenProp 枚举**——plugin_audio 只链
  BaseClass+Qt 不链 MediaCore，枚举不可达；key 词汇：position/size/
  rotation/crop_top/crop_bottom/crop_left/crop_right/corner_lt/corner_rt/
  corner_rb/corner_lb/opacity/audio_gain（未知 key 宿主 qWarning 漂移告警）。

**TweenSection 白名单过滤（不生成语义，用户定案 2026-11-17）**
- 构造签名 `TweenSection(const Clip&, const ClipTweenExposure&, QWidget*
  = nullptr)`；`row_exposed_[13]` 暴露位（空白名单 = 全暴露；
  page_visible=false 防御性同义化全不暴露——适配器已在构造前整页拦截）。
- **不生成而非隐藏**：未暴露行只推 Row 骨架（prop/name/components 留存、
  控件全 nullptr），`propCount()=13`/`valueSpin()`/`cornerSpin()`/`rows_`
  定长契约不变——测试遍历得 nullptr 而非越界。
- 网格行号运行计数收紧（无空洞行）；空组（组内零暴露行）标题+容器
  `setVisible(false)`（标题钮仍创建——grp_btn_ 定长契约与折叠联动
  connect 不感知可见性）；锚点属变换域随组 0 同进退（组 0 零暴露行则
  锚点不生成，anchor_combo_ = nullptr，refreshFrom 已有判空）。
- `recordKeyframe` 新增 `!r.value[0]` 护栏（公开 API 可直达未暴露行）；
  其余读侧护栏既有（refreshFrom :990/:1000、updateRowState :895）；
  applyTo 静态脏位驱动，退役行永不置位，写回零污染。

**插件声明（3 个，其余 5 个零改动全暴露）**
- Audio：`{true, {"audio_gain"}}`——动画页仅音频增益行（用户定案：
  画布变换/合成域属性对纯音频无意义）；
- Control / Artnet：`{false, {}}`——动画页整页不加（§4「实现期二选一」
  定案为默认隐藏；占位说明版退役为备选）。

**适配器闸门（makeClipPropertyPanel）**
- `factory ? factory->tweenExposure() : ClipTweenExposure{}` +
  `if (expo.page_visible)` 整页闸——Control/Artnet 不构造 TweenSection、
  不占页签、不进 panel_refs_；factory 缺失（插件加载失败）= 全暴露
  保守兜底，不因插件丢失而丧失动画能力。

**验证**：BaseClass 头变更铁律履行——8 插件 + ClipPanelBase + Flow
全链重编零错误（52 targets）。行为矩阵：Video/Image/Text/Live/Mapping
逐像素等价（空白名单全暴露）；Audio 动画页只剩音频增益一行（合成组，
其余四组隐藏）；Control/Artnet 无动画页。

## 11. P4-B/C/D/E 实施记录：三件套 + TweenSection 泛化（2026-11-18，全链重编验证通过）

**P4-B `src/Common/BaseClass/ClipSchema.h`（纯新增头，零 ABI 影响）**
- `ms::FieldType` 枚举（Double/Int/Enum/Bool/String/Color——词汇对齐
  Doc.md §3 终态；vec3 用 Double+components=3 表达）+ 折叠组常量
  `kFieldGroupXform/Pose/Crop/Corner/Comp`（v3 五组序号）。
- `ms::FieldDef`（声明式属性行，16 字段）：key（协议寻址词汇，跨 DLL
  字符串）/label/type/components/comp_names[3]/vmin/vmax/step/decimals/
  unit/default_value[3]/resettable/tweenable/group/page/visible（尾部
  追加，聚合初始化缺省零破坏）。**数据事实的唯一事实源**——插件作者
  今后只写 FieldDef 表。
- `ms::kTweenSchema[13]` inline constexpr tween 行表（数据自
  TweenSection kPropOrder 原型逐字段迁入，行序=UI 行序=prop_index）；
  `findTweenField(const char*|QString)` + `tweenFieldAt(row)` 查找。
  原 kPropOrder 行注释的语义定案（位置行 UI 两分量三分量数据契约、
  尺寸重置=素材原始尺寸 hint、旋转透视轴 ±89° 控件层特判、裁切行序
  上下左右、角点偏移量语义、音频增益 [0,2]）全部随迁为表注释。
- **BaseClass 不引 TweenProp**（铁律保持）：schema 纯数据零 MediaCore
  依赖；inline constexpr 每 TU 一份副本（trivially copyable 只读数据，
  无 ODR 风险），零导出符号零 .cpp。
- CMakeLists：ClipSchema.h 入 BaseClass_sources（BaseClass.dll 内容
  不变——纯头无消费者时零 ABI 影响）。

**P4-D TweenSection 泛化（kPropOrder 瘦身，两表契约锁定）**
- `PropDef` 12 字段 → **3 字段** `{prop, key, obj_suffix}`：保留的
  全是【本地知识】——TweenProp 通道映射（MediaCore 域，BaseClass schema
  不可达）、schema join 键、objectName 测试锚后缀（UI 域词汇）。
  显示名/值域/步长/decimals/后缀/分量名/默认值/resettable 消费点
  全部改经 `tweenFieldAt(i)` 读 schema。
- `propDefault` 表退役 → 重置路径读 `fd->default_value[c]`（默认值
  唯一事实同步收敛；opacity/audio_gain 默认 1.0 已在 schema 行内）。
- **两表漂移护栏**：`static_assert(schemaKeysAligned())`——constexpr
  逐字符比较 kPropOrder 与 kTweenSchema 的 13 行 key（自写循环，
  qstrcmp 非 constexpr）；行下标同时索引两表的隐式契约升级为编译期
  断言。
- 行为等价性：13 行行序/值域/步长/decimals/后缀/分量名/默认值逐项
  与原表一致（数据迁移非重构）；objectName（tween<suffix><分量> 测试
  锚）与 tooltip 全不变。

**P4-C `src/Common/ClipPanelBase/ClipStore.hpp/.cpp`（key 寻址存储端）**
- 构造收 `ClipSectionContext`（闭包**值拷贝**——适配器栈上 ctx 可先于
  store 销毁）+ PlayheadFn；`locateClip()` 每现查（vector 搬移悬垂
  防御纪律）。
- **get(key, comp, t_us=-1)** = 播放头处 WATCHOUT 终值——tween 域走
  `TweenEngine::resolveTransform` 整条解算（通道覆写+静态基准，与
  渲染端恒同口径），audio_gain 走 `resolveGain`（钳 [0,2]）；静态扩展
  anchor/depth_z/stacking 直读 Clip/Transform 字段。path 重载：
  `"key"` / `"key.分量名"`（schema comp_names）/ `"key.2"` 数字。
- **set(key, comp, value)** = 双模直写：通道存在 → auto-key 打包写键
  （编辑分量新值 + 其余分量播放头插值终值——与画布拖拽/值框同构；
  Position v2 携当前基准层级防老工程打零）；无通道 → 静态基准分量写
  （audio_gain 基准在 Clip 级；stacking 写入钳 [0,1]）。成功后
  `on_edited`（宿主 notifyClipEdited 三腿编排，与插件专属区同构）。
- **通道生命周期**：`hasChannel`（菱形三态后端）/ `toggleKey`（无通道
  建通道打首键；命中 ±20ms 删键、删空连通道删 R8 纪律；未命中打键
  ——TweenSection onDiamondClicked 同款）/ `removeChannel`（⟲ 重置的
  动画侧）。key→TweenProp 映射表落本库（kTweenKeys[13]，BaseClass
  schema 不引 TweenProp 边界保持）。

**P4-E `src/Common/ClipPanelBase/ClipInspector.hpp/.cpp`（通用渲染器）**
- 输入 = `std::vector<FieldDef>`（值语义）+ `setStore(ClipStore*)`——
  渲染端零类型知识。行 = 标签 + components×FloatDragValueWidget；
  type=Int 走 decimals=0；Enum/Bool/String/Color 第一版 qWarning 留痕
  （待消费者需要时分派专用控件）；visible=false 整行不生成。
- 灌值 `refresh()` = QSignalBlocker + 焦点保护（正在编辑分量跳过，
  TweenSection 同纪律）；写回 = valueChanged → `ClipStore::set`（双模
  直写 + 落定在 store 内收口——渲染端零编排知识）。objectName =
  `insp<key><分量名>` 测试锚。
- 首消费者待接入：片段专属区 FieldDef 化迁移（Text/Live/Control/
  Artnet/Mapping 逐插件，长期共存不强制）；TweenSection 换底后续评估。

**验证**：BaseClass → ClipPanelBase → Flow 全链重编零错误
（中间 LNK1104 ×2 均系 Flow.exe 进程占用 DLL，杀进程后过；修复：
ClipStore.cpp 成员名笔误 playhead_fn_→playhead_、ClipInspector.cpp
重复 onEdited 定义与漏 include、FieldDef 补尾部 visible 字段）。
片段专属区换 FieldDef 后续逐插件迁移——长期共存不强制（绞杀者模式）。

