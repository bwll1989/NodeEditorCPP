# Web Render 节点 CEF OSR 重构实现规划

> 规划稿 2026-11-18。决策链背景（WebView2+WGC / CEF OSR / OGraf / 数据边界对照评估）见
> `.workbuddy/memory/2026-11-18.md`。用户拍板：按 CEF 模式重构。
> 本文档是实施蓝图，动手前不需再评审结构，只需按阶段推进。

---

## 1. 目标

把 Web Render 节点从「外部 Edge/Chrome 进程 + CDP screencast JPEG 软解」重构为
「进程内 CEF OSR（off-screen rendering）+ GPU 共享纹理直通」。

**消灭的历史包袱**（每项都有本机实证）：

| # | 包袱 | 现状证据 |
|---|------|----------|
| 1 | Edge 157 launcher 模型 → `processDied` → 自动重启死循环，泄漏浏览器树 | test3.flow "一直显示自动重启中"（11-18 诊断，未修） |
| 2 | 退出挂起：dtor 零阻塞化只是止血，进程树关闭时序仍不受控 | 16:06-16:12 挂起记录 |
| 3 | DevTools 端口轮询（100ms×100 次）+ 探活猜测 | WebRenderBrowserProcess::pollDevToolsPort |
| 4 | JPEG 编码（浏览器内）→ imdecode 软解（线程池）→ 上传，CPU 管线 | onFrameCaptured → onFrameDecoded |
| 5 | 依赖系统装有 Edge/Chrome；display 机器部署假设被破坏 | Doc.md「本机需要已安装 Edge 或 Chrome」 |
| 6 | profile 目录每实例 UUID + 启动期清扫（绕开文件锁的补丁设计） | makeUniqueUserDir |

**新增能力**：GPU 帧直通（1080p60/4K30 可达）、Web 音频 PCM 可捕获（后续音频总线），
为后续 Three.js/p5.js 高分辨率回放铺路。

## 2. 保持不变（兼容契约，下游与工程文件零迁移）

- 端口协议：3 入（Active/URL/Reload）2 出（IMAGE/Info）；IMAGE 仍是 `ImageData`
  → `ImageTimestampRingQueue`，下游按 tick 取帧
- Info 键：`loaded/url/title/error/updates_per_second/active` 原义保留；
  `process_id` 改指 CEF 主进程，`debug_port` 恒 0（废弃），新增 `frame_path`（gpu/cpu）、`engine`（cef）
- 面板控件（WebRenderInterface）全部保留；状态文案微调
- `.flow` 持久化 10 键全兼容（active/sourceMode/url/html/width/height/maxFps/
  dpiScale/updateWhenLoaded/autoRestart），新增键只加不删（向后兼容）
- OSC `/active` `/url` 外部绑定不动

## 3. 架构定案

### 3.1 组件与文件

```
新增库 src/Common/CefBox/（SHARED，CefBox.dll）
  CefRuntime.{h,cpp}    进程级单例：引用计数初始化/关闭、external_message_pump 调度、
                        resource/cache 目录解析、CefApp 骨架
  CefOsrBrowser.{h,cpp} 每实例一个：CefClient + RenderHandler/LoadHandler/DisplayHandler/
                        RequestHandler(/AudioHandler P3)；navigate/loadHtml(临时文件)/
                        executeJs/resize/setFpsPacing/sendInput/close
  CefGlInterop.{h,cpp}  WGL_NV_DX_interop2 封装：自有 D3D11 设备、shared NT handle
                        → GL 纹理注册、逐帧 keyed-mutex acquire → 池化拷贝 → release
  CefBox/Doc.md
  subprocess/main.cpp   FlowCefHelper.exe（CefExecuteProcess，数行）
修改：
  WebRenderNode/CMakeLists.txt   链 CefBox、d3d11；删 WebSockets（CDP 专用）
  WebRenderDataModel.hpp         m_browser/m_cdp → m_cef（CefOsrBrowser）；
                                 startPipeline/stopPipeline/reloadNow/onFrame* 重写；
                                 publishPendingFrame 双轨（fromBgra8Mat / fromTexture）
  WebRenderInterface.hpp         状态文案微调；Info 表同步 Doc.md
删除：WebRenderBrowserProcess.{h,cpp}、WebRenderCdpClient.{h,cpp}
主程序/打包：
  <bin>/cef/ 运行时目录（libcef.dll、icudtl.dat、resources/、locales/、
  v8_context_snapshot.bin、FlowCefHelper.exe）随安装包；发布脚本拷贝
```

