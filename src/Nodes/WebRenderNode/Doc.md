# Web Render

## 1. 节点说明

将网页离屏渲染为图像，模式对齐 TouchDesigner **Web Render TOP**：

- **独立浏览器进程**（本机 Microsoft Edge / Google Chrome，headless）
- 通过 **Chrome DevTools Protocol (CDP)** 导航与截图
- 输出走项目统一的 **ImageTimestampRingQueue / ImageData**，下游按 `TimestampGenerator` tick 取帧

不依赖 Qt WebEngine / CEF 静态库；需本机已安装 Edge 或 Chrome。

## 2. 数据流

```text
WebRenderBrowserProcess
  Edge/Chrome --headless --remote-debugging-port=0 --user-data-dir=<唯一目录>
       ↓ DevToolsActivePort
WebRenderCdpClient
  Page.navigate / Page.reload
  Page.startScreencast (JPEG + Ack 背压，客户端按 Max FPS 丢帧)
       ↓ JPEG bytes → 线程池 imdecode → BGR Mat
WebRenderDataModel::publishPendingFrame（GUI 线程）
  ImageFrame::fromMat → pushFrame
m_outImage ──→ Image Display / Window Display / …
m_outInfo  ──→ Data Info（loaded / updates / url / title / error …）
```

> 性能提示：TD 用 CEF 共享纹理零拷贝；本节点仍经 JPEG→CPU→GPU。已开浏览器 GPU + screencast，30fps 应明显顺滑；若仍卡可降分辨率或 Max FPS。

## 3. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| Active | Variable | 启停渲染（对齐 TD `active`） |
| URL | Variable | 覆盖面板 URL（`url` 源模式下生效） |
| Reload | Variable | 脉冲为 true 时执行一键重载（见 Reload 按钮） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 共享 ring buffer |
| Info | Variable | `loaded`、`num_handle_updates`、`url`、`title`、`error`、`process_id`、`updates_per_second` 等 |

## 4. 界面参数（对齐 TD）

全部参数平铺显示（无折叠）。

| 参数 | 对应 TD | 说明 |
|------|---------|------|
| Active | Active | 启停 |
| Source | Source | `URL or File` / `HTML (DAT)` |
| URL | URL or File | `http(s)://`、`file:///`，运行中修改即实时导航 |
| HTML | DAT | 写入临时 HTML 后以 `file://` 打开 |
| Resolution | Custom Resolution | 视口宽高，停止调整 500ms 后自动重启浏览器进程（防抖） |
| Max FPS | Maximum Render Frame Rate | CDP 截图上限 |
| DPI Scale | DPI Scaling Factor | `Emulation.setDeviceMetricsOverride`，即时生效无需重启 |
| Advanced ▸ Only Update when Loaded | Only Update when Loaded | 未 load 完成前不截图（导航 15s 超时后自动恢复推流） |
| Advanced ▸ Restart if Process Died | Restart if Process Died | 子进程异常退出后自动拉起 |
| Reload | Reload Source / Reload / Restart | 一键重载：按状态自动选最轻量方式；**Shift+点击 = 强制重启进程** |

每个节点实例使用**唯一** `user-data-dir`（TD 同样要求不可共享）。

### Reload 按钮的合并逻辑

| 当前状态 | 行为 |
|----------|------|
| 浏览器进程未运行 | 拉起浏览器（等价原 Restart） |
| 进程在但 CDP 未连接 | 重连调试端口；端口未就绪则重启进程 |
| CDP 就绪 + HTML 源 | 重写临时 HTML 并加载（等价原 Reload Source） |
| CDP 就绪 + 页面已被站内跳转 | 导航回配置 URL（等价原 Reload Source） |
| CDP 就绪 + 页面仍在配置 URL 上 | 轻量 `Page.reload`（等价原 Reload Page） |
| Shift+点击 | 无条件重启浏览器进程 |

## 5. 使用说明

1. 添加 **Web Render**，勾选 Active，默认打开 `https://example.com`。
2. 输出接到 **Image Display** 或 **Window Display**。
3. Info 可接 Data Info 查看 `loaded` / `updates_per_second`。
4. 本地文件：`file:///D:/path/page.html`；或切到 HTML 源直接贴内容。
5. 需能访问目标页（内网仪表盘、本机 WebInterface 等填 `http://127.0.0.1:…` 即可）。

## 6. 实现要点

| 项目 | 说明 |
|------|------|
| 浏览器 | 优先 Edge，其次 Chrome；启用 GPU（ANGLE/D3D11） |
| 取帧 | `Page.startScreencast` JPEG，勿用轮询 PNG `captureScreenshot` |
| 解码 | `QtConcurrent` 后台 `imdecode`，积压丢帧 |
| 帧合并 | pending Mat + `publishPendingFrame`，与 Camera/RTSP 一致 |
| 时间戳 | `TimestampGenerator::getCurrentFrameCount()` |
| 共享纹理 | 仍非零拷贝；要对齐 TD 流畅度上限需 D3D/GL 共享纹理 |

## 7. 示例

Web Render（Active，URL=`http://127.0.0.1:8080`）→ Image Scale → Window Display，将 Web 仪表盘投到辅屏。
