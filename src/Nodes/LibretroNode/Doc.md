# LibretroNode（GBA 专用精简版）

把 mGBA 模拟器 core 作为节点运行：输出游戏画面与音频，输入手柄按键。
Core 固定为随主程序分发的 `bin/mgba_libretro.dll`（强制 GBA，不可更换）。

## 端口

| 方向 | 端口 | 类型 | 说明 |
|------|------|------|------|
| In 0 | Active | VariableData | 非 0 启动模拟线程 |
| In 1 | Reset | VariableData | 上升沿复位游戏 |
| In 2 | Up | VariableData | 电平语义：非 0 按住 ↑ |
| In 3 | Down | VariableData | 按住 ↓ |
| In 4 | Left | VariableData | 按住 ← |
| In 5 | Right | VariableData | 按住 → |
| In 6 | A | VariableData | 按住 A |
| In 7 | B | VariableData | 按住 B |
| In 8 | L | VariableData | 按住 L |
| In 9 | R | VariableData | 按住 R |
| In 10 | Start | VariableData | 按住 Start |
| In 11 | Select | VariableData | 按住 Select |
| Out 0 | IMAGE | ImageData | 游戏画面（全局时钟 50Hz 发布最新帧） |
| Out 1 | AUDIO | AudioData | 48kHz float 立体声（core 采样率自动重采样） |
| Out 2 | Info | VariableData | core 名称/版本/fps/分辨率/帧计数/rom/error |

按键端口断连（data == null）保持现状，演出中拔线不产生状态漂移。

## ROM 流程（媒体库）

1. 面板 ROM 下拉框列出媒体库 **Game 分类**（`.gba/.gb/.gbc`）；
2. 模型按文件名在媒体库 `allFiles()` 反查全路径（兜底 `Documents/Flow/Medias`）；
3. 启动时把源 ROM 拷贝到运行缓存 **`Documents/Flow/Cache/GBARoms`**
   （同大小复用，拷贝失败回退直接读源文件）；
4. 从缓存副本加载 core —— 演出中媒体库文件变动不影响运行中的模拟器；
5. 运行中切换 ROM：自动热重启（停旧模拟线程 → 新 ROM 重新拉起）。

存档（SRAM/即时存档）目录：`bin/saves/<core 名>/`。

## 属性（OSC 可控）

- `/active`、`/volume`
- `/reset`、`/savestate`、`/loadstate`（动作型，写 true 触发）
- 手柄按位（电平语义，按住 true）：`/up /down /left /right /a /b /l /r /start /select`

## 输出分辨率说明

GBA 原生 240×160（core 固定输出，无高清选项）。节点不做放大——
锐化/放大/滤镜请接下游 shader 图像处理节点，链路更灵活。

## 面板（GBA 实机布局）

Active 勾选、ROM（媒体库选择）、Volume；肩键行 `[L]…[R]`；
左 D-pad 十字 + 右 A/B 错位；`[Select]…[Start]` 系统行；
Reset / Save State / Load State；状态行。

## 实现原理

- `LibretroCore`：LoadLibrary 加载 core → 解析 ~20 个 `retro_*` 函数指针 →
  实现前端回调（environment / video_refresh / audio_sample_batch / input_state）。
- 模拟线程按 core 报告刷新率（GBA 59.7275Hz）驱动 `retro_run`；
  video 回调把 RGB565/XRGB8888 转 BGR `cv::Mat` 暂存；
  audio 回调把 int16 立体声（mGBA 65536Hz）追加 FIFO。
- 手柄三路掩码合并：面板按键 + OSC → `m_panelMask`；
  12 个输入端口电平 → `m_portMask`；每帧 `retro_run(panel | port)`。
- 模型线程订阅全局时钟 50Hz tick：
  图像 = 最新帧 → GPU 上传 → `ImageTimestampRingQueue`；
  音频 = FIFO 线性重采样到 48kHz → 960 采样/帧 → `AudioTimestampRingQueue`
  （timestamp = clock + FIXED_DELAY_FRAMES）。
- Save/Load State 走命令化转发，在模拟线程帧间执行 `retro_serialize/unserialize`。

## 随构建部署

- `bin/mgba_libretro.dll`（固定 core，MPL-2.0 可商用，直接位于 bin/ 下）
- `meteorain.gba`（测试用 homebrew，MeteoRain，MIT）→
  拷贝到媒体库存储目录 `Documents/Flow/Medias/`，Flow 启动扫描自动入库。

## 注意事项

- libretro API 本身 MIT；mGBA core 为 MPL-2.0 可商用；
  **其他 core（Snes9x、Genesis Plus GX 等）为非商业条款，勿随软件分发**。
- core 必须与主程序同为 x64。
- 节点参数（分辨率/fps）由 core 报告，loadGame 后自动生效。