### 3.2 消息循环与线程模型（决策点 D1，定案：external_message_pump）

- `CefSettings.windowless_rendering_enabled=true`、`external_message_pump=true`、
  `multi_threaded_message_loop=false` → **全部 CEF 回调在 GUI 主线程**。
- 理由（决定性）：`ImageGpuUpload` 的 upload/runGl 契约是「须 GUI 线程」。回调在
  GUI 线程 = OnPaint/OnAcceleratedPaint 里直接推帧，零投递、零 marshaling；
  也天然规避现在 CDP WebSocket 跨线程搬运 + 线程池解码的整条链。
- 泵驱动：`OnScheduleMessagePumpWork(delay_ms)` → `QTimer::singleShot`/
  事件投递 → `CefDoMessageLoopWork()`。GUI 空闲时不空转（CEF 会给 schedule 提示）。
- 风险预案：若实测 GUI 忙时 CEF 明显饿死（帧率塌陷），切
  `multi_threaded_message_loop=true` + 回调经 `QMetaObject::invokeMethod`
  投递（CefOsrBrowser 内部统一 marshal，接口不变）——架构上留好这条退路。

### 3.3 帧路径双轨与自动降级（决策点 D2，定案）

- **A 路（先落地，保底）**：`OnPaint` 直接给 BGRA 位图 →
  `ImageFrame::fromBgra8Mat`（跳过 imdecode 与色彩转换，保留 CPU 缓存，
  与现行为完全同型，只是把"JPEG→解码"换成"现成位图"）。
- **B 路（核心价值）**：`shared_texture_enabled=true` → `OnAcceleratedPaint`
  拿 D3D11 shared NT handle → 自有 D3D11 设备 `OpenSharedResource1` →
  `wglDXOpenDeviceNV/wglDXRegisterObjectNV` 注册进 ImageGpuUpload 离屏
  GL 上下文（`runGl` 内执行）→ 每帧 keyed-mutex acquire → **池化纹理拷贝**
  （glCopyTexSubImage2D，GPU 内亚毫秒）→ unlock → `ImageFrame::fromTexture`。
  不跨帧持有 CEF 纹理（CEF 小环复用），零 CPU 像素拷贝。
- **探测降级**：`wglDXOpenDeviceNV` 拿不到 / 首帧 interop 失败 → 自动落 A 路，
  Info 标 `frame_path=cpu`。双路共用同一 pushFrame 出口。

### 3.4 shim 与页面控制

- HTML 源维持「写临时文件 + file:// 加载」（现有行为，Doc 兼容；data URI 有长度限制不做）
- 导航完成后注入 `flow` shim JS（`CefFrame::ExecuteJavaScript`，等价原 CDP
  Runtime.evaluate 通道）：`flow.setParam/setTime/onFrame` 契约与上轮评估一致，
  OGraf 对齐（lifecycle+goToTime）另立任务，不在本重构内
- 交互下行（SendMouseEvent/KeyEvent）与音频（CefAudioHandler）：**本轮不实现**
  （2026-11-18 拍板暂缓），CefOsrBrowser 预留虚接口位即可

### 3.5 进程与 Job 模型

- `browser_subprocess_path = <bin>/FlowCefHelper.exe`（独立子进程工程，
  main 里 `CefExecuteProcess` 即退）——不复用 Flow.exe 自兼：单实例互斥、
  `--restart-delay-ms`、CrashDump 参数会互相干扰
- `ms::assignChildToJob(pid)` 挂 FlowCefHelper 树 → KILL_ON_JOB_CLOSE 陪葬
  （幽灵 socket 铁律延伸）；Flow 崩溃时 helper 不残留
- cache：`root_cache_path = Temp/NodeEditorWebRender/cef-root/`，进程级共享单份
  （多浏览器共用 request context）；孤儿清扫沿用 makeUniqueUserDir 启动期模式
  （目录改为 cef-root，同样"下次启动时必无文件锁"）
