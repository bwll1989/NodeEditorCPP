# UniversePlaybackNode

## 1. 节点说明

从媒体库中的 **FFV1** 视频文件回放 DMX 数据：视频须为宽 512 像素、高为 Universe 路数；每行 512 字节对应一路 Universe 的 512 通道。支持播放、循环、停止、清空，并可输出多路 Universe 数据包（与 DMX Universe 格式兼容）。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| PLAY | VariableData | `true` 开始播放，`false` 暂停 |
| LOOP | VariableData | 是否循环播放 |
| STOP | VariableData | `true` 时停止 |
| CLEAR | VariableData | `true` 时清空所有 DMX 数据 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| UNIVERSE1 … UNIVERSEn | VariableData | 各 Universe 的 Art-Net 数据包；数量由视频高度决定，默认 4 个端口可编辑 |

## 3. 界面说明

- **Universe / Subnet / Net**：起始 Universe 编号；多路时按索引递增 Universe（模 16）。
- **文件**：从媒体库选择 FFV1 视频（512×N）。
- **播放 / 循环**：控制回放。
- **清空**：清零 DMX 缓冲。
- **当前时间**：显示回放进度。

外部控制：`/universe`、`/subnet`、`/net`、`/play`、`/loop`、`/clear`、`/filename`。

## 4. 使用说明

1. 准备 FFV1、分辨率 512×Universe 数量的视频，放入媒体库。
2. 选择文件后，输出端口数量随视频高度自动调整。
3. 各 UNIVERSE 输出可接 **Artnet Out**。
4. 也可用 PLAY / STOP 等输入端口做远程触发。

## 5. 示例

预录灯光秀视频 → 本节点播放 → UNIVERSE1～4 分别接 4 个 Artnet Out 端口或合并后发送。
