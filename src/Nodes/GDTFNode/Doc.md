# GDTF Fixture（灯具文件解析节点）

解析 **GDTF（General Device Type Format）** 灯具描述文件（`.gdtf`，DIN SPEC 15800），
提取灯具元数据、DMX 模式/通道表、几何结构与色轮/轮片信息，以 VariableData 输出供下游节点消费。

规范文档：本目录 `gdtf-spec.md`（GDTF 1.2 Draft 1）。兼容 1.0 / 1.1 / 1.2 的 description.xml。

## 使用方法

1. 把 `.gdtf` 文件放入媒体库（`Documents/Flow/Medias/Fixtures/`，即 Media Library 的 **Fixture** 分类）。
2. 拖入 GDTF Fixture 节点，在文件选择框中选择灯具文件。
3. 在「模式」下拉框中选择 DMX Mode（解析后自动填充）。
4. 设置「起始地址」（1–512），节点自动完成通道映射（见下节）。
5. 在「通道控制」区拖动各通道滑块直接设定输出值（初值 = GDTF 通道默认值）。
6. 输出口连接 **DMXUniverse** 直接使用；或连 Extract / JavaScript 读取字段。
7. 需要外部驱动通道值时，把 VariableData 向量接到**输入口**（见下节）。

## 输入端口（通道值向量）

节点有 1 个输入口（VariableData，0 号），接受**通道值向量**，按当前模式占用通道数
（footprint）自动裁切/补零后覆盖输出：

- **映射规则**：向量第 `i` 个元素 → 起始地址第 `i` 个通道（四舍五入取整，钳制 0–255）。
  如 4CH 灯具 @ 001：向量 `[255, 128, 0, 64]` → 地址 1=255、2=128、3=0、4=64。
- **自动补零**：向量长度 < footprint 时，尾部通道补 0（黑场）。
- **自动裁切**：向量长度 > footprint 时，多余元素忽略。
- **优先级**：输入向量 > 滑块手动值 > GDTF 通道默认值；输入连接期间拖动滑块
  会被下次输入更新覆盖（滑块仅作显示反馈）。
- **断开回退**：输入连线删除后自动回退到滑块手动值 / GDTF 默认值。

典型上游：Variable 节点（向量）、Expression、JavaScript、Artnet/OSC 解析节点等——
可外部编排整段灯具通道值，节点本身负责 patch 映射。

## 通道控制（值输出）

映射表中每个**已分配地址**的物理通道在「通道控制」区自动生成一个拖动数值控件
（DMXDevice 节点同款 IntDragValueWidget，0–255）：

- 16bit 通道拆两行（MSB/LSB），行标签 `属性@地址`（如 `Dimmer@7 MSB`）。
- 拖动滑块 → 输出 `addressMap` 中该地址的值实时更新（下游 DMXUniverse 立即跟随）。
- 未分配（Offset="None"）的通道不生成控件。
- 用户值随工程保存/恢复；切换模式或文件时重置为默认值；修改起始地址时随地址平移。

## 自动通道映射（Patch）

选定 GDTF 文件 + DMX 模式 + 起始地址后，节点自动把 GDTF 的相对偏移解析为
**绝对 DMX 地址**，生成映射表，无需手动 patch：

- **起始地址**：Break 1 的基址（`startAddress` 属性，1–512，OSC `/startAddress`）。
- **多断点**：Break 1 从起始地址开始，后续断点按前序断点占用顺序接续成一段连续地址
  （满足单 Universe 灯具的绝大多数场景）。
- **offset 基准自适应**：GDTF 规范未明确 Offset 起点，实际文件 0-based（`Offset="0,1"`）
  与 1-based（`Offset="1"`）并存；节点按每断点最小 offset 归零，两种基准都正确落位。
- **16bit 通道**：offsets[0]=MSB（带默认值），offsets[1]=LSB（默认 0）。
- **未分配通道**：`Offset="None"`（如 master 关联的虚拟通道）→ 地址 -1、宽度 0。
- **占用范围**：`footprint` = 末绝对地址 − 起始地址 + 1；节点界面显示 `起始-末地址` 范围。

