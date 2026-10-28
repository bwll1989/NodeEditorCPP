# Web Render

## 1. 节点说明

把网页离屏渲染成图像，模式对齐 TouchDesigner **Web Render TOP**。

- 每个节点拉起自己的浏览器进程（本机 Microsoft Edge，没有则用 Google Chrome，`--headless=new`）
- 通过 Chrome DevTools Protocol 导航，并用 `Page.startScreencast` 连续取 JPEG
- 图像经 GPU 上传后写入 `ImageTimestampRingQueue`，下游按 `TimestampGenerator` 的 tick 取帧，节点不逐帧 `dataUpdated`

不依赖 Qt WebEngine / CEF。本机需要已安装 Edge 或 Chrome。面板在侧栏，不嵌在节点上。

缓存目录是系统临时目录下的 `NodeEditorWebRender/<uuid>/`，每个实例一份。节点销毁时删除。HTML 源会写成该目录里的 `webrender_source.html`，再以 `file://` 打开。

## 2. 数据流

```text
WebRenderBrowserProcess
  Edge/Chrome --headless=new --enable-gpu --remote-debugging-port=0
  --user-data-dir=<唯一目录> --window-size=<宽>,<高>
       ↓ DevToolsActivePort
WebRenderCdpClient
  Page.navigate / Page.reload(ignoreCache)
  Page.startScreencast（JPEG quality 70，Ack 背压，按 Max FPS 丢帧）
       ↓ JPEG → 线程池 imdecode（同时最多 2 帧，再来的丢掉）
WebRenderDataModel::publishPendingFrame
  ImageFrame::fromMat → GPU → pushFrame
IMAGE ──→ Image Display / Window Display / …
Info  ──→ 状态（loaded / url / title / error / updates_per_second …）
```

浏览器开了 GPU 合成（Windows 上 `--use-angle=d3d11`）。帧仍要 JPEG 解码再上传，不是共享纹理零拷贝。卡的时候先降分辨率或 Max FPS。

## 3. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| Active | Variable | 布尔启停。`true` 启动浏览器，`false` 停掉。与面板 Active、OSC `/active` 同步 |
| URL | Variable | 写入 URL。只在 Source 为 `URL or File` 且 CDP 已连接时立刻导航。OSC `/url` 同步 |
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
| `num_handle_updates` | CDP 收到的页面更新次数 |
| `url` | 当前页地址；还没导航时用面板里的 URL |
| `title` | 页面标题 |
| `error` | 最近一次错误，没有则为空 |
| `process_id` | 浏览器进程 PID |
| `debug_port` | CDP 端口 |
| `updates_per_second` | 实际上传到图像输出的帧率 |
| `active` | 是否处于启动状态 |
| `browser` | 实际启动的 Edge/Chrome 路径 |

## 4. 界面参数

参数全部平铺，没有折叠。

| 参数 | 默认 | 说明 |
|------|------|------|
| Active | 关 | 启停。打开工程时若保存为开，延迟约 600ms 再启动，避免一加载就拉浏览器 |
| Source | URL or File | `URL or File` 或 `HTML (DAT)`。切换后若已连接，立即按新源加载 |
| URL | `https://example.com` | `http(s)://` 或 `file:///`。运行中改完并结束编辑会马上导航 |
| HTML | 一段示例页 | 只在 HTML 源下显示。改文字不会自动刷新，要点 Reload |
| Resolution | 1280 × 720 | 宽 16–7680，高 16–4320。改完先更新视口，停止调整 500ms 后再重启浏览器 |
| Max FPS | 15 | 0.5–60。立刻作用到截图节流，不用重启 |
| DPI Scale | 1.00 | 0.25–4。`Emulation.setDeviceMetricsOverride`，立刻生效 |
| Only Update when Loaded | 开 | load 完成前不推帧。导航超过 15 秒仍未 load，自动恢复推流 |
| Restart if Process Died | 开 | 子进程异常退出约 800ms 后重新拉起 |
| Reload | — | 按当前状态选最轻的恢复方式。按住 Shift 再点 = 强制重启进程 |

启动失败（找不到浏览器、进程拉起失败）会自动取消 Active。

### Reload

Active 未勾选时，Reload 和 Shift+Reload 都不做事。

| 当前状态 | 行为 |
|----------|------|
| Active 已开，浏览器未运行 | 拉起浏览器 |
| 进程在，CDP 未连接 | 用已有调试端口重连；端口还没有则重新拉起 |
| CDP 已连接，HTML 源 | 重写临时 HTML 再打开 |
| CDP 已连接，页面已跳到别的地址 | 导航回面板里的 URL |
| CDP 已连接，页面仍停在配置 URL | `Page.reload`，忽略缓存 |
| Shift+点击 | 停掉进程再拉起 |

## 5. 使用说明

1. 添加 **Web Render**，勾选 Active。默认打开 `https://example.com`。
2. IMAGE 接到 **Image Display** 或 **Window Display**。
3. Info 可接到 Data Info，看 `loaded` 和 `updates_per_second`。
4. 本地文件填 `file:///D:/path/page.html`。或把 Source 改成 HTML，贴内容后点 Reload。
5. 本机页面直接填 `http://127.0.0.1:…`。

## 6. 保存内容

工程里保存：`active`、`sourceMode`、`url`、`html`、`width`、`height`、`maxFps`、`dpiScale`、`updateWhenLoaded`、`autoRestart`。不保存浏览器进程和临时目录。