- sandbox：首发禁用（不链 cef_sandbox.lib），降低构建面；记录为后续开关

## 4. 阶段计划（P0→P4，各阶段独立 commit 可回滚）

### P0 CEF 运行时底座（约 2 天）
1. CEF 二进制选型：cef-builds.spotifycdn.com **Windows64 Minimal** 发行包，
   取 Chromium 当前 stable 对应 SP build；**版本号锁进 CefBox/CMakeLists.txt
   注释**（同 build/assimp 惯例）；Release only
2. CefBox.dll：CefRuntime 单例（init/refcount/shutdown、external pump 接 Qt）、
   CefOsrBrowser 骨架（GetViewRect/OnPaint/生命周期回调）、libcef_dll_wrapper
   静态编译（源码随发行包，手写 CMake target，**不依赖 CEF 官方宏**
   ——规避其对 CMake 版本的挑剔）
3. FlowCefHelper 子进程工程 + Job 挂接
- 验收：独立测试工程加载 example.com，OnPaint 帧计数进 qDebug；
  Flow 启动/退出 CEF 不泄漏进程（任务管理器核对）

### P1 节点切换：CPU 帧直通替换 JPEG（约 1 天）
> **P0 已完成（2026-11-18）**：CEF 156.0.31+ge603d97+chromium-156.0.8078.100
> （sha1 d179fb25…）落地 `3rdParty/cef/`；`src/Common/CefBox/`（CefRuntime +
> CefOsrBrowser + FlowCefHelper + cefbox_test）编译通过；冒烟测试 PASS：
> 292 帧/10s（≈30fps OnPaint）、load 238ms、优雅关闭零残留进程。
> **坑 ①：CEF 156 auto de-elevation**——提权（管理员）运行的宿主进程在
> CefInitialize 早期被 CEF 自动降权重启，原进程 exit 38（CEF issue #4326，
> OnBeforeCommandLineProcessing 里补开关已无效）。缓解：非提权启动不受影响
> （Explorer 正常启动 Flow 无感）；测试/调试时命令行带 `--do-not-de-elevate`；
> P1 在 CefRuntime 增加提权检测给出明确错误，必要时再评估 IAT patch 方案。
> **坑 ②：CEF 156 中 `windowless_frame_rate` 只在 CefBrowserSettings**，
> CefSettings 已无此字段；`CefApp` 因 CefBaseRefCounted 纯虚不可直接实例化，
> 子进程需自定义空 App 类 + IMPLEMENT_REFCOUNTING。

### P1 节点切换：CPU 帧直通替换 JPEG（约 1 天）
1. WebRenderDataModel 换 m_cef；`OnPaint(BGRA)` → `ImageFrame::fromBgra8Mat`
   → `pushFrameToImageBufferDedup`（复用现有防抖/去重/FPS 统计）
2. startPipeline/stopPipeline/reloadNow 按 Doc.md 语义表重写
   （分辨率变更**不再需要重启进程**——`WasResized` 即可，防抖逻辑简化）
3. publishInfo 适配；WebRenderBrowserProcess/WebRenderCdpClient 停用（P4 删）
- 验收：wr_hang.flow、test3.flow、example.com、中文路径 file:/// 冒烟；
  1080p≥30fps；**多节点 active 时关 Flow 秒退**；重启死循环 bug 病灶消失
  （无外部进程即无 launcher 模型）；「自动重启中」不再出现

### P2 GPU 共享纹理（约 3 天，全计划最高风险）
1. CefGlInterop：D3D11(BGRA_SUPPORT) 设备 + WGL_NV_DX_interop2；
   `CefAcceleratedPaintInfo`（shared NT handle + keyed mutex + dirty rects）
   → 池化 GL 纹理拷贝 → `makeTextureLifetime` → `ImageFrame::fromTexture`
2. `shared_texture_enabled` 打开；逐帧 acquire/release；丢帧策略对齐 Max FPS
3. 降级探测（P1 路径兜底）；Info `frame_path` 上报
- 验收：GPU 路径 1080p60、4K30；imdecode CPU 尖峰归零（任务管理器）；
  3 节点并发稳定；驱动矩阵至少 NV + Intel 各一台