映射结果显示在节点界面：设备图片（zip 内 thumbnail.png）、起始地址旁的占用范围
（如 `1-4`），「通道控制」区每物理地址一个数值控件（`属性@地址` 标签，16bit 为 `首-末` 两行）。

## 输出数据（VariableData）

| 键 | 内容 |
|----|------|
| `default` | 完整灯具描述 map（见下），**含 `addressMap` / `channelTable` / `startAddress` / `footprint`** |
| `channels` | 当前模式映射后通道数组（每项追加 address/addresses/width/default8bit） |
| `attributeMap` | 属性 → {address, width, addresses, dmxBreak}（按属性名取通道） |
| `meta` | {manufacturer, name, longName, shortName, fixtureTypeId, dataVersion} |
| `mode` | 当前 DMX 模式名 |

### addressMap（对接 DMXUniverse）

`default` map 内的 `addressMap`：**绝对地址（1 起，字符串键）→ 8bit 默认值（0–255）**，
与 DMXUniverse 节点 `setInData` 的消费格式完全一致，连线上电即可用：

```json
"addressMap": { "1": 255, "2": 255, "3": 255, "4": 0 }   // Mode 4CH (RGBW) @ 001
```

### default map 主要键

| 键 | 说明 |
|----|------|
| `manufacturer` / `name` / `shortName` / `longName` / `description` | 灯具元数据 |
| `fixtureTypeId` | GDTF GUID |
| `dataVersion` | 文件格式版本（如 "1.2"） |
| `mode` | 当前 DMX 模式名 |
| `dmxModes` | 所有 DMX 模式数组 |
| `startAddress` | 当前起始地址（1–512） |
| `footprint` | 当前模式占用通道数 |
| `addressMap` | 绝对地址 → 8bit 默认值（DMXUniverse 直接消费） |
| `channelTable` | 映射后通道数组（同顶层 `channels`） |
| `attributes` | 属性定义（name/pretty/physicalUnit/feature…） |
| `wheels` | 色轮/ gobos 轮与轮片（含 WheelSlotIndex） |
| `models` | 模型尺寸（Length/Width/Height，米）与原始体类型 |
| `geometries` | 递归几何树（type/name/model/position 4x4 矩阵/beam 参数） |
| `revisions` | 修订历史 |

### 通道条目结构

```json
{
  "name": "Head_Dimmer",          // Geometry_Attribute（规范命名规则）
  "attribute": "Dimmer",
  "geometry": "Head",
  "dmxBreak": 1,                  // DMX 断点号（1 起；Overwrite 表示由引用覆写）
  "offsets": [0],                 // 相对断点的偏移（0 起，高位在前；"None" → []）
  "default": {"value":255,"bits":1,"normalized":1.0},
  "highlight": {...},
  "logicalChannels": [
    { "attribute": "Dimmer", "snap": "No", "mibFade": 0,
      "functions": [
        { "name": "Dimmer", "dmxFrom": {...}, "default": {...},
          "physicalFrom": 0, "physicalTo": 1, "wheel": "",
          "sets": [ {"name":"open","dmxFrom":{...}} ] }
      ] }
  ]
}
```

- DMX 值统一解析为 `{value, bits, shifted, normalized}`：`"255/1"` → 8bit 255 → 归一化 1.0；`"65535/2"` → 16bit 满量程。

## 节点属性（OSC 外部控制）

| 属性 | OSC 地址 | 说明 |
|------|----------|------|
| file | `/file` | .gdtf 文件（媒体库相对路径） |
| dmxMode | `/mode` | DMX 模式名 |
| startAddress | `/startAddress` | DMX 起始地址（1–512，默认 1） |
| （行值） | `/ch/{行号}` | 按"通道控制"行序号（0 起）设置该通道输出值 |

## 技术说明

- zip 解包：minizip（zlib-1.2.7 contrib）；XML 解析：tinyxml2。
- 三方源码来自 `3rdParty/Scalable Display Reference Implementation/ThirdParty`（zlib-1.2.7 / contrib/minizip / tinyxml2），编入静态库 `gdtf_thirdparty`，与 MpcdiNode 相同模式。
- 缩略图：自动读取 zip 内 `thumbnail.png` 显示在节点界面。
