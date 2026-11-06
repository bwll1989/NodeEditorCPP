# Face Detection

## 1. 节点说明

使用 YOLO11n 人脸检测模型定位画面中的人脸，输出标注图与检测数据。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 无 |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

- **IMAGE**（ImageData）：输入图像。
- **ENABLE**（VariableData）：启停检测（也可在面板点击「启动检测」）。

### 输出

- **IMAGE 0**（ImageData）：标出人脸框的标注图（绿色框 + 置信度文字；「绘制检测框」关闭后不再输出）。
- **RESULT**（VariableData）：检测结果对象，键：`default`（人脸数量）、`detections`（列表，每项含 `score`、`box:[x1,y1,x2,y2]`、`classId`、`className:"face"`）、`width`、`height`。

## 3. 界面与属性

控件位于右侧属性面板（`WidgetEmbeddable=false`，不嵌入节点体）：

- **置信度阈值**（confidence，默认 0.25）：拖拽数值控件
- **NMS 阈值**（nmsThreshold，默认 0.45）
- **最大帧率**（maxFps，默认 15，自动钳制 1–30 FPS）
- **绘制检测框**（drawOverlay，默认开）：关闭后仅更新 RESULT，省去标注图拷贝
- **启停按钮**（enabled，默认关）

模型路径固定为 `./plugins/Models/yolo11n-face-detection.onnx`，首次推理时懒加载。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/confidence` | 读/写 | 置信度阈值（Command 事件 payload 转 double） |
| `/nms` | 读/写 | NMS 非极大值抑制阈值 |
| `/enable` | 读/写 | 启停检测（payload 转 bool；false 时取消推理并清空输出） |

`maxFps`、`drawOverlay` 两个属性存在 Q_PROPERTY 但源码未调用 `registerExternalBinding`，无 OSC 地址，只能通过界面或 `.flow` 持久化设置。

## 5. 行为细节

- **推理调度**：由全局 TimestampGenerator 帧时钟驱动（QueuedConnection），仅当「已启用 + 有图像输入 + 出现新时间戳 + 距上次推理 ≥ 1000/maxFps 毫秒」才调度；推理进行中不重复入队，结束后自动尝试补一轮（合并为最新帧）。
- **预处理**：输入帧先回读为 Mat，长边超过 1280 时等比缩小；模型输入 640×640，blob 归一化 1/255。
- **执行**：ONNX Runtime 推理在 QtConcurrent 线程池执行；优先尝试 CUDA（device 0），失败自动回退 CPU；CUDA 会话用 1 线程，CPU 会话用至多 2 线程。运行期以原子取消标志支持随时中止。
- **后处理**：置信度过滤 → 坐标映射回原图 → NMS → 输出；检测框绘制在图像副本上（绿色框 + "Face xx%" 文字）。
- **输出更新**：RESULT 仅在检测结果与上次不同时更新（QMetaObject::invokeMethod 回主线程）；标注图仅在 drawOverlay 开启时推入输出环形缓冲（时间戳取全局帧计数，去重推送）。
- **停用清理**：`enabled=false` 时取消挂起推理、清空标注图环形缓冲、RESULT 置空；图像输入断开时同样清空标注图输出。
- **持久化**：`save()` 将 `confidence` / `nmsThreshold` / `enabled` / `maxFps` / `drawOverlay` 存入 `values`，`load()` 逐项恢复（缺省回退 0.25/0.45/true/15/true）。

## 6. 使用说明与典型接法

连接图像源后在面板点击「启动检测」（或 ENABLE 端口/OSC `/enable` 置真）；用 RESULT 的 `default` 键统计人数，用 `detections[].box` 裁剪人脸区域。高分辨率场景可将 maxFps 调低以控制 CPU 占用。

## 7. 示例

Camera → Face Detection → Image Switch，多人时切换特写逻辑。