### P3 交互与音频（【暂缓】2026-11-18 用户拍板：本轮不做）
- SendInput 事件下行、CefAudioHandler PCM → 后续按需立项（音频总线 T3/T4/T5 待办时再启动）
- 本轮仅在 CefOsrBrowser 预留虚接口位，不实现不测试；P4 发布不依赖本阶段

### P4 清理与部署（约 1 天）
1. 删 WebRenderBrowserProcess.*、WebRenderCdpClient.*；CMakeLists 去 WebSockets
2. Doc.md 重写（CEF 章节替换 Edge/CDP 描述，行为表保持结构）；info 键表更新
3. 发布链：cef/ 运行时随安装包（约 +60~70MB 压缩后）；Portable/Green 版确认
   免安装可用（CEF 不写注册表，天然绿色）
4. 全量回归：打开/退出/多工程切换/重启打开（restartAndOpenFlow 链）
- 验收：bin 体积、退出时序、老 .flow 打开渲染正常

**合计约 7 个工作日**（P0 2 + P1 1 + P2 3 + P4 1；交互/音频已拍板后置。
若 P2 驱动兼容受阻，P1 的 CPU 路径已可独立发布——比现状快一截，P2 顶部再挂 +2 天缓冲）。

## 5. 风险与预案

| 风险 | 影响 | 预案 |
|------|------|------|
| external pump 与 Qt 调度失配（GUI 忙 → CEF 饿死） | 帧率塌 | D1 预留 multi_threaded_message_loop 切换，CefOsrBrowser 内统一 marshal |
| interop 驱动差异（NV/AMD/Intel） | P2 不可用 | P1 CPU 路径永久兜底 + 自动降级探测，功能不劣于现状且已快一截 |
| CEF 宏与工程 CMake 版本冲突 | P0 卡壳 | 手写 wrapper/link target，绕开 CEF Macros（仅链接 libcef.lib + wrapper） |
| libcef 崩溃 = Flow 崩（对比 Edge 方案崩溃域隔离） | 稳定性 | CrashDump 已在位可归因；renderer 崩溃仍在子进程隔离；接受并记录 |
| CEF 回调阻塞 GUI（重页面 JS） | 界面卡 | 同 D1；帧推送带 tryLock 丢帧 |
| 多节点初始化/退出时序 | 挂起复发 | CefRuntime refcount + 退出协议：全 browser CloseBrowser(true) → OnBeforeClose 计数归零 → CefShutdown；**在 P0 就写退出协议测试**（历史教训：析构路径问题都是上线后炸的） |
| 中文/非常规路径 | 静默失败 | CEF 全 API 宽字符（std::wstring ← QString::toStdWString），**不过任何 ACP 窄转换**（u8Path 铁律的宽字符形态） |

## 6. 测试矩阵（P4 全量跑）

- 冒烟：wr_hang.flow / test3.flow / example.com / file:/// 中文路径 / HTML 源
- 回归：退出零挂起（3 节点 active 关 Flow）×20 次；重启打开工程链；分辨率拖动
- 性能：1080p30 / 1080p60 / 4K30 帧率与 CPU 占用，对照 JPEG 基线记录
- 破坏性：taskkill FlowCefHelper 树 → 自动恢复；断网加载 → 超时状态；错误 URL
- 部署：无 Edge 的干净机器（display 同配置）上验证 CEF 自足

## 7. 范围外（记录待办，不做）

- 交互下行 / Web 音频捕获（2026-11-18 拍板本轮暂缓；音频总线 T3/T4/T5 立项时一并启动）
- OGraf/shim 契约层、glTF 数据边界节点（另立任务，讨论已留档）
- display 端 Web 节点消费与 CEF 运行时分发（display 侧若要跑 Web 节点，
  cef/ 目录同样随 bin/display 打包——待 display 侧需求确认后立项）
- per-browser 独立 request context / 独立 cache（单实例够用，P4 后按需）

## 8. 决策记录（2026-11-18 用户拍板）

1. CEF 版本：Chromium 当前 stable 对应最新 SP build（默认，未提异议）
2. 交互/音频（原 P3）：**本轮不做**，接口位预留，按需另立项
3. Edge/CDP 旧后端：**不留双端过渡，直接替换**——P1 切换完成后旧文件即死代码，
   P4 删除；不做 backend 配置开关
