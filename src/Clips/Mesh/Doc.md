# Mesh 片段插件（mesh.plugin）

## 定位

3D 模型片段（阶段 3A，2026-11-18 用户定案「把模型做成一种片段类型」）：
模型素材（`.obj/.glb/.gltf/.fbx/.dae/.ply/.stl`）在时间轴上的第一类公民——
拖入轨道即建片段（与 Video/Image 同链），素材经导入钩子入库
（`AssetKind::Mesh`，path=绝对路径），片段 `asset_id` 绑定该素材。

## 语义

| 维度 | 语义 |
|------|------|
| 媒体绑定 | 有（面板页 1 媒体区可导入/换绑模型文件） |
| 时长 | 无固有时长（素材 duration_us=0）——拖入保持默认长度，RESIZEABLE 按演出拖拽 |
| 画布呈现 | **隐形**（同 Audio/Control/Artnet/Mapping 口径——模型经银幕 3D 视口呈现，不作为画布内容块） |
| 动画页 | 隐藏（transform 语义不在片段上；模型 placement 在银幕侧） |
| 专属编辑区 | 无（createPropertySection 默认 nullptr） |

## 消费端（谁在用模型）

* **银幕投影表面**：屏幕属性对话框「3D mapping」页 → 3D 模型下拉
  （`setMeshChoices`）→ OK 写 `Screen.model_asset_id`；预览画布
  `drawMeshWireframe` 按 placement（offset xyz + scale）画素模线框
  （`ms::loadMesh` Assimp 解析 + Y-up→Z-up + 落地归一化，进程级缓存）。
* **3B（后续）**：display 端投影渲染管线——投影仪画面投射到模型表面
  （GL 投影纹理 + 深度测试）。

## 类型键

`CLIP_TYPE = "Mesh"`（工厂表键/场景存储/typeForKind 映射键；目录名一致）。
导入扩展名白名单三处同表：MainWindow `kMesh` / TimeLineDefines `MeshTypes`
（拖放 gate）/ Assimp MeshLoader。

## 编辑链路

拖 .glb 入轨 → `onDropMediaFile` → import_media 钩子（扩展名→Mesh）
→ `onAddClip("Mesh")`（mesh.plugin 工厂）→ `SceneClipModel::setAssetId`。
双击片段 → 属性面板三页式（通用/动画隐藏/专属无）。
