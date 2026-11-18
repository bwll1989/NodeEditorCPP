# Web Render

## 1. 节点说明

> ⚠️ **版本要求**：本节点为 **Advance（进阶版）** 功能，需有效授权后才能加载使用。

把网页离屏渲染成图像，模式对齐 TouchDesigner **Web Render TOP**。

- 内嵌 **CEF（Chromium Embedded Framework 156 / Chromium 156）** 离屏浏览器，多节点共享一个进程内运行时，浏览器子进程统一由 `FlowCefHelper.exe` 承担（Flow.exe 永不充当 CEF 子进程）
- 帧输出两条路径，自动选择：
  - **GPU 共享纹理**（默认）：`OnAcceleratedPaint` → D3D11 `OpenSharedResource1` → `CopyResource` 到自有纹理 → `WGL_NV_DX_interop2` 映射 → FBO 拷入普通 GL 纹理，全程零 JPEG、零 CPU 位图拷贝
  - **CPU OnPaint 兜底**：BGRA8 软件位图 → `ImageFrame::fromBgra8Mat`。探测失败（无 D3D11 / 无 WGL_NV_DX_interop2 / ANGLE 上下文）或 GPU 链路连续 5 帧失败时自动降级，Info `frame_path` 可观测
- 图像写入 `ImageTimestampRingQueue`，下游按 `TimestampGenerator` 的 tick 取帧，节点不逐帧 `dataUpdated`

不依赖 Qt WebEngine，不再需要本机安装 Edge/Chrome。

CEF 运行时（`libcef.dll`、`icudtl.dat`、`v8_context_snapshot.bin`、`locales/` 等）随构建平铺到 `Flow.exe` 目录；`FlowCefHelper.exe` 同目录。

缓存目录是系统临时目录下的 `NodeEditorWebRender/`：`cef/` 为全进程共享的 CEF profile（多节点共用），HTML 源写成同目录的 `html_<uuid>.html` 再以 `file://` 打开。首个节点启动前整目录清扫（顺带收掉旧版 Edge 后端遗留的 UUID profile 孤儿）。

> ⚠️ **管理员权限**：Windows 管理员权限运行 Flow 时，CEF 的 auto de-elevation 会导致初始化失败（Web Render 报「CEF 运行时初始化失败」）。请以**普通权限**启动 Flow（双击默认即为普通权限）。
> 注意：不要试图给 Flow.exe 传 `--do-not-de-elevate`——主程序的命令行解析器不认识该开关，会直接退出。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 末端口为 Reload（重载页面触发） |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| Active | Variable | 布尔启停。`true` 创建离屏浏览器，`false` 优雅关闭。与面板 Active 同步，状态经 OSC `/active` 反馈 |
| URL | Variable | 写入 URL。运行中变化立刻导航。变化经 OSC `/url` 反馈 |
| Reload | Variable | 值为 `true` 时执行一次 Reload。Active 未勾选时无效 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 共享 ring buffer。只在新帧上传成功后更新 |
| Info | Variable | 见下表。状态变化时更新；渲染中大约每秒刷新一次帧率 |

Info 字段：

| 字段 | 说明 |
|------|------|
| `loaded` | 当前页面是否已 load |
| `num_handle_updates` | 累计推送到图像输出的帧数 |
| `url` | 当前页地址；还没导航时用面板里的 URL |
| `title` | 页面标题 |
| `error` | 最近一次错误，没有则为空 |
| `process_id` | Flow 主进程 PID（CEF 子进程由 FlowCefHelper 承担，不再是独立浏览器 PID） |
| `debug_port` | 恒为 0（CEF 后端无 CDP 调试端口，键名兼容保留） |
| `updates_per_second` | 实际推送到图像输出的帧率 |
| `active` | 是否处于启动状态 |
| `browser` | `FlowCefHelper (libcef 156.0.8078.100)` |
| `engine` | `cef` |
| `frame_path` | `gpu`（共享纹理直通）或 `cpu`（OnPaint 兜底） |

## 3. 界面与属性

参数全部平铺，没有折叠。

| 参数 | 默认 | 说明 |
|------|------|------|
| Active | 关 | 启停。打开工程时若保存为开，延迟约 600ms 再启动，避免一加载就建浏览器 |
| Source | URL or File | `URL or File` 或 `HTML (DAT)`。切换后若在运行，立即按新源加载 |
| URL | `https://example.com` | `http(s)://` 或 `file:///`（裸路径/无 scheme 域名自动补全）。运行中改完并结束编辑会马上导航 |
| HTML | 一段示例页 | 只在 HTML 源下显示。改文字不会自动刷新，要点 Reload |
| Resolution | 1280 × 720 | 宽 16–7680，高 16–4320。**即时生效**（CEF WasResized，不重启任何进程） |
| Max FPS | 15 | 0.5–60。节点侧节流（CEF 内部以约 30fps 推帧，按需丢弃），不用重启 |
| DPI Scale | 1.00 | 0.25–4。视口尺寸 = Resolution × DPI，即时生效 |
| Only Update when Loaded | 开 | load 完成前不推帧 |
| Restart if Process Died | 开 | 仅存档兼容。CEF 渲染进程异常由 Chromium 自愈，此开关不再有实际作用 |
| Reload | — | 按当前状态选最轻的恢复方式。按住 Shift 再点 = 重建浏览器实例 |

