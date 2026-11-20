# FaustNode

用户加载 `.dsp` 文件，用 Faust 语言自定义音频处理算法。

## 功能（2026-11-20，含音频端口）

- 内嵌 UI：文件路径行 + `...` 选择按钮 + `↻` 重载按钮 + 动态参数面板 + 状态标签
- 加载 `.dsp` → libfaust JIT（`createDSPFactoryFromString`，LLVM 后端）编译出 `llvm_dsp` 实例
- `dsp->buildUserInterface(ui)` 遍历参数 → `FloatDragValueWidget` / `QCheckBox` / bargraph 读数
- 控件改值 → 直接写 `*zone`（DSP 实例内部变量活指针）→ 参数实时生效
- 持久化：`save()` 存文件路径 + 内嵌源码 + 参数快照；`load()` 优先用内嵌源码（.flow 自包含）
- **音频端口**：端口数 = `dsp->getNumInputs()/getNumOutputs()`；Faust 通道 ↔ 单声道 AudioData 端口一一对应（`In 1/Out 1...`）
- **音频处理**：`FaustAudioWorker` 独立线程追帧，多输入按时间戳对齐取帧 → **一次 `compute()` 处理全部通道** → 各输出端口 `pushFrame()`（时间戳 = 输入戳 + 全局输出延迟）

## 控件映射（Faust UI.h → Qt）

| Faust 声明 | 控件 |
|---|---|
| `hslider` / `vslider` / `nentry` | `FloatDragValueWidget`（min/max/step；`[unit:dB]` 元数据转后缀） |
| `button` / `checkbox` | `QCheckBox` |
| `hbargraph` / `vbargraph` | `QLabel` 读数（QTimer 10Hz 轮询） |
| `hgroup` / `vgroup` / `tgroup` | 扁平表单里给 label 加 group 路径前缀 |

## 架构要点

- **继承**：`FaustDataModel : AbstractDelegateModel`；实现 `nPorts()` / `dataType()` / `portCaption()` / `outData()` / `setInData()`，端口数由 `.dsp` 动态决定并通过 `portsAboutToBeInserted/Deleted` 通知图
- **音频图惯例**：QtNodes 只交换 ring queue 句柄，样本不经过图传播；上游 `pushFrame` 经 `AudioTickWaiter` 直接唤醒 worker（20ms 兜底），按全局时间戳追帧
- **一次 compute**：Faust 的延迟线/反馈状态跨通道共享，不能照 AudioGate 逐通道独立处理；worker 将多输入按 `nextTs` 对齐取帧填进 `FAUSTFLOAT*[]`，一次 `compute(count, ins, outs)` 后逐通道 push 输出帧
- **生命周期铁律**：重载/销毁先 `worker->stopProcessing()`（join 线程）→ `teardownDsp()`（清 worker 缓存的 dsp 指针 → `delete m_dsp` → `deleteDSPFactory`）；端口通知必须在 worker 配置好新缓冲区之后，保证通知触发的 `outData()` 看到新 buffer
- **JIT 符号**：来自 `faust.dll`（LLVM 17 已静态链入），链接 `faust.lib` import lib，CMake 后置拷贝到 `bin/`
- **中文路径**：`QString` + `QFile` 读取（Qt 内部 UTF-16，不经 filesystem），源码以 UTF-8 `std::string` 传入 JIT，天然绕过 `u8Path` 坑
- **GUI 线程编译**：加载是低频操作（几十~几百 ms），JIT 在 GUI 线程同步执行；失败时 qWarning + 状态标签红字，端口归零、节点仍在图中
- **实时线程**：worker 入口 `AudioThreadRealtimeGuard(L"Pro Audio")`（MMCSS）；音频路径无锁分配（缓冲区预分配，短临界区仅拷贝句柄）

## 通道语义（重要，易踩坑）

**.dsp 的输入通道 ↔ 节点 In 端口一一对应，输出通道 ↔ Out 端口一一对应。没接线的输入端口 = 静音，不会自动复制。**

Faust 里 `(_ * gl, _ * gr)` 是**两进两出**：第一个 `_` 读 In 1、第二个 `_` 读 In 2。若只把单声道源接到 In 1，In 2 静音 → 右路输出 silence → **只有左耳有声**。这不是节点 bug（worker 逐端口如实处理）。

| 需求 | .dsp 写法 | 测试样例 |
|---|---|---|
| 单声道进 → 双耳都响 | `process = (*(gain)) <: _,_;`（`　<:　` 分叉复制） | `Temp/faustnode/test_stereo_mono.dsp` |
| 双声道各自处理 | `process = (_ * gl, _ * gr);` | `Temp/faustnode/test_stereo.dsp`（须接两路输入） |
| 单声道单耳 | `process = *(gain);` | `Temp/faustnode/test_simple.dsp` |

接线时：FaustNode 的 `Out 1/Out 2` 分别接 AudioDeviceOut 的 `In 1/In 2`（设备 port 0=左、port 1=右）。

## 验证（2026-11-20）

`Temp/faustnode/worker_probe.cpp`（独立 Qt probe，Release/MSVC 编译）实测：
- 单通道：`test_simple.dsp` gain 写 zone=0.25 → 输入 1.0 → 输出 0.25 ✓
- 双通道：`test_stereo.dsp` gainL=0.25/gainR=0.7，输入 1.0/0.5 → 输出 ch0=0.25、ch1=0.35 ✓
- 覆盖：JIT 编译 / 参数枚举 / zone 写回 / 追帧对齐 / compute / 输出帧元数据（960 samples, 48kHz, 32bit）/ 线程生命周期

## 下一版

1. 采样率从音频总线宿主取（现在硬编码 48000，与项目其他音频节点一致）
2. bargraph 读数 → display 电平上报
3. 编译移到 worker 线程 + 双实例交叉淡化（演出态热更新）
4. display 端 JIT 策略（随包 faust.dll / 预编译 C++ / interpreter 后端）
