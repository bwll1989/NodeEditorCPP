# NDI In

## 1. 节点说明

从局域网接收 NDI 视频流并输出为图像。可扫描并选择 NDI 源，支持远程控制启停与切源。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

- **SOURCE**（VariableData）：NDI 源名称（字符串）。
- **ENABLE**（VariableData）：开始/停止接收（布尔）。

### 输出

- **IMAGE**（ImageData）：当前 NDI 视频帧。

## 3. 界面与属性

控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 含义 |
|------|------|
| 源下拉列表 | 发现到的 NDI 源（按 `p_ndi_name`），选择即切源 |
| 刷新按钮 | 重新扫描局域网 NDI 源 |
| 启停按钮 | 开始/停止接收（勾选态） |
| 状态显示 | 连接状态指示 |

可外部控制的属性（Q_PROPERTY）：`sourceName`（NDI 源名称）、`enable`（是否接收）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/source` | 读写 | 设置 NDI 源名称（订阅 Command，切源） |
| `/enable` | 读写 | 启停接收（bool，绑定启停按钮） |
| `/refresh` | 命令 | 触发源列表刷新，回读 true 脉冲 |
| `/sourceName` | 只读状态 | 当前源名回读（member 绑定未订阅 Command，外部写无效；模型就绪时推送初值） |

> 注意：设置源请写 `/source`；`/sourceName` 仅用于回读当前值。

## 5. 行为细节

- **独立接收线程**：NDI 发现与接收在专用 QThread（HighestPriority）中运行，主线程只做帧上传，避免 UI 阻塞；线程空闲时以 10/50 ms 步进休眠。
- **源发现**：`NDIlib_find` 常驻扫描（show_local_sources=true），刷新请求在 worker 线程等待最多 100 ms 后取当前源列表回填下拉框。
- **切源**：按 `p_ndi_name` 连接；切源会销毁旧接收器并按 `NDIlib_recv_create_v3` 重建（最高带宽、BGRX/BGRA 色彩、不处理隔行）。
- **色彩转换**：BGRA/BGRX 直接包装；RGBA/RGBX/UYVY 转 BGRA；帧数据深拷贝后（NDI 缓冲立即释放）经队列合并（只保留最新一帧）交给 GUI 线程。
- **输出管线**：GUI 线程将 cv::Mat 上传 GPU 纹理（`ImageGpuUpload`，模型就绪时预热）并封装 ImageFrame，按时间戳去重后推入输出环形缓冲；GPU 上传失败丢弃该帧。
- **停止清理**：停止接收时释放挂起帧、清空输出缓冲、重置去重时间戳。
- **端口语义**：SOURCE 等价写 `/source` 属性（asString）；ENABLE 等价写 enable 属性（asBool）；断开输入不触发动作。
- **持久化**：`save()` 将 `currentSender`、`isReceiving` 写入 `values.*`；`load()` 兼容旧版顶层键格式，恢复后按值启停。
- **资源清理**：析构时反注册事件总线、停止接收、等待接收线程退出（最多 3 s，超时强制终止）。

## 6. 使用说明与典型接法

1. 点击刷新，从列表选择 NDI 源。
2. 开始接收后连接下游图像节点。
3. 可用 SOURCE、ENABLE 端口或 `/source`、`/enable`、`/refresh` 外部控制。

## 7. 示例

现场 NDI 摄像机 → NDI In → Pose Detection → 大屏预览。