启动失败（CEF 运行时初始化失败、离屏浏览器创建失败）会自动取消 Active。

### Reload

Active 未勾选时，Reload 和 Shift+Reload 都不做事。

| 当前状态 | 行为 |
|----------|------|
| Active 已开，浏览器未创建 | 重新走启动流程 |
| HTML 源 | 先重写临时 HTML，再按下面两条导航 |
| 目标 URL 与当前页相同 | `reloadPage()` |
| 目标 URL 不同 | `loadUrl()` 导航过去 |
| Shift+点击 | 优雅关闭当前浏览器实例并重建（不再有"进程重启"概念） |

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/active` | 只读 | 启动状态（布尔）；变化时自动推送反馈。 |
| `/url` | 只读 | 当前配置的 URL；变化时自动推送反馈（不是页面实际地址，页面地址看 Info 输出）。 |

两个地址均注册了带属性 member 的绑定：模型就绪时各推送一次初始值，此后属性每次变化自动推送。**外部写入均无效**，启停 / 改 URL / Reload 只能通过界面或输入端口。分辨率、Max FPS、DPI 等其余参数无对应 OSC 地址。

## 5. 行为细节

```text
CefRuntime（进程级单例，init-once）
  CefInitialize：no_sandbox + windowless_rendering_enabled
  + external_message_pump（泵=message-only 窗口 + SetTimer 兜底心跳
  + OnScheduleMessagePumpWork 低延迟路径，全部回调落 GUI 主线程）
       ↓ 每节点一个
CefOsrBrowser（SetAsWindowless）
  ├─ GPU 路径：OnAcceleratedPaint
  │    NT handle（无 keyed mutex，回调内有效）→ OpenSharedResource1
  │    → CopyResource 到自有 D3D 纹理 → wglDX interop lock
  │    → FBO glCopyTexSubImage2D 拷入普通 GL 纹理（与 interop 解耦，
  │      下游在共享组任意上下文 bind 均合法）
  │    → ImageFrame::fromTexture → pushFrame
  └─ CPU 路径：OnPaint BGRA8（预乘 alpha）→ clone 拷出 → 队列
       → ImageFrame::fromBgra8Mat → pushFrame
IMAGE ──→ Image Display / Window Display / …
Info  ──→ 状态（loaded / frame_path / updates_per_second …）
```

- CEF 以 `windowless_frame_rate≈30` 内部推帧；`Max FPS` 在节点侧节流（按接受间隔丢弃），两条帧路径共用同一节流与 `Only Update when Loaded` 门控。
- 分辨率 / DPI 变更走 `WasResized` 直达渲染器，**不再重启进程**（旧 Edge 后端 500ms 防抖重启已废除）。
- GPU 链路连续 5 帧失败 → 熔断本会话 GPU 路径，队列化重建浏览器降级 CPU；不在 CEF 泵回调内同步重入。
- 弹窗全部阻断；禁用后台节流/遮挡裁剪（disable-features=CalculateNativeWinOcclusion 等），后台节点持续出帧。
- 启动失败自动回滚 Active 为关。
- 退出协议：节点析构零阻塞（摘回调 + `close()`），进程退出钩子统一排空 `OnBeforeClose` 后 `CefShutdown` —— Flow 关闭秒退，无残留进程。
- 持久化：`values` 子对象 10 键（active / sourceMode / url / html / width / height / maxFps / dpiScale / updateWhenLoaded / autoRestart）；打开工程时 `active=true` 延迟约 600ms 自动启动。不保存浏览器实例与临时 HTML。

## 6. 使用说明与典型接法

1. 添加 **Web Render**，勾选 Active。默认打开 `https://example.com`。
2. IMAGE 接到 **Image Display** 或 **Window Display**。
3. Info 可接到 Data Info，看 `loaded`、`frame_path` 和 `updates_per_second`。
4. 本地文件填 `file:///D:/path/page.html`（中文路径可直接用）。或把 Source 改成 HTML，贴内容后点 Reload。
5. 本机页面直接填 `http://127.0.0.1:…`。
6. 需要强制回到初始状态时 Shift+Reload 重建浏览器实例。

## 7. 保存内容

工程里保存：`active`、`sourceMode`、`url`、`html`、`width`、`height`、`maxFps`、`dpiScale`、`updateWhenLoaded`、`autoRestart`。不保存浏览器实例、缓存与临时 HTML。
